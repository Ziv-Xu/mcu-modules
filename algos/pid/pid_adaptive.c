#include "pid_adaptive.h"

static pid_error_t adap_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *out);
static void        adap_reset(pid_controller_t *ctrl);
static pid_error_t adap_set_params(pid_controller_t *ctrl, const pid_params_t *params);
static pid_error_t adap_get_params(pid_controller_t *ctrl, pid_params_t *params);

pid_error_t PID_Adaptive_Init(pid_controller_t *ctrl, pid_adaptive_inst_t *inst, pid_controller_t *base_ctrl,
                              const pid_gain_schedule_entry_t *table, uint16_t table_size, pid_real_t *sched_var)
{
    if ((ctrl == NULL) || (inst == NULL) || (base_ctrl == NULL) || (table == NULL) || (sched_var == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    if (table_size == 0U)
    {
        return PID_ERR_PARAM;
    }

    inst->base_ctrl      = base_ctrl;
    inst->schedule_table = table;
    inst->table_size     = table_size;
    inst->schedule_var   = sched_var;

    ctrl->type       = PID_TYPE_ADAPTIVE;
    ctrl->impl       = inst;
    ctrl->compute    = adap_compute;
    ctrl->reset      = adap_reset;
    ctrl->set_params = adap_set_params;
    ctrl->get_params = adap_get_params;
    return PID_OK;
}

/**
 * @brief 查找增益调度表中匹配当前调度变量的参数。
 */
static const pid_params_t *adaptive_lookup(const pid_adaptive_inst_t *inst, pid_real_t var)
{
    uint16_t i;
    for (i = 0U; i < inst->table_size; i++)
    {
        if (var <= inst->schedule_table[i].threshold)
        {
            return &inst->schedule_table[i].params;
        }
    }
    /* 超过最大阈值时使用最后一项参数 */
    return &inst->schedule_table[inst->table_size - 1U].params;
}

static pid_error_t adap_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *out)
{
    if ((ctrl == NULL) || (out == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_adaptive_inst_t *inst = (pid_adaptive_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }

    /* 根据当前调度变量选择参数 */
    const pid_params_t *sel_params = adaptive_lookup(inst, *(inst->schedule_var));

    /* 更新基础控制器的参数 */
    pid_error_t err = inst->base_ctrl->set_params(inst->base_ctrl, sel_params);
    if (err != PID_OK)
    {
        return err;
    }

    /* 调用基础控制器计算 */
    return inst->base_ctrl->compute(inst->base_ctrl, sp, pv, out);
}

static void adap_reset(pid_controller_t *ctrl)
{
    if ((ctrl != NULL) && (ctrl->impl != NULL))
    {
        pid_adaptive_inst_t *inst = (pid_adaptive_inst_t *) ctrl->impl;
        if (inst->base_ctrl != NULL)
        {
            inst->base_ctrl->reset(inst->base_ctrl);
        }
    }
}

static pid_error_t adap_set_params(pid_controller_t *ctrl, const pid_params_t *params)
{
    (void) ctrl;
    (void) params;
    return PID_ERR_PARAM; /* 参数通过调度表管理 */
}

static pid_error_t adap_get_params(pid_controller_t *ctrl, pid_params_t *params)
{
    (void) ctrl;
    (void) params;
    return PID_ERR_PARAM;
}
