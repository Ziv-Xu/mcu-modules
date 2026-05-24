/**
 * @file    uart_core.c
 * @brief   UART 核心逻辑实现
 */

#include "uart_core.h"
#include <string.h>

/*==================== 内部辅助宏 ====================*/

#define UART_ENTER_CRITICAL(huart) UART_MUTEX_LOCK((huart)->lock)
#define UART_EXIT_CRITICAL(huart)  UART_MUTEX_UNLOCK((huart)->lock)

/*==================== 初始化 ====================*/

uart_status_t uart_init(uart_handle_t *huart, const uart_config_t *config, const uart_hal_ops_t *ops, void *priv)
{
    if (huart == NULL || config == NULL || ops == NULL)
        return UART_BAD_PARAM;

    memset(huart, 0, sizeof(uart_handle_t));
    huart->config = *config;
    huart->ops    = ops;
    huart->priv   = priv;

    UART_MUTEX_INIT(huart->lock);

#if UART_CFG_RINGBUF_ENABLE
    ringbuf_init(&huart->tx_rb, huart->tx_buf_mem, UART_CFG_TX_RINGBUF_SIZE);
    ringbuf_init(&huart->rx_rb, huart->rx_buf_mem, UART_CFG_RX_RINGBUF_SIZE);
#endif

    uart_status_t ret = ops->init(huart);
    if (ret != UART_OK)
    {
        UART_MUTEX_DEINIT(huart->lock);
        return ret;
    }

    /* 默认使能接收中断，数据进入环形缓冲区 */
    huart->ops->enable_rx_int(huart);
    huart->rx_state = UART_RX_IDLE;

    return UART_OK;
}

uart_status_t uart_deinit(uart_handle_t *huart)
{
    if (huart == NULL)
        return UART_BAD_PARAM;
    huart->ops->disable_tx_int(huart);
    huart->ops->disable_rx_int(huart);
    huart->ops->disable_idle_int(huart);
    huart->ops->deinit(huart);
    UART_MUTEX_DEINIT(huart->lock);
    memset(huart, 0, sizeof(uart_handle_t));
    return UART_OK;
}

/*==================== 阻塞发送/接收 ====================*/

uart_status_t uart_send(uart_handle_t *huart, const uint8_t *pdata, size_t len, uint32_t timeout_ms)
{
    if (huart == NULL || pdata == NULL || len == 0)
        return UART_BAD_PARAM;
    const uart_hal_ops_t *ops = huart->ops;

    uint32_t start = ops->get_tick_ms();
    size_t   sent  = 0;

    while (sent < len)
    {
        /* 检查超时 */
        if (timeout_ms > 0)
        {
            uint32_t elapsed = ops->get_tick_ms() - start;
            if (elapsed >= timeout_ms)
                return UART_TIMEOUT;
        }

        /* 等待发送寄存器空（需平台定义 UART_FLAG_TXE） */
        if (ops->get_status(huart) & UART_FLAG_TXE)
        {
            ops->send_byte(huart, pdata[sent++]);
        }
    }

    /* 等待发送完成（TC）以确保数据完全移出 */
    if (timeout_ms > 0)
    {
        uint32_t start2 = ops->get_tick_ms();
        while (!(ops->get_status(huart) & UART_FLAG_TC))
        {
            if ((ops->get_tick_ms() - start2) >= timeout_ms)
                return UART_TIMEOUT;
        }
        ops->clear_flags(huart, UART_FLAG_TC);
    }
    return UART_OK;
}

uart_status_t uart_receive(uart_handle_t *huart, uint8_t *pdata, size_t len, uint32_t timeout_ms)
{
    if (huart == NULL || pdata == NULL || len == 0)
        return UART_BAD_PARAM;
    const uart_hal_ops_t *ops = huart->ops;

    uint32_t start = ops->get_tick_ms();
    size_t   recv  = 0;

    while (recv < len)
    {
        if (timeout_ms > 0)
        {
            uint32_t elapsed = ops->get_tick_ms() - start;
            if (elapsed >= timeout_ms)
                return UART_TIMEOUT;
        }

        if (ops->get_status(huart) & UART_FLAG_RXNE)
        {
            pdata[recv++] = ops->recv_byte(huart);
            start         = ops->get_tick_ms(); /* 字节间超时重置 */
        }
    }
    return UART_OK;
}

