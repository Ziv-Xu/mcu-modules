/**
 * @file    pid_cascade.h
 * @brief   串级 PID 控制器框架。
 */

#ifndef PID_CASCADE_H
#define PID_CASCADE_H

#include "pid_template.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        pid_controller_t *outer;         /**< 外环控制器（用户已初始化） */
        pid_controller_t *inner;         /**< 内环控制器（用户已初始化） */
        pid_real_t        inner_sp_low;  /**< 内环设定值下限 */
        pid_real_t        inner_sp_high; /**< 内环设定值上限 */
    } pid_cascade_inst_t;

    /**
     * @brief 初始化串级控制器句柄。
     * @param ctrl  统一句柄。
     * @param inst  串级实例。
     * @param outer 已初始化的外环控制器句柄。
     * @param inner 已初始化的内环控制器句柄。
     * @param inner_sp_limits 内环设定值限幅。
     * @return 错误码。
     */
    pid_error_t PID_Cascade_Init(pid_controller_t *ctrl, pid_cascade_inst_t *inst, pid_controller_t *outer,
                                 pid_controller_t *inner, pid_real_t inner_sp_low, pid_real_t inner_sp_high);

#ifdef __cplusplus
}
#endif

#endif /* PID_CASCADE_H */
