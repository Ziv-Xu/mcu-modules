/**
 * @file    pid_template.h
 * @brief   全局配置、数据类型定义、硬件抽象层接口、临界区宏及错误码。
 * @details 用户可通过在本文件或编译选项中定义宏来裁剪功能、切换数据类型。
 *          所有硬件相关操作通过注册函数指针完成，杜绝直接访问寄存器。
 */

#ifndef PID_TEMPLATE_H
#define PID_TEMPLATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/
/* 数据类型抽象 */
/*---------------------------------------------------------------------------*/

/**
 * @brief 可配置的 PID 实数类型。
 *        默认使用单精度浮点，可通过定义 PID_USE_DOUBLE 切换为双精度，
 *        或定义 PID_USE_FIXED_Q15 切换为定点 Q15（需自行实现定点运算宏）。
 */
#if defined(PID_USE_DOUBLE)
    typedef double pid_real_t;
#define PID_REAL_ABS(x) ((x) < 0.0 ? -(x) : (x))
#elif defined(PID_USE_FIXED_Q15)
typedef int16_t pid_real_t; /* 须配套定点乘除宏，此处仅作示例 */
#define PID_REAL_ABS(x) ((x) < 0 ? (pid_real_t) (-(x)) : (x))
#else
typedef float pid_real_t;
#define PID_REAL_ABS(x) ((x) < 0.0f ? -(x) : (x))
#endif

/**
 * @brief 内联函数宏，可跨编译器配置。
 */
#ifndef PID_INLINE
#define PID_INLINE static inline
#endif

/*---------------------------------------------------------------------------*/
/* 临界区保护（用户必须按平台实现） */
/*---------------------------------------------------------------------------*/
#ifndef PID_ENTER_CRITICAL
#define PID_ENTER_CRITICAL() /* 进入临界区 */
#endif
#ifndef PID_EXIT_CRITICAL
#define PID_EXIT_CRITICAL() /* 退出临界区 */
#endif

    /*---------------------------------------------------------------------------*/
    /* 硬件抽象层 */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief 硬件抽象层接口。
     * @note  所有与硬件交互的操作均通过此结构体中的函数指针完成。
     *        若某项不需要，可将其设为 NULL。
     */
    typedef struct
    {
        /**
         * @brief 获取系统时间戳（单位：微秒）。
         * @return 当前时间戳（自由计数，仅相对差值有意义）。
         */
        uint32_t (*get_timestamp_us)(void);

        /**
         * @brief 输出执行器写入（如 DAC / PWM 占空比）。
         *        若用户自行读取输出值并执行，此指针可置为 NULL。
         * @param value 控制器输出值。
         */
        void (*set_output)(pid_real_t value);
    } pid_hal_t;

    /*---------------------------------------------------------------------------*/
    /* 错误码 */
    /*---------------------------------------------------------------------------*/
    typedef enum
    {
        PID_OK              = 0,
        PID_ERR_PARAM       = -1, /**< 参数无效（Kp/Ki/Kd为负，采样时间过小等） */
        PID_ERR_SAMPLE_TIME = -2, /**< 采样时间异常 */
        PID_ERR_NULL_PTR    = -3, /**< 传入了空指针 */
        PID_ERR_NOT_INIT    = -4, /**< 控制器尚未初始化 */
        PID_ERR_IMPL        = -5  /**< 实现内部错误 */
    } pid_error_t;

    /*---------------------------------------------------------------------------*/
    /* 控制器类型标签（用于多态识别） */
    /*---------------------------------------------------------------------------*/
    typedef enum
    {
        PID_TYPE_POSITIONAL  = 0,
        PID_TYPE_INCREMENTAL = 1,
        PID_TYPE_CASCADE     = 2,
        PID_TYPE_ADAPTIVE    = 3
    } pid_type_t;

    /*---------------------------------------------------------------------------*/
    /* 通用参数结构体（持久化接口使用） */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief 基本 PID 参数。
     * @note  不同变体可在此基础上派生扩展参数结构体，但均需包含本基类。
     */
    typedef struct
    {
        pid_real_t kp;             /**< 比例增益 */
        pid_real_t ki;             /**< 积分增益 */
        pid_real_t kd;             /**< 微分增益 */
        uint32_t   sample_time_ms; /**< 期望采样周期（毫秒） */
    } pid_params_t;

    /**
     * @brief 参数序列化/反序列化接口声明（由用户实现）。
     * @param params 参数结构体指针。
     * @param buffer 存储/读取的缓冲区。
     * @param buf_len 缓冲区长度。
     * @return 实际处理的字节数，<0 表示错误。
     */
    int pid_params_serialize(const pid_params_t *params, uint8_t *buffer, size_t buf_len);
    int pid_params_deserialize(pid_params_t *params, const uint8_t *buffer, size_t buf_len);

    /*---------------------------------------------------------------------------*/
    /* 统一控制器句柄（虚函数表风格多态） */
    /*---------------------------------------------------------------------------*/
    typedef struct pid_controller_t
    {
        pid_type_t type; /**< 控制器变体类型 */
        void      *impl; /**< 指向具体实现结构体 */

        /**
         * @brief 执行一次 PID 计算。
         * @param ctrl  控制器句柄。
         * @param sp    设定值。
         * @param pv    过程变量（测量值）。
         * @param output 计算所得的控制输出（由实现写入）。
         * @return 错误码。
         */
        pid_error_t (*compute)(struct pid_controller_t *ctrl, pid_real_t sp, pid_real_t pv, pid_real_t *output);

        /**
         * @brief 复位控制器状态（清除积分、历史误差等）。
         * @param ctrl 控制器句柄。
         */
        void (*reset)(struct pid_controller_t *ctrl);

        /**
         * @brief 在线修改 PID 参数。
         * @param ctrl   控制器句柄。
         * @param params 新参数（实现内自行处理平稳过渡）。
         * @return 错误码。
         */
        pid_error_t (*set_params)(struct pid_controller_t *ctrl, const pid_params_t *params);

        /**
         * @brief 获取当前 PID 参数，用于持久化或监控。
         * @param ctrl   控制器句柄。
         * @param params 输出参数结构体。
         * @return 错误码。
         */
        pid_error_t (*get_params)(struct pid_controller_t *ctrl, pid_params_t *params);
    } pid_controller_t;

#ifdef __cplusplus
}
#endif

#endif /* PID_TEMPLATE_H */
