/**
 * @file    uart_core.h
 * @brief   UART 核心逻辑层接口
 * @author  Ziv-Xu
 * @date    2026-05-22
 * @copyright Copyright (c) 2026, All rights reserved.
 *
 * @details 提供统一的 UART 操作 API，屏蔽底层硬件差异。
 *          所有多实例、缓冲管理、超时、回调均在此层实现。
 *
 * 修改历史:
 *   - 2026-05-22: 初始版本
 */

#ifndef UART_CORE_H__
#define UART_CORE_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "uart_hal.h"
#include "uart_ringbuf.h"
#include "uart_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*==================== 内部状态枚举 ====================*/

    /** @brief 发送状态 */
    typedef enum
    {
        UART_TX_IDLE = 0,
        UART_TX_BUSY,
        UART_TX_ERROR
    } uart_tx_state_t;

    /** @brief 接收状态 */
    typedef enum
    {
        UART_RX_IDLE = 0,
        UART_RX_LINEAR,      /**< 异步指定长度接收 */
        UART_RX_IDLE_ACTIVE, /**< 空闲中断接收模式 */
        UART_RX_ERROR
    } uart_rx_state_t;

    /*==================== 句柄定义 ====================*/

    /**
     * @brief UART 实例句柄
     */
    typedef struct uart_handle
    {
        /* 配置与硬件 */
        uart_config_t         config; /**< 当前配置 */
        const uart_hal_ops_t *ops;    /**< 硬件操作集 */
        void                 *priv;   /**< 硬件私有数据（如 USART_TypeDef*） */

        /* 状态 */
        uart_tx_state_t tx_state;
        uart_rx_state_t rx_state;

        /* 发送缓冲区 */
#if UART_CFG_RINGBUF_ENABLE
        ringbuf_t tx_rb;
        uint8_t   tx_buf_mem[UART_CFG_TX_RINGBUF_SIZE];
#endif

        /* 接收缓冲区（环形）*/
#if UART_CFG_RINGBUF_ENABLE
        ringbuf_t rx_rb;
        uint8_t   rx_buf_mem[UART_CFG_RX_RINGBUF_SIZE];
#endif

        /* 异步接收参数（线性模式） */
        uint8_t *rx_user_buf;   /**< 用户提供的接收缓冲区 */
        size_t   rx_expect_len; /**< 期望接收长度 */
        size_t   rx_recv_cnt;   /**< 已接收字节数 */

        /* 超时 */
        uint32_t tx_timeout_start;
        uint32_t rx_timeout_start;

        /* 回调函数 */
        void (*tx_cplt_cb)(struct uart_handle *);             /**< 发送完成 */
        void (*rx_cplt_cb)(struct uart_handle *, size_t len); /**< 接收完成（指定长度或空闲） */
        void (*error_cb)(struct uart_handle *, uart_error_t); /**< 错误发生 */

        /* 线程安全 */
        UART_MUTEX_TYPE lock;
    } uart_handle_t;

    /*==================== API 函数原型 ====================*/

    /**
     * @brief 初始化 UART 实例
     * @param huart  句柄指针
     * @param config 配置参数
     * @param ops    硬件操作集（平台实现）
     * @param priv   硬件私有数据（如 USART 基地址）
     * @return       UART_OK 成功，其他失败
     */
    uart_status_t uart_init(uart_handle_t *huart, const uart_config_t *config, const uart_hal_ops_t *ops, void *priv);

    /**
     * @brief 反初始化 UART 实例
     */
    uart_status_t uart_deinit(uart_handle_t *huart);

    /* ---- 阻塞传输 ---- */

    /**
     * @brief 阻塞发送指定长度数据
     * @param timeout_ms  超时时间（毫秒），0 表示永不超时
     */
    uart_status_t uart_send(uart_handle_t *huart, const uint8_t *pdata, size_t len, uint32_t timeout_ms);

    /**
     * @brief 阻塞接收指定长度数据
     */
    uart_status_t uart_receive(uart_handle_t *huart, uint8_t *pdata, size_t len, uint32_t timeout_ms);

/* ---- 异步传输（非阻塞，需回调） ---- */
#if UART_CFG_ASYNC_ENABLE

    /**
     * @brief 异步发送（数据拷贝到内部缓冲区后立即返回）
     *        发送完成后调用 tx_cplt_cb
     */
    uart_status_t uart_send_async(uart_handle_t *huart, const uint8_t *pdata, size_t len);

    /**
     * @brief 异步接收指定长度（数据存入用户缓冲区）
     *        接收完成后调用 rx_cplt_cb，参数 len 为实际接收长度
     */
    uart_status_t uart_receive_async(uart_handle_t *huart, uint8_t *pdata, size_t len);

#endif /* UART_CFG_ASYNC_ENABLE */

    /* ---- 不定长接收（空闲中断） ---- */

    /**
     * @brief 启动空闲中断接收模式
     *        接收到数据后自动存入内部环形缓冲区，空闲中断触发后调用 rx_cplt_cb
     * @param buffer 当空闲发生时，用户提供的存放数据的缓冲区（可为 NULL，需从内部缓冲拷贝）
     * @param max_len 最大期望长度
     * @note  若 buffer == NULL，则回调时用户需通过 uart_get_rx_count 获取长度并自行从内部缓冲读取
     */
    uart_status_t uart_start_rx_idle(uart_handle_t *huart, uint8_t *buffer, size_t max_len);

    /**
     * @brief 获取空闲中断模式已接收的字节数（位于内部环形缓冲区）
     */
    size_t uart_get_rx_count(uart_handle_t *huart);

    /* ---- 回调设置 ---- */
    void uart_set_tx_cplt_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *));
    void uart_set_rx_cplt_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *, size_t len));
    void uart_set_error_callback(uart_handle_t *huart, void (*cb)(uart_handle_t *, uart_error_t err));

    /* ---- 杂项 ---- */

    /**
     * @brief 等待发送完成（清空发送缓冲）并刷新
     */
    uart_status_t uart_flush(uart_handle_t *huart);

    /**
     * @brief 立即中止当前传输（发送和接收），回调错误
     */
    uart_status_t uart_abort(uart_handle_t *huart);

    /*==================== 核心层内部函数（中断内调用） ====================*/

    void uart_core_tx_isr(uart_handle_t *huart);
    void uart_core_rx_isr(uart_handle_t *huart);
    void uart_core_idle_isr(uart_handle_t *huart);
    void uart_core_error_isr(uart_handle_t *huart, uart_error_t err);

#ifdef __cplusplus
}
#endif

#endif /* UART_CORE_H__ */
