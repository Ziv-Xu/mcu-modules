/**
 * @file    fifo.c
 * @brief   字节型 FIFO 实现
 */

#include "fifo.h"
#include <string.h>

/**
 * @brief 初始化 FIFO
 */
void fifo_init(fifo_t *fifo, uint8_t *buffer, size_t size)
{
    if (fifo == NULL || buffer == NULL || size == 0)
    {
        return; /* 无效参数静默忽略，实际项目可加入断言 */
    }
    fifo->buffer  = buffer;
    fifo->size    = size;
    fifo->head    = 0;
    fifo->tail    = 0;
    fifo->is_full = false;
}

/**
 * @brief 写入一个字节（中断安全）
 */
bool fifo_put(fifo_t *fifo, uint8_t byte)
{
    if (fifo == NULL)
    {
        return false;
    }

    FIFO_LOCK();
    if (fifo->is_full)
    {
        FIFO_UNLOCK();
        return false;
    }

    fifo->buffer[fifo->tail] = byte;
    fifo->tail               = (fifo->tail + 1) % fifo->size;

    /* 判断写指针追上读指针则置满 */
    if (fifo->tail == fifo->head)
    {
        fifo->is_full = true;
    }
    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 读取一个字节（中断安全）
 */
bool fifo_get(fifo_t *fifo, uint8_t *byte)
{
    if (fifo == NULL || byte == NULL)
    {
        return false;
    }

    FIFO_LOCK();
    if (fifo_is_empty(fifo))
    {
        FIFO_UNLOCK();
        return false;
    }

    *byte         = fifo->buffer[fifo->head];
    fifo->head    = (fifo->head + 1) % fifo->size;
    fifo->is_full = false; /* 读取后一定不满 */

    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 查看下一个字节（不修改读索引）
 */
bool fifo_peek(const fifo_t *fifo, uint8_t *byte)
{
    if (fifo == NULL || byte == NULL)
    {
        return false;
    }

    FIFO_LOCK();
    if (fifo_is_empty(fifo))
    {
        FIFO_UNLOCK();
        return false;
    }

    *byte = fifo->buffer[fifo->head];
    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 清空 FIFO
 */
void fifo_flush(fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return;
    }

    FIFO_LOCK();
    fifo->head    = 0;
    fifo->tail    = 0;
    fifo->is_full = false;
    FIFO_UNLOCK();
}

/**
 * @brief 判断是否为空
 */
bool fifo_is_empty(const fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return true; /* 无效句柄视为空 */
    }
    return (fifo->head == fifo->tail) && !fifo->is_full;
}

/**
 * @brief 判断是否已满
 */
bool fifo_is_full(const fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return false;
    }
    return fifo->is_full;
}

/**
 * @brief 已存储字节数
 */
size_t fifo_count(const fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return 0;
    }

    FIFO_LOCK();
    size_t count;
    if (fifo->is_full)
    {
        count = fifo->size;
    }
    else if (fifo->tail >= fifo->head)
    {
        count = fifo->tail - fifo->head;
    }
    else
    {
        count = fifo->size - fifo->head + fifo->tail;
    }
    FIFO_UNLOCK();
    return count;
}

/**
 * @brief 剩余空间字节数
 */
size_t fifo_free(const fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return 0;
    }
    return fifo->size - fifo_count(fifo);
}
