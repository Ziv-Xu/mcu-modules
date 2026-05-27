#include "pid_incremental.h"

static pid_error_t inc_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *out);
static void        inc_reset(pid_controller_t *ctrl);
static pid_error_t inc_set_params(pid_controller_t *ctrl, const pid_params_t *params);
static pid_error_t inc_get_params(pid_controller_t *ctrl, pid_params_t *params);

pid_error_t PID_Incremental_Init(pid_controller_t *ctrl, pid_inc_inst_t *inst, const pid_inc_config_t *config)
{
    if ((ctrl == NULL) || (inst == NULL) || (config == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    if ((config->params.sample_time_ms == 0U) || (config->params.kp < (pid_real_t) 0)
        || (config->params.ki < (pid_real_t) 0) || (config->params.kd < (pid_real_t) 0))
    {
        return PID_ERR_PARAM;
    }

    inst->params            = config->params;
    inst->integral_sep      = config->integral_sep;
    inst->output_rate_limit = config->output_rate_limit;
    inst->output_low        = config->output_low;
    inst->output_high       = config->output_high;
    inst->manual_output     = config->manual_output;
    inst->manual_mode       = config->manual_mode;
    inst->last_manual       = config->manual_mode;
    inst->hal               = config->hal;

    pid_core_inc_init(&inst->core);
    inst->cumulative_output = config->manual_output; /* 初始输出 */

    if ((inst->hal != NULL) && (inst->hal->get_timestamp_us != NULL))
    {
        inst->last_time_us = inst->hal->get_timestamp_us();
    }
    else
    {
        inst->last_time_us = 0U;
    }

    ctrl->type       = PID_TYPE_INCREMENTAL;
    ctrl->impl       = inst;
    ctrl->compute    = inc_compute;
    ctrl->reset      = inc_reset;
    ctrl->set_params = inc_set_params;
    ctrl->get_params = inc_get_params;
    return PID_OK;
}

static pid_error_t inc_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *output)
{
    if ((ctrl == NULL) || (output == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_inc_inst_t *inst = (pid_inc_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }

    pid_real_t dt;
    if ((inst->hal != NULL) && (inst->hal->get_timestamp_us != NULL))
    {
        uint32_t now       = inst->hal->get_timestamp_us();
        dt                 = (pid_real_t) (now - inst->last_time_us) * 1.0e-6f;
        inst->last_time_us = now;
    }
    else
    {
        dt = (pid_real_t) inst->params.sample_time_ms * 1.0e-3f;
    }
    if (dt <= (pid_real_t) 0)
    {
        return PID_ERR_SAMPLE_TIME;
    }

    pid_real_t error = sp - pv;

    /* 手动模式：输出跟踪手动值，更新累计输出和历史 */
    if (inst->manual_mode)
    {
        inst->cumulative_output = inst->manual_output;
        pid_rate_limit_force(&inst->output_rate_limit, inst->manual_output);
        inst->core.last_error = error;
        inst->core.prev_error = error; /* 避免下次微分阶跃 */
        *output               = inst->manual_output;
        inst->last_manual     = true;
        if ((inst->hal != NULL) && (inst->hal->set_output != NULL))
        {
            inst->hal->set_output(*output);
        }
        return PID_OK;
    }

    if (inst->last_manual)
    {
        /* 切换瞬间，维持累积输出，误差历史同步 */
        inst->core.last_error = error;
        inst->core.prev_error = error;
        inst->last_manual     = false;
    }

    /* 积分分离：决定是否使用积分项 */
    pid_real_t ki_active = pid_integral_sep_is_active(&inst->integral_sep, error) ? inst->params.ki : (pid_real_t) 0;

    pid_real_t delta;
    pid_core_inc_compute(&inst->core, error, dt, inst->params.kp, ki_active, inst->params.kd, &delta);

    /* 累积绝对输出并限幅 */
    inst->cumulative_output += delta;
    if (inst->cumulative_output > inst->output_high)
    {
        inst->cumulative_output = inst->output_high;
    }
    else if (inst->cumulative_output < inst->output_low)
    {
        inst->cumulative_output = inst->output_low;
    }
    else
    { /* 空 */
    }

    /* 输出变化率限制 */
    *output = pid_rate_limit_apply(&inst->output_rate_limit, inst->cumulative_output, dt);

    /* 同步累积输出，防止率限制偏差积累 */
    inst->cumulative_output = *output;

    if ((inst->hal != NULL) && (inst->hal->set_output != NULL))
    {
        inst->hal->set_output(*output);
    }
    return PID_OK;
}

static void inc_reset(pid_controller_t *ctrl)
{
    if ((ctrl != NULL) && (ctrl->impl != NULL))
    {
        pid_inc_inst_t *inst = (pid_inc_inst_t *) ctrl->impl;
        pid_core_inc_init(&inst->core);
        inst->cumulative_output = (pid_real_t) 0;
        pid_rate_limit_force(&inst->output_rate_limit, (pid_real_t) 0);
        inst->last_manual = inst->manual_mode;
    }
}

static pid_error_t inc_set_params(pid_controller_t *ctrl, const pid_params_t *params)
{
    if ((ctrl == NULL) || (params == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_inc_inst_t *inst = (pid_inc_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    PID_ENTER_CRITICAL();
    inst->params = *params;
    PID_EXIT_CRITICAL();
    return PID_OK;
}

static pid_error_t inc_get_params(pid_controller_t *ctrl, pid_params_t *params)
{
    if ((ctrl == NULL) || (params == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_inc_inst_t *inst = (pid_inc_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    *params = inst->params;
    return PID_OK;
}

pid_error_t PID_Incremental_GetConfig(pid_controller_t *ctrl, pid_inc_config_t *config)
{
    if ((ctrl == NULL) || (config == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_inc_inst_t *inst = (pid_inc_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }
    config->params            = inst->params;
    config->integral_sep      = inst->integral_sep;
    config->output_rate_limit = inst->output_rate_limit;
    config->output_low        = inst->output_low;
    config->output_high       = inst->output_high;
    config->manual_output     = inst->manual_output;
    config->manual_mode       = inst->manual_mode;
    config->hal               = inst->hal;
    return PID_OK;
}
