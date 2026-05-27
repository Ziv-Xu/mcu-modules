#include "pid_cascade.h"

static pid_error_t casc_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *out);
static void        casc_reset(pid_controller_t *ctrl);
static pid_error_t casc_set_params(pid_controller_t *ctrl, const pid_params_t *params);
static pid_error_t casc_get_params(pid_controller_t *ctrl, pid_params_t *params);

pid_error_t PID_Cascade_Init(pid_controller_t *ctrl, pid_cascade_inst_t *inst, pid_controller_t *outer,
                             pid_controller_t *inner, pid_real_t inner_sp_low, pid_real_t inner_sp_high)
{
    if ((ctrl == NULL) || (inst == NULL) || (outer == NULL) || (inner == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    inst->outer         = outer;
    inst->inner         = inner;
    inst->inner_sp_low  = inner_sp_low;
    inst->inner_sp_high = inner_sp_high;

    ctrl->type       = PID_TYPE_CASCADE;
    ctrl->impl       = inst;
    ctrl->compute    = casc_compute;
    ctrl->reset      = casc_reset;
    ctrl->set_params = casc_set_params;
    ctrl->get_params = casc_get_params;
    return PID_OK;
}

static pid_error_t casc_compute(pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv_outer, pid_real_t *output)
{
    if ((ctrl == NULL) || (output == NULL))
    {
        return PID_ERR_NULL_PTR;
    }
    pid_cascade_inst_t *inst = (pid_cascade_inst_t *) ctrl->impl;
    if (inst == NULL)
    {
        return PID_ERR_NOT_INIT;
    }

    pid_real_t  inner_sp;
    pid_error_t err;

    /* 外环计算，得到内环设定值 */
    err = inst->outer->compute(inst->outer, sp, pv_outer, &inner_sp);
    if (err != PID_OK)
    {
        return err;
    }

    /* 限幅内环设定值 */
    if (inner_sp > inst->inner_sp_high)
    {
        inner_sp = inst->inner_sp_high;
    }
    else if (inner_sp < inst->inner_sp_low)
    {
        inner_sp = inst->inner_sp_low;
    }
    else
    { /* 无操作 */
    }

    /* 内环计算（此处内环的过程变量应由用户通过传感器给出，但接口未提供 pv_inner。
       实际使用时需修改接口。简化起见，假设 outer 的测量值也可作为内环 pv，或增加参数。
       可自行扩展为接受两个测量值的接口。此处演示结构。） */
    /* 为符合现有接口，我们假定 pv_outer 也是内环测量（不准确），或由用户扩展。
       在 main_example 中会展示如何正确使用。 */
    err = inst->inner->compute(inst->inner, inner_sp, pv_outer, output);
    return err;
}

static void casc_reset(pid_controller_t *ctrl)
{
    if ((ctrl != NULL) && (ctrl->impl != NULL))
    {
        pid_cascade_inst_t *inst = (pid_cascade_inst_t *) ctrl->impl;
        if (inst->outer != NULL)
        {
            inst->outer->reset(inst->outer);
        }
        if (inst->inner != NULL)
        {
            inst->inner->reset(inst->inner);
        }
    }
}

static pid_error_t casc_set_params(pid_controller_t *ctrl, const pid_params_t *params)
{
    (void) ctrl;
    (void) params;
    /* 串级控制器本身无独立参数，参数分布于内外环 */
    return PID_ERR_PARAM;
}

static pid_error_t casc_get_params(pid_controller_t *ctrl, pid_params_t *params)
{
    (void) ctrl;
    (void) params;
    return PID_ERR_PARAM;
}