/*==================== 异步发送 ====================*/

#if UART_CFG_ASYNC_ENABLE

uart_status_t uart_send_async(uart_handle_t *huart, const uint8_t *pdata, size_t len)
{
    if (huart == NULL || pdata == NULL || len == 0)
        return UART_BAD_PARAM;

    UART_ENTER_CRITICAL(huart);
    if (huart->tx_state == UART_TX_BUSY)
    {
        UART_EXIT_CRITICAL(huart);
        return UART_BUSY;
    }

    /* 将数据填入发送环形缓冲 */
    for (size_t i = 0; i < len; i++)
    {
        if (!ringbuf_put(&huart->tx_rb, pdata[i]))
        {
            huart->tx_state = UART_TX_ERROR;
            UART_EXIT_CRITICAL(huart);
            return UART_ERROR;
        }
    }

    huart->tx_state = UART_TX_BUSY;
    huart->ops->enable_tx_int(huart); /* 开始发送 */
    UART_EXIT_CRITICAL(huart);
    return UART_OK;
}

uart_status_t uart_receive_async(uart_handle_t *huart, uint8_t *pdata, size_t len)
{
    if (huart == NULL || pdata == NULL || len == 0)
        return UART_BAD_PARAM;

    UART_ENTER_CRITICAL(huart);
    if (huart->rx_state != UART_RX_IDLE)
    {
        UART_EXIT_CRITICAL(huart);
        return UART_BUSY;
    }

    huart->rx_user_buf   = pdata;
    huart->rx_expect_len = len;
    huart->rx_recv_cnt   = 0;
    huart->rx_state      = UART_RX_LINEAR;

    /* 使能接收中断（可能已经使能），并确保空闲中断暂时关闭 */
    huart->ops->enable_rx_int(huart);
    huart->ops->disable_idle_int(huart);
    UART_EXIT_CRITICAL(huart);
    return UART_OK;
}

#endif /* UART_CFG_ASYNC_ENABLE */

/*==================== 不定长接收（空闲中断） ====================*/

uart_status_t uart_start_rx_idle(uart_handle_t *huart, uint8_t *buffer, size_t max_len)
{
    if (huart == NULL)
        return UART_BAD_PARAM;

    UART_ENTER_CRITICAL(huart);
    if (huart->rx_state != UART_RX_IDLE)
    {
        UART_EXIT_CRITICAL(huart);
        return UART_BUSY;
    }

    /* 清空接收环形缓冲 */
    ringbuf_flush(&huart->rx_rb);

    huart->rx_user_buf   = buffer;
    huart->rx_expect_len = max_len;
    huart->rx_recv_cnt   = 0;
    huart->rx_state      = UART_RX_IDLE_ACTIVE;

    huart->ops->enable_rx_int(huart);
    huart->ops->enable_idle_int(huart);
    UART_EXIT_CRITICAL(huart);
    return UART_OK;
}

size_t uart_get_rx_count(uart_handle_t *huart)
{
    if (huart == NULL)
        return 0;
    return ringbuf_available(&huart->rx_rb);
}

/*==================== 回调设置 ====================*/

void uart_set_tx_cplt_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *))
{
    if (huart)
        huart->tx_cplt_cb = cb;
}

void uart_set_rx_cplt_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *, size_t len))
{
    if (huart)
        huart->rx_cplt_cb = cb;
}

void uart_set_error_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *, uart_error_t err))
{
    if (huart)
        huart->error_cb = cb;
}

/*==================== 杂项 ====================*/

uart_status_t uart_flush(uart_handle_t *huart)
{
    if (huart == NULL)
        return UART_BAD_PARAM;
    /* 等待发送完成 */
    const uart_hal_ops_t *ops = huart->ops;
    while (ringbuf_available(&huart->tx_rb) > 0 || !(ops->get_status(huart) & UART_FLAG_TC))
    {
        /* 可加入超时 */
    }
    ops->clear_flags(huart, UART_FLAG_TC);
    return UART_OK;
}

