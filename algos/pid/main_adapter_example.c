/**
 * @file    main_adapter_example.c
 * @brief   多控制器适配示例：演示位置式、增量式、串级、自适应PID的集成与切换。
 * @note    所有硬件操作均通过模拟的 HAL 实现，可在无硬件环境下编译运行。
 */

#include "pid_positional.h"
#include "pid_incremental.h"
#include "pid_cascade.h"
#include "pid_adaptive.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*===========================================================================*/
/* 1. 模拟硬件抽象层实现                                                     */
/*===========================================================================*/

static uint32_t simulated_time_us = 0U; /* 模拟系统时间 */

/**
 * @brief 模拟获取微秒时间戳（每次调用递增 100ms）。
 */
static uint32_t mock_get_timestamp_us(void)
{
    simulated_time_us += 100000U; /* 假设固定 100ms 调用周期 */
    return simulated_time_us;
}

/**
 * @brief 模拟输出执行器（DAC / PWM）。
 */
static void mock_set_output(pid_real_t value)
{
    /* 真实环境：将 value 写入硬件寄存器 */
    (void) value;
}

/**
 * @brief 全局 HAL 实例，所有控制器可共享。
 */
static const pid_hal_t shared_hal = {.get_timestamp_us = mock_get_timestamp_us, .set_output = mock_set_output};

/*===========================================================================*/
/* 2. 模拟过程变量获取（温度、流量等）                                        */
/*===========================================================================*/

/**
 * @brief 模拟温度传感器（外环被控对象）。
 */
static pid_real_t read_temperature(void)
{
    /* 真实环境：ADC 采集并转换为物理量。此处返回固定值进行演示。 */
    return 25.0f;
}

/**
 * @brief 模拟流量/速度传感器（内环被控对象，用于串级）。
 */
static pid_real_t read_flow(void)
{
    return 10.0f;
}

/*===========================================================================*/
/* 3. 各控制器静态实例及配置                                                 */
/*===========================================================================*/

/* 3.1 位置式 PID 控制器（用于加热器） ------------------------------------- */
static pid_pos_inst_t   heater_inst;
static pid_controller_t heater_ctrl;

/* 3.2 增量式 PID 控制器（用于阀门执行器） --------------------------------- */
static pid_inc_inst_t   valve_inst;
static pid_controller_t valve_ctrl;

/* 3.3 串级 PID 控制器（外环温度，内环流量） ------------------------------- */
static pid_cascade_inst_t cascade_inst;
static pid_controller_t   cascade_ctrl;

/* 3.4 自适应 PID 控制器（基础为位置式，带增益调度） ----------------------- */
static pid_pos_inst_t      adaptive_base_inst; /* 基础位置式控制器 */
static pid_controller_t    adaptive_base_ctrl; /* 基础控制器句柄 */
static pid_adaptive_inst_t adaptive_inst;      /* 自适应包装器 */
static pid_controller_t    adaptive_ctrl;      /* 对外统一句柄 */

/* 自适应调度变量：本次误差的绝对值 */
static pid_real_t adaptive_sched_var = 0.0f;

/* 增益调度表（按误差绝对值区间） */
static const pid_gain_schedule_entry_t adaptive_schedule[] = {
    {.threshold = 2.0f,  .params = {2.0f, 0.05f, 0.1f, 100U}}, /* 小误差：温和参数 */
    {.threshold = 10.0f, .params = {5.0f, 0.2f, 0.3f, 100U} }, /* 中误差：较强响应 */
    {.threshold = 50.0f, .params = {10.0f, 0.5f, 1.0f, 100U}}  /* 大误差：强烈响应 */
};

/*===========================================================================*/
/* 4. 初始化所有控制器                                                        */
/*===========================================================================*/

/**
 * @brief 初始化位置式加热器 PID。
 */
