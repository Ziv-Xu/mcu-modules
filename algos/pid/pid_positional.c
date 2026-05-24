/**
 * @file    pid_positional.c
 * @brief   位置式 PID 控制器实现。
 */

#include "pid_positional.h"

/* 前向声明 */
static pid_error_t pos_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *out);
static void        pos_reset(pid_controller_t *ctrl);
static pid_error_t pos_set_params(pid_controller_t *ctrl, const pid_params_t *params);
static pid_error_t pos_get_params(pid_controller_t *ctrl, pid_params_t *params);

/*---------------------------------------------------------------------------*/
pid_error_t PID_Positional_Init(pid_controller_t *ctrl, pid_pos_inst_t *inst, const pid_pos_config_t *config)
{
    if ((ctrl == NULL) || (inst == NULL) || (config == NULL))
    {
        return PID_ERR_NULL_PTR;
    }

    /* 参数检查 */
    if ((config->params.sample_time_ms == 0U) || (config->params.kp < (pid_real_t) 0)
        || (config->params.ki < (pid_real_t) 0) || (config->params.kd < (pid_real_t) 0))
    {
        return PID_ERR_PARAM;
    }

    /* 复制配置 */
    inst->params            = config->params;
    inst->antiwindup        = config->antiwindup;
    inst->deriv_filter      = config->deriv_filter;
    inst->deadband          = config->deadband;
    inst->setpoint_ramp     = config->setpoint_ramp;
    inst->output_rate_limit = config->output_rate_limit;
    inst->manual_output     = config->manual_output;
    inst->manual_mode       = config->manual_mode;
    inst->last_manual       = config->manual_mode;
    inst->hal               = config->hal;

    /* 初始化核心 */
    pid_core_pos_init(&inst->core);

    /* 跟踪手动输出：若初始为手动，将积分和斜坡设到手动值 */
    if (inst->manual_mode)
    {
        pid_ramp_force(&inst->setpoint_ramp, (pid_real_t) 0); /* 由首次 compute 处理 */
        inst->core.integral = inst->manual_output;
    }

    /* 记录起始时间 */
    if ((inst->hal != NULL) && (inst->hal->get_timestamp_us != NULL))
    {
        inst->last_time_us = inst->hal->get_timestamp_us();
    }
    else
    {
        inst->last_time_us = 0U;
    }

    /* 填充虚函数表 */
    ctrl->type       = PID_TYPE_POSITIONAL;
    ctrl->impl       = inst;
    ctrl->compute    = pos_compute;
    ctrl->reset      = pos_reset;
    ctrl->set_params = pos_set_params;
    ctrl->get_params = pos_get_params;

    return PID_OK;
}

