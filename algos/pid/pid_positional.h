/**
 * @file    pid_positional.h
 * @brief   位置式 PID 控制器，组合核心与扩展，提供统一接口。
 */

#ifndef PID_POSITIONAL_H
#define PID_POSITIONAL_H

#include "pid_config.h"
#include "pid_template.h"
#include "pid_core.h"
#include "pid_ext.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*---------------------------------------------------------------------------*/
    /* 位置式 PID 配置与实例结构体 */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        pid_params_t     params;            /**< 基本 PID 参数 */
        pid_antiwindup_t antiwindup;        /**< 抗饱和配置 */
        pid_filter_t     deriv_filter;      /**< 微分滤波器 */
        pid_deadband_t   deadband;          /**< 误差死区 */
        pid_ramp_t       setpoint_ramp;     /**< 设定值斜坡 */
        pid_rate_limit_t output_rate_limit; /**< 输出变化率限制 */
        pid_real_t       manual_output;     /**< 手动模式下的输出值 */
        bool             manual_mode;       /**< true=手动，false=自动 */
        const pid_hal_t *hal;               /**< 硬件抽象层（时间戳、输出） */
    } pid_pos_config_t;

    typedef struct
    {
        pid_core_pos_t   core; /**< 位置式核心 */
        pid_antiwindup_t antiwindup;
        pid_filter_t     deriv_filter;
        pid_deadband_t   deadband;
        pid_ramp_t       setpoint_ramp;
        pid_rate_limit_t output_rate_limit;
        pid_real_t       manual_output;
        bool             manual_mode;
        bool             last_manual; /**< 上次手动标志，检测切换沿 */
        const pid_hal_t *hal;
        uint32_t         last_time_us; /**< 上一次调用时间戳(us) */
        pid_params_t     params;       /**< 运行时参数（可在线修改） */
    } pid_pos_inst_t;

    /*---------------------------------------------------------------------------*/
    /* 接口函数 */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief 初始化位置式 PID 控制器并绑定到统一句柄。
     * @param ctrl   统一控制器句柄（由用户分配）。
     * @param inst   位置式实例（由用户分配）。
     * @param config 初始化配置。
     * @return 错误码。
     */
    pid_error_t PID_Positional_Init(pid_controller_t *ctrl, pid_pos_inst_t *inst, const pid_pos_config_t *config);

    /**
     * @brief 获取位置式控制器的配置（用于持久化）。
     * @param ctrl   控制器句柄。
     * @param config 输出配置结构体。
     * @return 错误码。
     */
    pid_error_t PID_Positional_GetConfig(pid_controller_t *ctrl, pid_pos_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* PID_POSITIONAL_H */
