/**
 * @file    pid_incremental.h
 * @brief   增量式 PID 控制器，集成积分分离、输出变化率限制。
 */

#ifndef PID_INCREMENTAL_H
#define PID_INCREMENTAL_H

#include "pid_template.h"
#include "pid_core.h"
#include "pid_ext.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        pid_params_t       params;
        pid_integral_sep_t integral_sep;      /**< 积分分离 */
        pid_rate_limit_t   output_rate_limit; /**< 输出变化率限制（作用于绝对输出） */
        pid_real_t         output_low;        /**< 绝对输出下限 */
        pid_real_t         output_high;       /**< 绝对输出上限 */
        pid_real_t         manual_output;
        bool               manual_mode;
        const pid_hal_t   *hal;
    } pid_inc_config_t;

    typedef struct
    {
        pid_core_inc_t     core;
        pid_integral_sep_t integral_sep;
        pid_rate_limit_t   output_rate_limit;
        pid_real_t         output_low;
        pid_real_t         output_high;
        pid_real_t         cumulative_output; /**< 累积绝对输出 */
        pid_real_t         manual_output;
        bool               manual_mode;
        bool               last_manual;
        const pid_hal_t   *hal;
        uint32_t           last_time_us;
        pid_params_t       params;
    } pid_inc_inst_t;

    pid_error_t PID_Incremental_Init(pid_controller_t *ctrl, pid_inc_inst_t *inst, const pid_inc_config_t *config);

    pid_error_t PID_Incremental_GetConfig(pid_controller_t *ctrl, pid_inc_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* PID_INCREMENTAL_H */
