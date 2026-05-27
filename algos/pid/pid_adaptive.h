/**
 * @file    pid_adaptive.h
 * @brief   自适应 PID 基础框架（基于误差分段的增益调度查表法）。
 */

#ifndef PID_ADAPTIVE_H
#define PID_ADAPTIVE_H

#include "pid_config.h"
#include "pid_template.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 增益调度表项。
     */
    typedef struct
    {
        pid_real_t   threshold; /**< 调度变量的上限（误差绝对值等） */
        pid_params_t params;    /**< 该区间对应的 PID 参数 */
    } pid_gain_schedule_entry_t;

    /**
     * @brief 自适应控制器实例。
     */
    typedef struct
    {
        pid_controller_t                *base_ctrl;      /**< 底层控制器（通常为位置式） */
        const pid_gain_schedule_entry_t *schedule_table; /**< 增益调度表（升序排列） */
        uint16_t                         table_size;     /**< 表项数量 */
        pid_real_t                      *schedule_var;   /**< 指向调度变量的指针（如误差绝对值） */
    } pid_adaptive_inst_t;

    /**
     * @brief 初始化自适应控制器。
     * @param ctrl       统一句柄。
     * @param inst       自适应实例。
     * @param base_ctrl  已初始化的底层基础控制器。
     * @param table      增益调度表（用户静态定义）。
     * @param table_size 表大小。
     * @param sched_var  调度变量的指针（例如 &error_abs）。
     * @return 错误码。
     */
    pid_error_t PID_Adaptive_Init(pid_controller_t *ctrl, pid_adaptive_inst_t *inst, pid_controller_t *base_ctrl,
                                  const pid_gain_schedule_entry_t *table, uint16_t table_size, pid_real_t *sched_var);

#ifdef __cplusplus
}
#endif

#endif /* PID_ADAPTIVE_H */