uart_status_t uart_abort(uart_handle_t *huart)
{
    if (huart == NULL)
        return UART_BAD_PARAM;
    UART_ENTER_CRITICAL(huart);
    huart->ops->disable_tx_int(huart);
    huart->ops->disable_rx_int(huart);
    huart->ops->disable_idle_int(huart);
    huart->tx_state = UART_TX_IDLE;
    huart->rx_state = UART_RX_IDLE;
    ringbuf_flush(&huart->tx_rb);
    ringbuf_flush(&huart->rx_rb);
    UART_EXIT_CRITICAL(huart);
    return UART_OK;
}

/*==================== 中断处理函数（由 HAL ISR 调用） ====================*/

void uart_core_tx_isr(uart_handle_t *huart)
{
#if UART_CFG_RINGBUF_ENABLE
    if (huart->tx_state != UART_TX_BUSY)
        return;

    if (ringbuf_available(&huart->tx_rb) > 0)
    {
        uint8_t data;
        ringbuf_get(&huart->tx_rb, &data);
        huart->ops->send_byte(huart, data);
    }
    else
    {
        /* 发送缓冲空，停止 TX 中断 */
        huart->ops->disable_tx_int(huart);
        huart->tx_state = UART_TX_IDLE;
        if (huart->tx_cplt_cb)
        {
            huart->tx_cplt_cb(huart);
        }
    }
#endif
}

void uart_core_rx_isr(uart_handle_t *huart)
{
    uint8_t data = huart->ops->recv_byte(huart);

    if (huart->rx_state == UART_RX_IDLE_ACTIVE)
    {
        /* 空闲中断接收模式：数据存入环形缓冲 */
        ringbuf_put(&huart->rx_rb, data);
    }
#if UART_CFG_ASYNC_ENABLE
    else if (huart->rx_state == UART_RX_LINEAR)
    {
        /* 线性接收模式 */
        if (huart->rx_recv_cnt < huart->rx_expect_len)
        {
            huart->rx_user_buf[huart->rx_recv_cnt++] = data;
            if (huart->rx_recv_cnt >= huart->rx_expect_len)
            {
                huart->ops->disable_rx_int(huart);
                huart->rx_state = UART_RX_IDLE;
                if (huart->rx_cplt_cb)
                {
                    huart->rx_cplt_cb(huart, huart->rx_recv_cnt);
                }
            }
        }
    }
#endif
    else
    {
        /* 默认仍存入环形缓冲（后台接收） */
        ringbuf_put(&huart->rx_rb, data);
    }
}

void uart_core_idle_isr(uart_handle_t *huart)
{
    if (huart->rx_state != UART_RX_IDLE_ACTIVE)
        return;

    size_t recv_len = ringbuf_available(&huart->rx_rb);
    if (huart->rx_user_buf != NULL && recv_len > 0)
    {
        /* 将环形缓冲数据拷贝到用户缓冲区 */
        size_t copy_len = (recv_len > huart->rx_expect_len) ? huart->rx_expect_len : recv_len;
        for (size_t i = 0; i < copy_len; i++)
        {
            ringbuf_get(&huart->rx_rb, &huart->rx_user_buf[i]);
        }
        recv_len = copy_len;
    }

    huart->ops->disable_idle_int(huart);
    huart->rx_state = UART_RX_IDLE;

    if (huart->rx_cplt_cb)
    {
        huart->rx_cplt_cb(huart, recv_len);
    }
}

void uart_core_error_isr(uart_handle_t *huart, uart_error_t err)
{
    /* 错误恢复：清空接收缓冲、状态复位 */
    ringbuf_flush(&huart->rx_rb);
    huart->rx_state = UART_RX_IDLE;
    huart->ops->disable_rx_int(huart);
    huart->ops->enable_rx_int(huart); /* 重新使能 */

    if (huart->error_cb)
    {
        huart->error_cb(huart, err);
    }
}
