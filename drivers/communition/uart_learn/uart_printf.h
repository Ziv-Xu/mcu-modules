/**
 * @file    uart_printf.h
 * @brief   轻量级 printf 重定向（可选）
 *
 * @note    启用 UART_CFG_PRINTF_ENABLE 后，可通过 uart_printf 输出格式化字符串。
 *          不依赖动态内存，支持 %d, %x, %s, %c 等基本格式。
 *          也可通过重定向标准库 _write 实现，但本文件提供独立实现。
 */

#ifndef UART_PRINTF_H__
#define UART_PRINTF_H__

#include "uart_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if UART_CFG_PRINTF_ENABLE

    /**
     * @brief 绑定用于 printf 输出的 UART 句柄
     * @param huart 需初始化的串口句柄
     */
    void uart_printf_set_handle(uart_handle_t *huart);

    /**
     * @brief 自定义轻量 printf，输出到绑定的串口
     * @param fmt  格式化字符串（支持 %d, %u, %x, %X, %s, %c, %%）
     * @param ...  可变参数
     * @note       不支持浮点数、字段宽度等高级特性，适合嵌入式使用。
     */
    void uart_printf(const char *fmt, ...);

#endif /* UART_CFG_PRINTF_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* UART_PRINTF_H__ */
