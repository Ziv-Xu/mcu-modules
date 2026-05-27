/**
 * @file    pid_cascade.h
 * @brief   串级 PID 控制器框架。
 * @note    串级pid控制使用前可以先进行分析，看一看对于需要调节的量如何进行分解
 *          比如最经典的是三级串级，广泛用于高性能伺服控制位置环（最外层）→ 速度环 → 电流环（最内层）
 *          每一层都测量一个物理量：编码器（位置）、测速机或编码器差分（速度）、电流传感器（电流）
 * @note    又比如电机控制定速度，这个就只有一个层，使用单环就可以比较好的控制，但是如果套用串级pid，可能会更加的平滑
 */

#ifndef PID_CASCADE_H
#define PID_CASCADE_H

#include "pid_config.h"
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