/*---------------------------------------------------------------------------*/
static pid_error_t pos_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *output)
{
    if ((ctrl == NULL) || (output == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_pos_inst_t *inst = (pid_pos_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }

    pid_real_t dt;
    pid_real_t error;
    pid_real_t p_term, d_term, raw_output;
    pid_real_t filtered_d;

    /*-------------- 获取时间戳并计算 dt --------------*/
    if ((inst->hal != NULL) && (inst->hal->get_timestamp_us != NULL))
    {
        uint32_t now       = inst->hal->get_timestamp_us();
        dt                 = (pid_real_t) (now - inst->last_time_us) * 1.0e-6f; /* us -> s */
        inst->last_time_us = now;
    }
    else
    {
        /* 无 HAL 时视为固定采样周期 */
        dt = (pid_real_t) inst->params.sample_time_ms * 1.0e-3f;
    }

    /* 采样时间异常保护 */
    if (dt <= (pid_real_t) 0)
    {
        return PID_ERR_SAMPLE_TIME;
    }

    /*-------------- 手动/自动无扰切换 --------------*/
    if (inst->manual_mode)
    {
        /* 手动模式：直接输出手动值，并跟踪斜坡和积分 */
        pid_ramp_force(&inst->setpoint_ramp, pv); /* 斜坡跟踪 PV */
        inst->core.last_error = (pid_real_t) 0;
        inst->core.integral   = inst->manual_output; /* 积分跟踪输出 */
        pid_rate_limit_force(&inst->output_rate_limit, inst->manual_output);
        *output           = inst->manual_output;
        inst->last_manual = true;

        /* 调用 HAL 输出（如果存在） */
        if ((inst->hal != NULL) && (inst->hal->set_output != NULL))
        {
            inst->hal->set_output(*output);
        }
        return PID_OK;
    }

    /* 从手动切回自动的瞬间，无扰初始化 */
    if (inst->last_manual)
    {
        pid_ramp_force(&inst->setpoint_ramp, pv); /* 斜坡起始为当前 PV */
        inst->core.last_error = (pid_real_t) 0;
        /* 积分值保持为上一次手动输出，P 和 D 将在此基础上叠加，输出无跳变 */
        pid_rate_limit_force(&inst->output_rate_limit, inst->manual_output);
        inst->last_manual = false;
    }

    /*-------------- 设定值斜坡 --------------*/
    pid_real_t effective_sp = pid_ramp_update(&inst->setpoint_ramp, sp, dt);

    /*-------------- 误差计算与死区 --------------*/
    error = pid_deadband_apply(&inst->deadband, effective_sp - pv);

    /*-------------- 计算 P 和 D（分离出来用于抗饱和反算） --------------*/
    p_term = inst->params.kp * error;

    /* 微分项（先计算差分，再滤波） */
    if (dt > (pid_real_t) 0)
    {
        d_term = inst->params.kd * (error - inst->core.last_error) / dt;
    }
    else
    {
        d_term = (pid_real_t) 0;
    }
    filtered_d = pid_filter_apply(&inst->deriv_filter, d_term);

    /* 记录本次误差（在积分计算前，因为积分可能被修改） */
    inst->core.last_error = error;

    /*-------------- 核心积分与原始输出 --------------*/
    /* 手动计算积分（不使用核心函数，以便抗饱和） */
    if (inst->params.ki != (pid_real_t) 0)
    {
        inst->core.integral += inst->params.ki * error * dt;
    }

    raw_output = p_term + inst->core.integral + filtered_d;

    /*-------------- 抗饱和钳位 --------------*/
    pid_antiwindup_clamp(&inst->antiwindup, &raw_output, p_term, filtered_d, &inst->core.integral);

    /*-------------- 输出变化率限制 --------------*/
    *output = pid_rate_limit_apply(&inst->output_rate_limit, raw_output, dt);

    /*-------------- HAL 输出 --------------*/
    if ((inst->hal != NULL) && (inst->hal->set_output != NULL))
    {
        inst->hal->set_output(*output);
    }

    return PID_OK;
}

/*---------------------------------------------------------------------------*/
static void pos_reset(pid_controller_t *ctrl)
{
    if ((ctrl != NULL) && (ctrl->impl != NULL))
    {
        pid_pos_inst_t *inst = (pid_pos_inst_t *) ctrl->impl;
        pid_core_pos_init(&inst->core);
        pid_ramp_force(&inst->setpoint_ramp, (pid_real_t) 0);
        pid_rate_limit_force(&inst->output_rate_limit, (pid_real_t) 0);
        inst->last_manual = inst->manual_mode;
    }
}

/*---------------------------------------------------------------------------*/
static pid_error_t pos_set_params(pid_controller_t *ctrl, const pid_params_t *params)
{
    if ((ctrl == NULL) || (params == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_pos_inst_t *inst = (pid_pos_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    /* 运行时可安全更新 Kp, Ki, Kd 和采样时间 */
    PID_ENTER_CRITICAL();
    inst->params = *params;
    PID_EXIT_CRITICAL();
    return PID_OK;
}

/*---------------------------------------------------------------------------*/
static pid_error_t pos_get_params(pid_controller_t *ctrl, pid_params_t *params)
{
    if ((ctrl == NULL) || (params == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_pos_inst_t *inst = (pid_pos_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    *params = inst->params;
    return PID_OK;
}

/*---------------------------------------------------------------------------*/
pid_error_t PID_Positional_GetConfig(pid_controller_t *ctrl, pid_pos_config_t *config)
{
    if ((ctrl == NULL) || (config == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_pos_inst_t *inst = (pid_pos_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    config->params            = inst->params;
    config->antiwindup        = inst->antiwindup;
    config->deriv_filter      = inst->deriv_filter;
    config->deadband          = inst->deadband;
    config->setpoint_ramp     = inst->setpoint_ramp;
    config->output_rate_limit = inst->output_rate_limit;
    config->manual_output     = inst->manual_output;
    config->manual_mode       = inst->manual_mode;
    config->hal               = inst->hal;
    return PID_OK;
}