static void init_heater_pid(void)
{
    pid_pos_config_t cfg;
    cfg.params.kp             = 3.0f;
    cfg.params.ki             = 0.1f;
    cfg.params.kd             = 0.5f;
    cfg.params.sample_time_ms = 100U;
    pid_antiwindup_init(&cfg.antiwindup, 0.0f, 100.0f);
    pid_filter_init(&cfg.deriv_filter, 0.1f);
    pid_deadband_init(&cfg.deadband, 0.2f);
    pid_ramp_init(&cfg.setpoint_ramp, 2.0f);
    pid_rate_limit_init(&cfg.output_rate_limit, 20.0f);
    cfg.manual_mode   = false;
    cfg.manual_output = 0.0f;
    cfg.hal           = &shared_hal;

    if (PID_Positional_Init(&heater_ctrl, &heater_inst, &cfg) != PID_OK)
    {
        printf("Heater PID init failed!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 初始化增量式阀门 PID。
 */
static void init_valve_pid(void)
{
    pid_inc_config_t cfg;
    cfg.params.kp             = 1.5f;
    cfg.params.ki             = 0.2f;
    cfg.params.kd             = 0.0f; /* 阀门通常不需要微分 */
    cfg.params.sample_time_ms = 100U;
    pid_integral_sep_init(&cfg.integral_sep, 5.0f);
    pid_rate_limit_init(&cfg.output_rate_limit, 10.0f);
    cfg.output_low    = 0.0f;
    cfg.output_high   = 100.0f;
    cfg.manual_mode   = false;
    cfg.manual_output = 0.0f;
    cfg.hal           = &shared_hal;

    if (PID_Incremental_Init(&valve_ctrl, &valve_inst, &cfg) != PID_OK)
    {
        printf("Valve PID init failed!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 初始化串级 PID（外环加热器位置式 + 内环阀门增量式）。
 */
static void init_cascade_pid(void)
{
    /* 注意：串级的内外环使用已经初始化好的 heater_ctrl 和 valve_ctrl，
       但为避免与单独使用的控制器冲突，此处可复制配置新建实例。
       为简化，本示例直接重用 heater_ctrl 和 valve_ctrl，但在实际应用中
       应为串级单独创建独立实例。此处仅演示框架。 */
    if (PID_Cascade_Init(&cascade_ctrl, &cascade_inst, &heater_ctrl, /* 外环 */
                         &valve_ctrl,                                /* 内环 */
                         0.0f,                                       /* 内环设定值下限 */
                         80.0f)                                      /* 内环设定值上限 */
        != PID_OK)
    {
        printf("Cascade PID init failed!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 初始化自适应 PID（基础位置式 + 增益调度）。
 */
static void init_adaptive_pid(void)
{
    /* 先初始化基础位置式控制器（参数将被调度表覆盖，此处可给任意有效值） */
    pid_pos_config_t base_cfg;
    base_cfg.params.kp             = 1.0f;
    base_cfg.params.ki             = 0.0f;
    base_cfg.params.kd             = 0.0f;
    base_cfg.params.sample_time_ms = 100U;
    pid_antiwindup_init(&base_cfg.antiwindup, 0.0f, 100.0f);
    pid_filter_init(&base_cfg.deriv_filter, 0.1f);
    pid_deadband_init(&base_cfg.deadband, 0.1f);
    pid_ramp_init(&base_cfg.setpoint_ramp, 1.0f);
    pid_rate_limit_init(&base_cfg.output_rate_limit, 30.0f);
    base_cfg.manual_mode   = false;
    base_cfg.manual_output = 0.0f;
    base_cfg.hal           = &shared_hal;

    if (PID_Positional_Init(&adaptive_base_ctrl, &adaptive_base_inst, &base_cfg) != PID_OK)
    {
        printf("Adaptive base PID init failed!\n");
        exit(EXIT_FAILURE);
    }

    /* 再初始化自适应包装器 */
    if (PID_Adaptive_Init(&adaptive_ctrl, &adaptive_inst, &adaptive_base_ctrl, adaptive_schedule,
                          sizeof(adaptive_schedule) / sizeof(adaptive_schedule[0]),
                          &adaptive_sched_var) /* 调度变量指针 */
        != PID_OK)
    {
        printf("Adaptive PID init failed!\n");
        exit(EXIT_FAILURE);
    }
}

/*===========================================================================*/
/* 5. 演示循环（每个控制器以不同策略运行）                                    */
/*===========================================================================*/

static void demo_loop(void)
{
    pid_real_t  temp_sp = 80.0f; /* 温度设定 */
    pid_real_t  flow_sp = 50.0f; /* 流量设定（用于串级外环设定） */
    pid_real_t  temperature;
    pid_real_t  flow;
    pid_real_t  output;
    pid_error_t err;

    printf("\n===== PID Multi-Controller Demo Start =====\n");

    for (int cycle = 0; cycle < 500; cycle++)
    {
        /* 模拟过程变量 */
        temperature = read_temperature();
        flow        = read_flow();

        /* ---- 1. 位置式加热器 PID ---- */
        /* 在某个时刻切换到手动模式并恢复 */
        if (cycle == 100)
        {
            heater_inst.manual_mode   = true;
            heater_inst.manual_output = 40.0f;
            printf("[Cycle %d] Heater -> Manual (40%%)\n", cycle);
        }
        else if (cycle == 200)
        {
            heater_inst.manual_mode = false; /* 自动切回，内部无扰 */
            printf("[Cycle %d] Heater -> Auto\n", cycle);
        }

        err = heater_ctrl.compute(&heater_ctrl, temp_sp, temperature, &output);
        if (err != PID_OK)
        {
            printf("Heater PID error: %d\n", (int) err);
        }
        else
        {
            printf("Heater output: %6.2f%%\n", (double) output);
        }

        /* ---- 2. 增量式阀门 PID ---- */
        if (cycle == 150)
        {
            valve_inst.manual_mode   = true;
            valve_inst.manual_output = 60.0f;
            printf("[Cycle %d] Valve -> Manual (60%%)\n", cycle);
        }
        else if (cycle == 250)
        {
            valve_inst.manual_mode = false;
            printf("[Cycle %d] Valve -> Auto\n", cycle);
        }

        err = valve_ctrl.compute(&valve_ctrl, 40.0f, flow, &output);
        if (err != PID_OK)
        {
            printf("Valve PID error: %d\n", (int) err);
        }
        else
        {
            printf("Valve output : %6.2f%%\n", (double) output);
        }

        /* ---- 3. 串级 PID ---- */
        /* 注：本串级示例中外环测量用温度，内环测量用流量，但 cascade_compute
           简化了内环测量值的传入。在实际代码中需扩展接口，此处仅演示调用形式。 */
        err = cascade_ctrl.compute(&cascade_ctrl, flow_sp, temperature, &output);
        if (err != PID_OK)
        {
            printf("Cascade PID error: %d\n", (int) err);
        }
        else
        {
            printf("Cascade out : %6.2f%%\n", (double) output);
        }

        /* ---- 4. 自适应 PID ---- */
        /* 调度变量设为当前温度误差绝对值 */
        adaptive_sched_var = (pid_real_t) fabs((double) (temp_sp - temperature));
        err                = adaptive_ctrl.compute(&adaptive_ctrl, temp_sp, temperature, &output);
        if (err != PID_OK)
        {
            printf("Adaptive PID error: %d\n", (int) err);
        }
        else
        {
            printf("Adaptive out: %6.2f%%  (sched_var=%.2f)\n", (double) output, (double) adaptive_sched_var);
        }

        printf("--------------------------------------------\n");

        /* 模拟适当延迟（实际由硬件定时器触发，此处无操作） */
    }

    printf("===== Demo Finished =====\n");
}

/*===========================================================================*/
/* 6. 主函数入口                                                             */
/*===========================================================================*/

int main(void)
{
    printf("Initializing all PID controllers...\n");

    init_heater_pid();
    init_valve_pid();
    init_cascade_pid();
    init_adaptive_pid();

    printf("All controllers initialized successfully.\n");

    demo_loop();

    return 0;
}
