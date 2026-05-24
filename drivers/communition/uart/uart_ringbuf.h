/**
 * @file    uart_ringbuf.h
 * @brief   高效的环形缓冲区实现
 * @author  Ziv-Xu
 * @date    2026-05-22
 * @copyright Copyright (c) 2026, All rights reserved.
 *
 * @details 无锁设计，适用于单生产者-单消费者场景（如中断与主循环）。
 *          需在操作前后调用临界区保护（由核心层负责）。
 *
 * 修改历史:
 *   - 2026-05-22: 初始版本
 */

#ifndef UART_RINGBUF_H__
#define UART_RINGBUF_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 环形缓冲区控制块
     */
    typedef struct
    {
        uint8_t *buffer; /**< 缓冲区存储空间指针（由用户提供） */
        size_t   size;   /**< 缓冲区总大小（字节） */
        size_t   head;   /**< 读索引（消费者） */
        size_t   tail;   /**< 写索引（生产者） */
        size_t   count;  /**< 当前有效数据量 */
    } ringbuf_t;

    /**
     * @brief 初始化环形缓冲区
     * @param rb      环形缓冲区实例指针
     * @param buffer  数据存储数组
     * @param size    数组大小（必须为 2 的幂，以便高效取模，本实现使用通用取模）
     */
    void ringbuf_init(ringbuf_t *rb, uint8_t *buffer, size_t size);

    /**
     * @brief 向缓冲区写入一个字节
     * @return true 成功，false 缓冲区满
     */
    bool ringbuf_put(ringbuf_t *rb, uint8_t data);

    /**
     * @brief 从缓冲区读取一个字节
     * @param data  输出数据指针
     * @return true 成功，false 缓冲区空
     */
    bool ringbuf_get(ringbuf_t *rb, uint8_t *data);

    /**
     * @brief 查看缓冲区中指定偏移的字节（不取出）
     * @param offset  偏移量（0 指向第一个可读字节）
     * @param data    输出数据指针
     * @return true 成功，false offset 无效
     */
    bool ringbuf_peek(const ringbuf_t *rb, size_t offset, uint8_t *data);

    /**
     * @brief 获取当前可读字节数
     */
    size_t ringbuf_available(const ringbuf_t *rb);

    /**
     * @brief 获取当前空闲空间
     */
    size_t ringbuf_free(const ringbuf_t *rb);

    /**
     * @brief 丢弃指定数量的字节（从头部移除）
     * @param count 要丢弃的字节数
     * @return 实际丢弃的字节数
     */
    size_t ringbuf_skip(ringbuf_t *rb, size_t count);

    /**
     * @brief 清空缓冲区
     */
    void ringbuf_flush(ringbuf_t *rb);

    /**
     * @brief 检查缓冲区是否为空
     */
    bool ringbuf_is_empty(const ringbuf_t *rb);

    /**
     * @brief 检查缓冲区是否已满
     */
    bool ringbuf_is_full(const ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* UART_RINGBUF_H__ */
