/**
 * @file    fifo_template.h
 * @brief   C 泛型 FIFO 模板（通过宏展开生成类型安全代码）
 * @note    用户需先定义 FIFO_NAME、FIFO_TYPE、FIFO_SIZE 再包含本文件。
 *          可选：FIFO_STORAGE 静态数组修饰符（如 static），FIFO_LOCK/UNLOCK。
 *
 * @example 创建一个 16 个 uint32_t 的 FIFO：
 *          #define FIFO_NAME  my_u32_fifo
 *          #define FIFO_TYPE  uint32_t
 *          #define FIFO_SIZE  16
 *          #include "fifo_template.h"
 *
 *          随后即可使用：my_u32_fifo_init() / my_u32_fifo_put() 等函数。
 *
 * @note    本模板不支持指针类型直接写入（若 FIFO_TYPE 为指针类型，存储指针值本身）。
 *          如需存储变长数据，可结合字节 FIFO 自行封装。
 */

/* 防止重复包含 */
#ifdef FIFO_NAME
#ifdef FIFO_TEMPLATE_INTERNAL
#undef FIFO_TEMPLATE_INTERNAL
#endif

/* 拼接标识符辅助宏 */
#define FIFO_CONCAT_(a, b) a##b
#define FIFO_CONCAT(a, b)  FIFO_CONCAT_(a, b)

/* 生成函数/变量名 */
#define FIFO_FUNC(name) FIFO_CONCAT(FIFO_NAME, name)
#define FIFO_VAR(name)  FIFO_CONCAT(FIFO_NAME, name)

/* 默认静态内联数组 */
#ifndef FIFO_STORAGE
#define FIFO_STORAGE static
#endif

/* 临界区默认无操作 */
#ifndef FIFO_LOCK
#define FIFO_LOCK()
#endif
#ifndef FIFO_UNLOCK
#define FIFO_UNLOCK()
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 静态存储缓冲区 */
FIFO_STORAGE FIFO_TYPE FIFO_VAR(_buffer)[FIFO_SIZE];

/* 控制结构 */
typedef struct
{
    FIFO_TYPE *buffer; /* 指向 _buffer */
    size_t     size;   /* FIFO_SIZE    */
    size_t     head;
    size_t     tail;
    bool       is_full;
} FIFO_CONCAT(FIFO_NAME, _t);

static FIFO_CONCAT(FIFO_NAME, _t) FIFO_VAR(_fifo);

/**
 * @brief 初始化 FIFO
 */
static void FIFO_FUNC(_init)(void)
{
    FIFO_VAR(_fifo).buffer  = FIFO_VAR(_buffer);
    FIFO_VAR(_fifo).size    = FIFO_SIZE;
    FIFO_VAR(_fifo).head    = 0;
    FIFO_VAR(_fifo).tail    = 0;
    FIFO_VAR(_fifo).is_full = false;
}

/**
 * @brief 写入一个元素（满时返回 false）
 * @param item 待写入元素
 * @return     true 成功
 */
static bool FIFO_FUNC(_put)(FIFO_TYPE item)
{
    FIFO_TYPE *const buf  = FIFO_VAR(_fifo).buffer;
    const size_t     size = FIFO_VAR(_fifo).size;
    size_t           tail = FIFO_VAR(_fifo).tail;

    FIFO_LOCK();
    if (FIFO_VAR(_fifo).is_full)
    {
        FIFO_UNLOCK();
        return false;
    }

    buf[tail]            = item;
    tail                 = (tail + 1) % size;
    FIFO_VAR(_fifo).tail = tail;

    if (tail == FIFO_VAR(_fifo).head)
    {
        FIFO_VAR(_fifo).is_full = true;
    }
    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 读取一个元素（空时返回 false）
 * @param item 读取数据保存地址
 * @return     true 成功
 */
static bool FIFO_FUNC(_get)(FIFO_TYPE *item)
{
    if (item == NULL)
        return false;

    FIFO_TYPE *const buf  = FIFO_VAR(_fifo).buffer;
    const size_t     size = FIFO_VAR(_fifo).size;
    size_t           head = FIFO_VAR(_fifo).head;

    FIFO_LOCK();
    if (FIFO_FUNC(_is_empty)())
    {
        FIFO_UNLOCK();
        return false;
    }

    *item                   = buf[head];
    head                    = (head + 1) % size;
    FIFO_VAR(_fifo).head    = head;
    FIFO_VAR(_fifo).is_full = false;
    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 查看队首元素但不取出
 */
static bool FIFO_FUNC(_peek)(FIFO_TYPE *item)
{
    if (item == NULL)
        return false;

    FIFO_LOCK();
    if (FIFO_FUNC(_is_empty)())
    {
        FIFO_UNLOCK();
        return false;
    }

    *item = FIFO_VAR(_fifo).buffer[FIFO_VAR(_fifo).head];
    FIFO_UNLOCK();
    return true;
}

/**
 * @brief 清空 FIFO
 */
static void FIFO_FUNC(_flush)(void)
{
    FIFO_LOCK();
    FIFO_VAR(_fifo).head    = 0;
    FIFO_VAR(_fifo).tail    = 0;
    FIFO_VAR(_fifo).is_full = false;
    FIFO_UNLOCK();
}

/**
 * @brief 是否为空
 */
static bool FIFO_FUNC(_is_empty)(void)
{
    return (FIFO_VAR(_fifo).head == FIFO_VAR(_fifo).tail) && !FIFO_VAR(_fifo).is_full;
}

/**
 * @brief 是否已满
 */
static bool FIFO_FUNC(_is_full)(void)
{
    return FIFO_VAR(_fifo).is_full;
}

/**
 * @brief 已存储元素个数
 */
static size_t FIFO_FUNC(_count)(void)
{
    size_t       count;
    const size_t head = FIFO_VAR(_fifo).head;
    const size_t tail = FIFO_VAR(_fifo).tail;

    FIFO_LOCK();
    if (FIFO_VAR(_fifo).is_full)
    {
        count = FIFO_VAR(_fifo).size;
    }
    else if (tail >= head)
    {
        count = tail - head;
    }
    else
    {
        count = FIFO_VAR(_fifo).size - head + tail;
    }
    FIFO_UNLOCK();
    return count;
}

/**
 * @brief 剩余可写元素个数
 */
static size_t FIFO_FUNC(_free)(void)
{
    return FIFO_VAR(_fifo).size - FIFO_FUNC(_count)();
}

/**
 * @brief 获取 FIFO 控制结构指针（高级用法）
 */
static FIFO_CONCAT(FIFO_NAME, _t) * FIFO_FUNC(_handle)(void)
{
    return &FIFO_VAR(_fifo);
}

/* 清理宏，防止影响后续包含 */
#undef FIFO_NAME
#undef FIFO_TYPE
#undef FIFO_SIZE
#ifdef FIFO_STORAGE_USER
#undef FIFO_STORAGE
#endif

#else
/* 标记首次包含已生成内部函数 */
#define FIFO_TEMPLATE_INTERNAL
#endif /* FIFO_NAME */
