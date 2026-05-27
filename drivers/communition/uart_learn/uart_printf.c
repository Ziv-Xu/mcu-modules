/**
 * @file    uart_printf.c
 * @brief   轻量 printf 实现
 */

#include "uart_printf.h"
#include <stdarg.h>
#include <stdbool.h>

#if UART_CFG_PRINTF_ENABLE

static uart_handle_t *pr_huart = NULL;

void uart_printf_set_handle(uart_handle_t *huart)
{
    pr_huart = huart;
}

static void uart_putc(char c)
{
    if (pr_huart)
    {
        uart_send(pr_huart, (const uint8_t *) &c, 1, 100);
    }
}

static void uart_puts(const char *str)
{
    while (*str)
    {
        uart_putc(*str++);
    }
}

static void uart_print_int(int32_t num, int base, bool uppercase)
{
    char     buf[12]; /* 最大支持 32 位 -2147483648 */
    uint8_t  idx      = 0;
    bool     negative = false;
    uint32_t unum;

    if (base == 10 && num < 0)
    {
        negative = true;
        unum     = (uint32_t) (-num);
    }
    else
    {
        unum = (uint32_t) num;
    }

    do
    {
        uint32_t rem = unum % base;
        buf[idx++] = (rem < 10) ? (char) ('0' + rem) : (uppercase ? (char) ('A' + rem - 10) : (char) ('a' + rem - 10));
        unum /= base;
    } while (unum > 0);

    if (negative)
        uart_putc('-');
    while (idx > 0)
    {
        uart_putc(buf[--idx]);
    }
}

void uart_printf(const char *fmt, ...)
{
    if (pr_huart == NULL || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);

    while (*fmt)
    {
        if (*fmt != '%')
        {
            uart_putc(*fmt++);
            continue;
        }
        fmt++; /* skip '%' */
        switch (*fmt)
        {
            case 'd':
                uart_print_int(va_arg(args, int32_t), 10, false);
                break;
            case 'u':
                uart_print_int(va_arg(args, uint32_t), 10, false);
                break;
            case 'x':
                uart_print_int(va_arg(args, uint32_t), 16, false);
                break;
            case 'X':
                uart_print_int(va_arg(args, uint32_t), 16, true);
                break;
            case 'c':
                uart_putc((char) va_arg(args, int));
                break;
            case 's':
            {
                const char *str = va_arg(args, const char *);
                if (str)
                    uart_puts(str);
                else
                    uart_puts("(null)");
                break;
            }
            case '%':
                uart_putc('%');
                break;
            default:
                uart_putc('%');
                uart_putc(*fmt);
                break;
        }
        fmt++;
    }
    va_end(args);
}

#endif /* UART_CFG_PRINTF_ENABLE */
