/**
 * @file    main_example.c
 * @brief   演示如何集成位置式 PID 到温控循环，注册 HAL，手动/自动切换。
 */

#include "pid_positional.h"
#include <stdio.h> /* 仅用于演示输出 */

/*---------------------------------------------------------------------------*/
/* 用户实现的硬件抽象层 */
/*---------------------------------------------------------------------------*/
static uint32_t mock_timestamp_us = 0;

uint32_t my_get_timestamp_us(void)
{
    /* 真实环境应读取硬件定时器，此处模拟递增 */
    mock_timestamp_us += 100000U; /* 假设周期 100ms */
    return mock_timestamp_us;
}

void my_set_output(pid_real_t value)
{
    /* 真实环境将 value 写入 DAC 或 PWM */
    printf("Output set to: %.2f\n", (double) value);
}

static const pid_hal_t my_hal = {.get_timestamp_us = my_get_timestamp_us, .set_output = my_set_output};

/*---------------------------------------------------------------------------*/
/* 模拟温度传感器读取 */
/*---------------------------------------------------------------------------*/
static pid_real_t read_temperature(void)
{
    /* 真实环境读取 ADC，此处返回恒定值模拟 */
    return 25.0f;
}

/*---------------------------------------------------------------------------*/
int main(void)
{
    /* 1. 分配控制器实例内存（静态） */
    pid_pos_inst_t   pos_inst;
    pid_controller_t pos_ctrl;
    pid_pos_config_t pos_cfg;

    /* 2. 填写配置 */
    pos_cfg.params.kp             = 2.0f;
    pos_cfg.params.ki             = 0.1f;
    pos_cfg.params.kd             = 0.5f;
    pos_cfg.params.sample_time_ms = 100U;
    pid_antiwindup_init(&pos_cfg.antiwindup, 0.0f, 100.0f); /* 输出 0~100% */
    pid_filter_init(&pos_cfg.deriv_filter, 0.1f);           /* 滤波系数 */
    pid_deadband_init(&pos_cfg.deadband, 0.2f);             /* 死区 */
    pid_ramp_init(&pos_cfg.setpoint_ramp, 1.0f);            /* 设定值每秒变化不超过1度 */
    pid_rate_limit_init(&pos_cfg.output_rate_limit, 50.0f); /* 输出每秒变化不超过50% */
    pos_cfg.manual_mode   = false;
    pos_cfg.manual_output = 0.0f;
    pos_cfg.hal           = &my_hal;

    /* 3. 初始化控制器 */
    if (PID_Positional_Init(&pos_ctrl, &pos_inst, &pos_cfg) != PID_OK)
    {
        /* 错误处理 */
        return -1;
    }

    /* 4. 主控制循环 */
    pid_real_t  setpoint = 80.0f;
    pid_real_t  temperature;
    pid_real_t  output;
    pid_error_t err;

    for (int i = 0; i < 1000; i++)
    {
        temperature = read_temperature();

        /* 演示手动/自动切换：在第200次循环时切到手动，输出固定50% */
        if (i == 200)
        {
            pos_inst.manual_mode   = true;
            pos_inst.manual_output = 50.0f;
        }
        else if (i == 400)
        {
            /* 切回自动，无需额外操作，控制器内部完成无扰切换 */
            pos_inst.manual_mode = false;
        }

        err = pos_ctrl.compute(&pos_ctrl, setpoint, temperature, &output);
        if (err != PID_OK)
        {
            /* 错误处理 */
        }

        /* 在这里可以将 output 传递给执行器，但 HAL 内已调用 set_output */
        (void) output;
    }

    return 0;
}
