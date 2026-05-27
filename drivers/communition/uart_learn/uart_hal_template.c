/**
 * @file    uart_hal_template.c
 * @brief   HAL 弱实现模板及中断处理骨架
 * @author  Your Name
 * @date    2026-05-22
 * @copyright Copyright (c) 2026, All rights reserved.
 *
 * @details 此文件提供所有 HAL 操作的弱定义实现（默认返回错误或空操作），
 *          并给出一个标准的 ISR 处理骨架，用户移植时需根据硬件填充具体操作。
 *
 * 修改历史:
 *   - 2026-05-22: 初始版本
 */

#include "uart_hal.h"
#include "uart_core.h" /* 核心层，用于中断内回调 */

/*==================== 弱实现 HAL 操作 ====================*/

__attribute__((weak)) uart_status_t hal_init(struct uart_handle *huart)
{
    (void) huart;
    /* 用户需实现：使能时钟、配置 GPIO、设置波特率等 */
    return UART_ERROR;
}

__attribute__((weak)) uart_status_t hal_deinit(struct uart_handle *huart)
{
    (void) huart;
    return UART_ERROR;
}

__attribute__((weak)) void hal_send_byte(struct uart_handle *huart, uint8_t data)
{
    (void) huart;
    (void) data;
    /* 用户需实现：等待 TXE 置位，然后写 DR */
}

__attribute__((weak)) uint8_t hal_recv_byte(struct uart_handle *huart)
{
    (void) huart;
    /* 用户需实现：读取 DR */
    return 0;
}

__attribute__((weak)) uint32_t hal_get_status(struct uart_handle *huart)
{
    (void) huart;
    /* 用户需实现：返回 SR 寄存器 */
    return 0;
}

__attribute__((weak)) void hal_clear_flags(struct uart_handle *huart, uint32_t flags)
{
    (void) huart;
    (void) flags;
    /* 用户需实现：写相应标志位清除（如写 USART_ICR） */
}

__attribute__((weak)) void hal_enable_tx_int(struct uart_handle *huart)
{
    (void) huart;
    /* 用户需实现：置位 TXEIE/TCIE */
}

__attribute__((weak)) void hal_disable_tx_int(struct uart_handle *huart)
{
    (void) huart;
}

__attribute__((weak)) void hal_enable_rx_int(struct uart_handle *huart)
{
    (void) huart;
}

__attribute__((weak)) void hal_disable_rx_int(struct uart_handle *huart)
{
    (void) huart;
}

__attribute__((weak)) void hal_enable_idle_int(struct uart_handle *huart)
{
    (void) huart;
    /* 用户需实现：置位 IDLEIE */
}

__attribute__((weak)) void hal_disable_idle_int(struct uart_handle *huart)
{
    (void) huart;
}

__attribute__((weak)) uart_status_t hal_dma_tx_start(struct uart_handle *huart, const uint8_t *data, size_t len)
{
    (void) huart;
    (void) data;
    (void) len;
    return UART_ERROR;
}

__attribute__((weak)) uart_status_t hal_dma_rx_start(struct uart_handle *huart, uint8_t *data, size_t len)
{
    (void) huart;
    (void) data;
    (void) len;
    return UART_ERROR;
}

__attribute__((weak)) uint32_t hal_get_tick_ms(void)
{
    /* 用户需实现：返回系统毫秒计数器 */
    return 0;
}

/*==================== 中断处理骨架 ====================*/

/**
 * @brief 通用 UART 中断服务程序入口
 *
 * 用户需在具体平台的 ISR（如 USART1_IRQHandler）中调用本函数，
 * 传入对应的句柄。此函数读取硬件状态并调用核心层事件处理。
 *
 * 使用示例（STM32）：
 * @code
 * void USART1_IRQHandler(void) {
 *     uart_hal_irq_handler(&huart1);
 * }
 * @endcode
 */
void uart_hal_irq_handler(struct uart_handle *huart)
{
    if (huart == NULL)
        return;
    const uart_hal_ops_t *ops = huart->ops;
    if (ops == NULL)
        return;

    uint32_t status = ops->get_status(huart);

    /* ---- 需要用户根据平台定义标志位 ---- */
    /* 示例标志位定义（用户应在平台头文件中给出）：
       #define UART_FLAG_RXNE   (1 << 5)
       #define UART_FLAG_TXE    (1 << 7)
       #define UART_FLAG_IDLE   (1 << 4)
       #define UART_FLAG_ORE    (1 << 3)
       #define UART_FLAG_FE     (1 << 0)
       #define UART_FLAG_PE     (1 << 1)
       #define UART_FLAG_NE     (1 << 2)
    */

    /* 接收数据就绪 */
    if (status & UART_FLAG_RXNE)
    {
        uart_core_rx_isr(huart);
    }

    /* 发送数据寄存器空 */
    if (status & UART_FLAG_TXE)
    {
        uart_core_tx_isr(huart);
    }

    /* 线路空闲中断 */
    if (status & UART_FLAG_IDLE)
    {
        ops->clear_flags(huart, UART_FLAG_IDLE);
        uart_core_idle_isr(huart);
    }

    /* 错误处理：溢出/帧错/校验/噪声 */
    if (status & (UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_PE | UART_FLAG_NE))
    {
        uart_error_t err = UART_ERR_NONE;
        if (status & UART_FLAG_FE)
            err |= UART_ERR_FRAME;
        if (status & UART_FLAG_PE)
            err |= UART_ERR_PARITY;
        if (status & UART_FLAG_ORE)
            err |= UART_ERR_OVERRUN;
        if (status & UART_FLAG_NE)
            err |= UART_ERR_NOISE;
        ops->clear_flags(huart, UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_PE | UART_FLAG_NE);
        uart_core_error_isr(huart, err);
    }
}
