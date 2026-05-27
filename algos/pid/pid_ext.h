/**
 * @file    pid_ext.h
 * @brief   PID 扩展功能组件：抗饱和、微分滤波、死区、设定值斜坡、输出变化率限制、积分分离判断。
 * @details 所有扩展均为独立结构体与函数，可自由组合到具体控制器中。
 */

#ifndef PID_EXT_H
#define PID_EXT_H

#include "pid_config.h"
#include "pid_template.h"
#include "pid_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*---------------------------------------------------------------------------*/
    /* 抗饱和（反计算法） */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;      /**< 启用标志 */
        pid_real_t output_high; /**< 输出上限 */
        pid_real_t output_low;  /**< 输出下限 */
    } pid_antiwindup_t;

    PID_INLINE void pid_antiwindup_init(pid_antiwindup_t *aw, pid_real_t low, pid_real_t high)
    {
        if (aw != NULL)
        {
            aw->enable      = true;
            aw->output_low  = low;
            aw->output_high = high;
        }
    }

    /**
     * @brief 执行抗饱和钳位与反算。
     * @param aw       抗饱和实例。
     * @param output   原始 PID 输出（将被钳位修正）。
     * @param p_term   比例项。
     * @param d_term   微分项。
     * @param integral 积分项指针（将被反算修正）。
     */
    PID_INLINE void pid_antiwindup_clamp(pid_antiwindup_t *aw, pid_real_t *output, pid_real_t p_term, pid_real_t d_term,
                                         pid_real_t *integral)
    {
        if ((aw == NULL) || (!aw->enable))
        {
            return;
        }

        if (*output > aw->output_high)
        {
            *output   = aw->output_high;
            *integral = aw->output_high - p_term - d_term;
        }
        else if (*output < aw->output_low)
        {
            *output   = aw->output_low;
            *integral = aw->output_low - p_term - d_term;
        }
        else
        {
            /* 不饱和，无需修正 */
        }
    }

    /*---------------------------------------------------------------------------*/
    /* 一阶低通滤波器（用于微分项） */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;      /**< 启用标志 */
        pid_real_t alpha;       /**< 滤波系数 (0~1)，越小滤波越强 */
        pid_real_t last_output; /**< 上一次滤波输出 */
    } pid_filter_t;

    PID_INLINE void pid_filter_init(pid_filter_t *f, pid_real_t alpha)
    {
        if (f != NULL)
        {
            f->enable      = true;
            f->alpha       = alpha;
            f->last_output = (pid_real_t) 0;
        }
    }

    /**
     * @brief 一阶低通滤波。
     * @param f     滤波器实例。
     * @param input 原始输入。
     * @return 滤波后的值。
     */
    PID_INLINE pid_real_t pid_filter_apply(pid_filter_t *f, pid_real_t input)
    {
        if ((f == NULL) || (!f->enable))
        {
            return input;
        }
        f->last_output = f->last_output + f->alpha * (input - f->last_output);
        return f->last_output;
    }

    /*---------------------------------------------------------------------------*/
    /* 死区 */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;
        pid_real_t threshold; /**< 死区宽度（绝对值） */
    } pid_deadband_t;

    PID_INLINE void pid_deadband_init(pid_deadband_t *db, pid_real_t threshold)
    {
        if (db != NULL)
        {
            db->enable    = true;
            db->threshold = threshold;
        }
    }

    /**
     * @brief 死区处理。
     * @param db    死区实例。
     * @param error 原始误差。
     * @return 处理后的误差。
     */
    PID_INLINE pid_real_t pid_deadband_apply(pid_deadband_t *db, pid_real_t error)
    {
        if ((db != NULL) && db->enable)
        {
            if (PID_REAL_ABS(error) < db->threshold)
            {
                return (pid_real_t) 0;
            }
        }
        return error;
    }

    /*---------------------------------------------------------------------------*/
    /* 设定值斜坡（变化率限制） */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;
        pid_real_t current;    /**< 当前斜坡设定值 */
        pid_real_t rate_limit; /**< 最大变化率（单位/秒） */
    } pid_ramp_t;

    PID_INLINE void pid_ramp_init(pid_ramp_t *ramp, pid_real_t rate_limit)
    {
        if (ramp != NULL)
        {
            ramp->enable     = true;
            ramp->current    = (pid_real_t) 0;
            ramp->rate_limit = rate_limit;
        }
    }

    /**
     * @brief 更新斜坡设定值。
     * @param ramp   斜坡实例。
     * @param target 目标设定值。
     * @param dt     采样间隔（秒）。
     * @return 当前平滑后的设定值。
     */
    PID_INLINE pid_real_t pid_ramp_update(pid_ramp_t *ramp, pid_real_t target, pid_real_t dt)
    {
        if ((ramp == NULL) || (!ramp->enable))
        {
            return target;
        }

        pid_real_t max_step = ramp->rate_limit * dt;
        if (max_step < (pid_real_t) 0)
        {
            max_step = (pid_real_t) 0;
        }

        pid_real_t diff = target - ramp->current;
        if (diff > max_step)
        {
            ramp->current += max_step;
        }
        else if (diff < -max_step)
        {
            ramp->current -= max_step;
        }
        else
        {
            ramp->current = target;
        }
        return ramp->current;
    }

    /**
     * @brief 强制设置斜坡当前值（用于无扰切换）。
     * @param ramp  斜坡实例。
     * @param value 当前值。
     */
    PID_INLINE void pid_ramp_force(pid_ramp_t *ramp, pid_real_t value)
    {
        if (ramp != NULL)
        {
            ramp->current = value;
        }
    }

    /*---------------------------------------------------------------------------*/
    /* 输出变化率限制 */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;
        pid_real_t max_rate;    /**< 最大变化率（单位/秒） */
        pid_real_t last_output; /**< 上一次输出 */
    } pid_rate_limit_t;

    PID_INLINE void pid_rate_limit_init(pid_rate_limit_t *rl, pid_real_t max_rate)
    {
        if (rl != NULL)
        {
            rl->enable      = true;
            rl->max_rate    = max_rate;
            rl->last_output = (pid_real_t) 0;
        }
    }

    /**
     * @brief 应用输出变化率限制。
     * @param rl     率限制实例。
     * @param output 当前计算原始输出。
     * @param dt     采样间隔（秒）。
     * @return 限制后的输出。
     */
    PID_INLINE pid_real_t pid_rate_limit_apply(pid_rate_limit_t *rl, pid_real_t output, pid_real_t dt)
    {
        if ((rl == NULL) || (!rl->enable))
        {
            rl->last_output = output; /* 跟踪实际输出，防止切回时跳变 */
            return output;
        }

        pid_real_t max_step = rl->max_rate * dt;
        if (max_step < (pid_real_t) 0)
        {
            max_step = (pid_real_t) 0;
        }

        pid_real_t diff = output - rl->last_output;
        if (diff > max_step)
        {
            rl->last_output += max_step;
        }
        else if (diff < -max_step)
        {
            rl->last_output -= max_step;
        }
        else
        {
            rl->last_output = output;
        }
        return rl->last_output;
    }

    /**
     * @brief 强制设置率限制器当前值（用于手动/自动切换）。
     */
    PID_INLINE void pid_rate_limit_force(pid_rate_limit_t *rl, pid_real_t value)
    {
        if (rl != NULL)
        {
            rl->last_output = value;
        }
    }

    /*---------------------------------------------------------------------------*/
    /* 积分分离判断 */
    /*---------------------------------------------------------------------------*/
    typedef struct
    {
        bool       enable;
        pid_real_t threshold; /**< 误差绝对值超过此值则积分关闭 */
    } pid_integral_sep_t;

    PID_INLINE void pid_integral_sep_init(pid_integral_sep_t *sep, pid_real_t threshold)
    {
        if (sep != NULL)
        {
            sep->enable    = true;
            sep->threshold = threshold;
        }
    }

    /**
     * @brief 判断积分是否应激活。
     * @param sep   积分分离实例。
     * @param error 当前误差。
     * @return true 激活积分，false 关闭积分。
     */
    PID_INLINE bool pid_integral_sep_is_active(pid_integral_sep_t *sep, pid_real_t error)
    {
        if ((sep != NULL) && sep->enable)
        {
            return (PID_REAL_ABS(error) <= sep->threshold);
        }
        return true;
    }

#ifdef __cplusplus
}
#endif

#endif /* PID_EXT_H */
