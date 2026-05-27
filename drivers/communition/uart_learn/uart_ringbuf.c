/**
 * @file    uart_ringbuf.c
 * @brief   环形缓冲区实现
 */

#include "uart_ringbuf.h"
#include <string.h>

void ringbuf_init(ringbuf_t *rb, uint8_t *buffer, size_t size)
{
    if (rb == NULL || buffer == NULL || size == 0)
        return;
    rb->buffer = buffer;
    rb->size   = size;
    rb->head   = 0;
    rb->tail   = 0;
    rb->count  = 0;
}

bool ringbuf_put(ringbuf_t *rb, uint8_t data)
{
    if (rb == NULL || ringbuf_is_full(rb))
        return false;
    rb->buffer[rb->tail] = data;
    rb->tail             = (rb->tail + 1) % rb->size;
    rb->count++;
    return true;
}

bool ringbuf_get(ringbuf_t *rb, uint8_t *data)
{
    if (rb == NULL || data == NULL || ringbuf_is_empty(rb))
        return false;
    *data    = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % rb->size;
    rb->count--;
    return true;
}

bool ringbuf_peek(const ringbuf_t *rb, size_t offset, uint8_t *data)
{
    if (rb == NULL || data == NULL || offset >= rb->count)
        return false;
    size_t idx = (rb->head + offset) % rb->size;
    *data      = rb->buffer[idx];
    return true;
}

size_t ringbuf_available(const ringbuf_t *rb)
{
    if (rb == NULL)
        return 0;
    return rb->count;
}

size_t ringbuf_free(const ringbuf_t *rb)
{
    if (rb == NULL)
        return 0;
    return rb->size - rb->count;
}

size_t ringbuf_skip(ringbuf_t *rb, size_t count)
{
    if (rb == NULL)
        return 0;
    if (count > rb->count)
        count = rb->count;
    rb->head = (rb->head + count) % rb->size;
    rb->count -= count;
    return count;
}

void ringbuf_flush(ringbuf_t *rb)
{
    if (rb == NULL)
        return;
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
}

bool ringbuf_is_empty(const ringbuf_t *rb)
{
    return (rb == NULL || rb->count == 0);
}

bool ringbuf_is_full(const ringbuf_t *rb)
{
    return (rb == NULL || rb->count == rb->size);
}
