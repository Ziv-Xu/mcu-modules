/**
 * @file    uart_config.h
 * @brief   UART 驱动可裁剪配置头文件
 * @author  Ziv-Xu
 * @date    2026-05-22
 * @copyright Copyright (c) 2026, All rights reserved.
 *
 * @details 通过修改本文件的宏定义，可以控制 UART 驱动的功能模块，
 *          以适应资源受限或功能丰富的应用场景。
 *
 * 修改历史:
 *   - 2026-05-22: 初始版本
 */

#ifndef UART_CONFIG_H__
#define UART_CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*==================== 功能裁剪 ====================*/

/**
 * @brief 启用环形缓冲区（用于中断接收/发送缓冲）
 *        若为 0，所有异步收发功能不可用，仅保留阻塞 API。
 */
#define UART_CFG_RINGBUF_ENABLE 1

/**
 * @brief 启用异步（非阻塞）收发 API
 *        依赖 UART_CFG_RINGBUF_ENABLE。
 */
#define UART_CFG_ASYNC_ENABLE 1

/**
 * @brief 启用 DMA 支持（预留 scatter-gather 接口）
 *        当前版本需用户自行实现 DMA 相关 HAL 操作。
 */
#define UART_CFG_DMA_ENABLE 0

/**
 * @brief 启用自定义 printf 重定向
 */
#define UART_CFG_PRINTF_ENABLE 0

/**
 * @brief 启用 RTOS 线程安全支持
 *        若为 1，需在下方定义互斥锁相关宏。
 */
#define UART_CFG_RTOS_ENABLE 0

/*==================== 缓冲区大小 ====================*/

/**
 * @brief 默认发送环形缓冲区大小（字节）
 */
#define UART_CFG_TX_RINGBUF_SIZE 256

/**
 * @brief 默认接收环形缓冲区大小（字节）
 */
#define UART_CFG_RX_RINGBUF_SIZE 256

    /*==================== RTOS 互斥锁配置（当 UART_CFG_RTOS_ENABLE = 1 时） ====================*/

#if UART_CFG_RTOS_ENABLE
/* 包含 RTOS 头文件，例如 FreeRTOS */
/* #include "FreeRTOS.h" */
/* #include "semphr.h" */

/** @brief 互斥锁类型 */
#define UART_MUTEX_TYPE SemaphoreHandle_t

/** @brief 静态初始化互斥锁，传入 huart->lock */
#define UART_MUTEX_INIT(lock)           \
    do                                  \
    {                                   \
        lock = xSemaphoreCreateMutex(); \
    } while (0)

/** @brief 上锁 */
#define UART_MUTEX_LOCK(lock) xSemaphoreTake(lock, portMAX_DELAY)

/** @brief 解锁 */
#define UART_MUTEX_UNLOCK(lock) xSemaphoreGive(lock)

/** @brief 删除锁 */
#define UART_MUTEX_DEINIT(lock) vSemaphoreDelete(lock)
#else
/* 裸机：通过关中断保护，将在 HAL 层实现 */
#define UART_MUTEX_TYPE       void *
#define UART_MUTEX_INIT(lock) (void) (lock)
#define UART_MUTEX_LOCK(lock)
#define UART_MUTEX_UNLOCK(lock)
#define UART_MUTEX_DEINIT(lock)
#endif

#ifdef __cplusplus
}
#endif

#endif /* UART_CONFIG_H__ */
