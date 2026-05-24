/**
 * @file    uart_hal.h
 * @brief   硬件抽象层接口定义
 * @author  Ziv-Xu
 * @date    2026-05-22
 * @copyright Copyright (c) 2026, All rights reserved.
 *
 * @details 本文件定义了与硬件无关的 UART 操作接口集。
 *          所有平台相关的硬件操作均通过本结构体中的函数指针调用。
 *          移植到新 MCU 时，只需实现这些函数并赋值给 uart_hal_ops_t 结构体。
 *
 * 修改历史:
 *   - 2026-05-22: 初始版本
 */

#ifndef UART_HAL_H__
#define UART_HAL_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "uart_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*==================== 通用枚举与配置结构体 ====================*/

    /** @brief 数据位 */
    typedef enum
    {
        UART_DATA_BITS_5 = 5,
        UART_DATA_BITS_6 = 6,
        UART_DATA_BITS_7 = 7,
        UART_DATA_BITS_8 = 8,
        UART_DATA_BITS_9 = 9
    } uart_data_bits_t;

    /** @brief 停止位 */
    typedef enum
    {
        UART_STOP_BITS_1 = 0,
        UART_STOP_BITS_0_5,
        UART_STOP_BITS_2,
        UART_STOP_BITS_1_5
    } uart_stop_bits_t;

    /** @brief 奇偶校验 */
    typedef enum
    {
        UART_PARITY_NONE = 0,
        UART_PARITY_ODD,
        UART_PARITY_EVEN,
        UART_PARITY_MARK,
        UART_PARITY_SPACE
    } uart_parity_t;

    /** @brief 硬件流控 */
    typedef enum
    {
        UART_FLOW_CONTROL_NONE = 0,
        UART_FLOW_CONTROL_RTS,
        UART_FLOW_CONTROL_CTS,
        UART_FLOW_CONTROL_RTS_CTS
    } uart_flow_control_t;

    /** @brief UART 状态码 */
    typedef enum
    {
        UART_OK        = 0,
        UART_ERROR     = -1,
        UART_BUSY      = -2,
        UART_TIMEOUT   = -3,
        UART_BAD_PARAM = -4
    } uart_status_t;

    /** @brief UART 错误类型掩码 */
    typedef enum
    {
        UART_ERR_NONE    = 0x00,
        UART_ERR_FRAME   = 0x01, /**< 帧错误 */
        UART_ERR_PARITY  = 0x02, /**< 奇偶校验错 */
        UART_ERR_OVERRUN = 0x04, /**< 溢出错误 */
        UART_ERR_NOISE   = 0x08, /**< 噪声错误 */
        UART_ERR_DMA     = 0x10, /**< DMA 传输错误 */
        UART_ERR_OTHER   = 0x80  /**< 其他错误 */
    } uart_error_t;

    /** @brief UART 通用配置 */
    typedef struct
    {
        uint32_t            baudrate;     /**< 波特率 */
        uart_data_bits_t    data_bits;    /**< 数据位 */
        uart_stop_bits_t    stop_bits;    /**< 停止位 */
        uart_parity_t       parity;       /**< 奇偶校验 */
        uart_flow_control_t flow_control; /**< 流控 */
        bool                inversion;    /**< 是否启用信号极性反转（预留） */
    } uart_config_t;

    /*==================== 前向声明 ====================*/

    struct uart_handle; /**< 核心层句柄，在 uart_core.h 中定义 */

    /*==================== HAL 操作函数集 ====================*/

    /**
     * @brief 硬件抽象层操作集
     * @note  每个平台需实现一个 const 实例，并在初始化时赋值给句柄。
     */
    typedef struct uart_hal_ops
    {
        /** @brief 初始化硬件（时钟、GPIO、UART 寄存器等） */
        uart_status_t (*init)(struct uart_handle *huart);

        /** @brief 反初始化硬件（恢复默认状态） */
        uart_status_t (*deinit)(struct uart_handle *huart);

        /** @brief 发送一个字节（阻塞直到发送寄存器空） */
        void (*send_byte)(struct uart_handle *huart, uint8_t data);

        /** @brief 读取一个字节（需要调用前保证数据就绪） */
        uint8_t (*recv_byte)(struct uart_handle *huart);

        /** @brief 获取 UART 状态标志（按位组合，由具体平台定义） */
        uint32_t (*get_status)(struct uart_handle *huart);

        /** @brief 清除指定标志 */
        void (*clear_flags)(struct uart_handle *huart, uint32_t flags);

        /** @brief 使能发送中断（TXE 或 TC） */
        void (*enable_tx_int)(struct uart_handle *huart);

        /** @brief 禁能发送中断 */
        void (*disable_tx_int)(struct uart_handle *huart);

        /** @brief 使能接收中断（RXNE） */
        void (*enable_rx_int)(struct uart_handle *huart);

        /** @brief 禁能接收中断 */
        void (*disable_rx_int)(struct uart_handle *huart);

        /** @brief 使能空闲线路检测中断（IDLE） */
        void (*enable_idle_int)(struct uart_handle *huart);

        /** @brief 禁能空闲中断 */
        void (*disable_idle_int)(struct uart_handle *huart);

        /** @brief 启动 DMA 发送（预留） */
        uart_status_t (*dma_tx_start)(struct uart_handle *huart, const uint8_t *data, size_t len);

        /** @brief 启动 DMA 接收（预留） */
        uart_status_t (*dma_rx_start)(struct uart_handle *huart, uint8_t *data, size_t len);

        /** @brief 获取系统滴答毫秒计数（用于超时计算） */
        uint32_t (*get_tick_ms)(void);

        /* 可扩展更多操作 */
    } uart_hal_ops_t;

    /*==================== 平台需要提供的标志位定义示例 ====================*/

    /*
     * 每个平台需自定义以下标志位，并在 get_status / clear_flags 中使用。
     * 示例（STM32）：
     * #define UART_FLAG_TXE      USART_SR_TXE
     * #define UART_FLAG_TC       USART_SR_TC
     * #define UART_FLAG_RXNE     USART_SR_RXNE
     * #define UART_FLAG_IDLE     USART_SR_IDLE
     * #define UART_FLAG_FE       USART_SR_FE
     * #define UART_FLAG_PE       USART_SR_PE
     * #define UART_FLAG_ORE      USART_SR_ORE
     * #define UART_FLAG_NE       USART_SR_NE
     */

    /*==================== 全局中断处理骨架（弱函数） ====================*/

    /**
     * @brief UART 全局中断处理入口（弱函数）
     * @param huart  串口句柄指针
     * @note  用户需在各自平台的 ISR 中调用此函数。
     *        该函数由核心层提供实现，调用 HAL 标志读取并分发事件。
     */
    void uart_hal_irq_handler(struct uart_handle *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_HAL_H__ */
