/**
 * @file    pid_core.h
 * @brief   最小化的位置式与增量式 PID 核心状态与计算。
 * @details 核心仅包含比例、积分、微分基本运算，不涉及任何扩展。
 *          积分项对外暴露，便于抗饱和等模块直接修正。
 */

#ifndef PID_CORE_H
#define PID_CORE_H

#include "pid_template.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*---------------------------------------------------------------------------*/
    /* 位置式 PID 核心 */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief 位置式 PID 核心状态。
     */
    typedef struct
    {
        pid_real_t last_error; /**< 上一次误差 e(k-1) */
        pid_real_t integral;   /**< 积分累加项（外部可修正以抗饱和） */
    } pid_core_pos_t;

    /**
     * @brief 初始化位置式核心。
     * @param core 核心状态指针。
     */
    PID_INLINE void pid_core_pos_init(pid_core_pos_t *core)
    {
        if (core != NULL)
        {
            core->last_error = (pid_real_t) 0;
            core->integral   = (pid_real_t) 0;
        }
    }

    /**
     * @brief 执行一次位置式核心计算（无扩展）。
     * @param core   核心状态。
     * @param error  当前误差 e(k)。
     * @param dt     采样间隔（秒）。
     * @param kp     比例增益。
     * @param ki     积分增益。
     * @param kd     微分增益。
     * @param output 输出原始 PID 值（未限幅）。
     * @note  积分项累加后直接存入 core->integral，微分采用后向差分。
     */
    PID_INLINE void pid_core_pos_compute(pid_core_pos_t *core, pid_real_t error, pid_real_t dt, pid_real_t kp,
                                         pid_real_t ki, pid_real_t kd, pid_real_t *output)
    {
        pid_real_t p_term, d_term;

        /* 比例项 */
        p_term = kp * error;

        /* 积分项（梯形积分，简单矩形亦可） */
        if (ki != (pid_real_t) 0)
        {
            core->integral += ki * error * dt;
        }

        /* 微分项（后向差分，带零除保护） */
        if (dt > (pid_real_t) 0)
        {
            d_term = kd * (error - core->last_error) / dt;
        }
        else
        {
            d_term = (pid_real_t) 0;
        }

        /* 记录本次误差 */
        core->last_error = error;

        /* 原始输出 = P + I + D */
        *output = p_term + core->integral + d_term;
    }

    /*---------------------------------------------------------------------------*/
    /* 增量式 PID 核心 */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief 增量式 PID 核心状态。
     */
    typedef struct
    {
        pid_real_t last_error; /**< e(k-1) */
        pid_real_t prev_error; /**< e(k-2) */
    } pid_core_inc_t;

    /**
     * @brief 初始化增量式核心。
     * @param core 核心状态指针。
     */
    PID_INLINE void pid_core_inc_init(pid_core_inc_t *core)
    {
        if (core != NULL)
        {
            core->last_error = (pid_real_t) 0;
            core->prev_error = (pid_real_t) 0;
        }
    }

    /**
     * @brief 执行一次增量式核心计算。
     * @param core   核心状态。
     * @param error  当前误差 e(k)。
     * @param dt     采样间隔（秒）。
     * @param kp     比例增益。
     * @param ki     积分增益（使用当前误差积分，非增量积分）。
     * @param kd     微分增益。
     * @param delta  输出的增量 Δu。
     */
    PID_INLINE void pid_core_inc_compute(pid_core_inc_t *core, pid_real_t error, pid_real_t dt, pid_real_t kp,
                                         pid_real_t ki, pid_real_t kd, pid_real_t *delta)
    {
        pid_real_t p_delta, i_delta, d_delta;

        /* 比例增量 */
        p_delta = kp * (error - core->last_error);

        /* 积分增量（使用当前误差） */
        i_delta = ki * error * dt;

        /* 微分增量（二阶差分） */
        if (dt > (pid_real_t) 0)
        {
            d_delta = (kd / dt) * (error - ((pid_real_t) 2 * core->last_error) + core->prev_error);
        }
        else
        {
            d_delta = (pid_real_t) 0;
        }

        /* 更新历史误差 */
        core->prev_error = core->last_error;
        core->last_error = error;

        *delta = p_delta + i_delta + d_delta;
    }

#ifdef __cplusplus
}
#endif

#endif /* PID_CORE_H */
