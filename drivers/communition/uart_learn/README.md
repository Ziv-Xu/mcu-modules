 # 实现 HAL 操作集
创建 uart_hal_<平台名>.c，实现 uart_hal_ops_t 中的全部函数：

init: 配置时钟、GPIO、UART 模式、波特率、中断优先级等。

deinit: 关闭时钟、恢复 GPIO 为默认。

send_byte / recv_byte: 直接操作数据寄存器。

get_status / clear_flags: 读取和清除状态寄存器。

enable/disable_tx/rx/idle_int: 控制中断使能位。

dma_tx_start / dma_rx_start (可选): 配置 DMA 传输。

get_tick_ms: 返回系统毫秒时间戳（如 HAL_GetTick() 或自定义计数器）。

### 示例 (STM32 HAL 库)：

```c  
const uart_hal_ops_t uart_hal_stm32 = 
{
    .init = stm32_hal_init,
    .deinit = stm32_hal_deinit,
    /* ... */
};
```
### 示例 (裸寄存器)：

```c
static uint32_t reg_get_status(struct uart_handle *huart) 
{
    USART_TypeDef *uart = (USART_TypeDef *)huart->priv;
    return uart->SR;
}
```
# 集成中断服务
在 MCU 的向量中断函数中调用 uart_hal_irq_handler(&your_huart)：

```c
void USART1_IRQHandler(void) 
{
    uart_hal_irq_handler(&huart1);
}
```
# 配置工程
将 uart_config.h 中的宏按需调整（如缓冲区大小、使能异步/DMA/RTOS/printf）。

若启用 RTOS，需在 uart_config.h 中提供正确的锁类型和操作。

确保标准 C 头文件（<stdint.h>, <stddef.h>, <stdbool.h>）可用。

# 验证
使用提供的示例（轮询发送/接收、中断不定长接收）测试功能。

# 注意事项
get_tick_ms 必须在硬件初始化后才能正确工作。

中断优先级需配置合理，避免嵌套耗时过长。

环形缓冲区操作在中断与主循环间共享，RTOS 环境下已由锁保护；裸机环境需在 HAL ISR 中避免重入，本模板已在核心层通过宏预留了关中断接口。


# 典型使用示例

```c
#include "uart_core.h"
#include "uart_printf.h"

/* 假设已实现 HAL 操作集 uart_hal_ops_stm32 */
extern const uart_hal_ops_t uart_hal_ops_stm32;

/* 定义串口句柄 */
uart_handle_t huart1;

/* 接收完成回调（不定长空闲中断） */
void on_rx_idle(uart_handle_t *h, size_t len)
{
    if (len > 0)
    {
        /* 回显接收到的数据 */
        uart_send(h, rx_buffer, len, 0);
    }
    /* 重新启动不定长接收 */
    uart_start_rx_idle(h, rx_buffer, sizeof(rx_buffer));
}

/* 发送完成回调 */
void on_tx_done(uart_handle_t *h)
{
    /* 可以发送下一包数据 */
}

/* 错误回调 */
void on_error(uart_handle_t *h, uart_error_t err)
{
    /* 日志记录或恢复处理 */
}

/* 主程序 */
int main(void)
{
    /* 系统初始化（时钟、滴答计时器等） */

    uart_config_t config = {.baudrate     = 115200,
                            .data_bits    = UART_DATA_BITS_8,
                            .stop_bits    = UART_STOP_BITS_1,
                            .parity       = UART_PARITY_NONE,
                            .flow_control = UART_FLOW_CONTROL_NONE};

    /* 初始化串口1，priv 传入 USART1 基地址 */
    uart_init(&huart1, &config, &uart_hal_ops_stm32, (void *) USART1);

    /* 设置回调 */
    uart_set_rx_cplt_callback(&huart1, on_rx_idle);
    uart_set_tx_cplt_callback(&huart1, on_tx_done);
    uart_set_error_callback(&huart1, on_error);

    /* 方式1：阻塞发送 "Hello" */
    uart_send(&huart1, (const uint8_t *) "Hello, UART!\r\n", 14, 1000);

    /* 方式2：启动不定长接收（空闲中断），缓冲区在回调中回显 */
    uint8_t rx_buf[128];
    uart_start_rx_idle(&huart1, rx_buf, sizeof(rx_buf));

#if UART_CFG_PRINTF_ENABLE
    /* 轻量 printf 输出 */
    uart_printf_set_handle(&huart1);
    uart_printf("System started. Tick: %u\r\n", huart1.ops->get_tick_ms());
#endif

    while (1)
    {
        /* 主循环可处理其他任务 */
    }
}

/* 移植时需要实现的 HAL 操作集示例 (stm32寄存器) */
/*
static uart_status_t my_init(struct uart_handle *huart) {
    USART_TypeDef *uart = (USART_TypeDef *)huart->priv;
    // 使能 GPIO 时钟和 USART 时钟...
    // 配置 CR1, CR2, CR3 根据 huart->config...
    uart->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    return UART_OK;
}
const uart_hal_ops_t uart_hal_ops_stm32 = {
    .init = my_init,
    ...
};
*/
