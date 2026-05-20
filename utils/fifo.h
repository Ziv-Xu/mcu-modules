/**
 * @file    fifo.h
 * @brief   轻量级字节型环形缓冲区 FIFO
 * @note    静态内存分配，中断安全（可配置临界区）
 */

#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* 用户可根据平台重定义临界区保护宏，默认空（非中断安全） */
#ifndef FIFO_LOCK
#define FIFO_LOCK()
#endif

#ifndef FIFO_UNLOCK
#define FIFO_UNLOCK()
#endif

    /**
     * @brief FIFO 控制句柄
     */
    typedef struct
    {
        uint8_t *buffer;  /* 缓冲区指针（用户提供） */
        size_t   size;    /* 缓冲区总大小（字节）  */
        size_t   head;    /* 读索引               */
        size_t   tail;    /* 写索引               */
        bool     is_full; /* 满标志（区分空/满）    */
    } fifo_t;

    /**
     * @brief 初始化 FIFO
     * @param fifo   FIFO 实例指针
     * @param buffer 用户分配的缓冲区
     * @param size   缓冲区大小（字节数）
     */
    void fifo_init(fifo_t *fifo, uint8_t *buffer, size_t size);

    /**
     * @brief 写入一个字节（非阻塞，满时返回 false）
     * @param fifo FIFO 实例指针
     * @param byte 待写入字节
     * @return     true 写入成功，false 已满
     */
    bool fifo_put(fifo_t *fifo, uint8_t byte);

    /**
     * @brief 读取一个字节（非阻塞，空时返回 false）
     * @param fifo FIFO 实例指针
     * @param byte 读出字节保存地址
     * @return     true 读取成功，false 已空
     */
    bool fifo_get(fifo_t *fifo, uint8_t *byte);

    /**
     * @brief 查看下一个字节但不取出
     * @param fifo FIFO 实例指针
     * @param byte 读出字节保存地址
     * @return     true 成功，false 已空
     */
    bool fifo_peek(const fifo_t *fifo, uint8_t *byte);

    /**
     * @brief 清空 FIFO
     * @param fifo FIFO 实例指针
     */
    void fifo_flush(fifo_t *fifo);

    /**
     * @brief 判断 FIFO 是否为空
     * @param fifo FIFO 实例指针
     * @return     true 为空
     */
    bool fifo_is_empty(const fifo_t *fifo);

    /**
     * @brief 判断 FIFO 是否已满
     * @param fifo FIFO 实例指针
     * @return     true 已满
     */
    bool fifo_is_full(const fifo_t *fifo);

    /**
     * @brief 获取当前已存储字节数
     * @param fifo FIFO 实例指针
     * @return     已用字节数
     */
    size_t fifo_count(const fifo_t *fifo);

    /**
     * @brief 获取剩余可写入字节数
     * @param fifo FIFO 实例指针
     * @return     空闲字节数
     */
    size_t fifo_free(const fifo_t *fifo);

#ifdef __cplusplus
}
#endif

#endif /* FIFO_H */
