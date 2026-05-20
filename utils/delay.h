#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================
 * 延时模式选择 (可按需强制指定)
 *   DELAY_MODE_DWT    1 : 强制使用 DWT (仅 M3/M4/M7/M33 等)
 *   DELAY_MODE_SYSTICK 2: 强制使用 SysTick 轮询
 *   DELAY_MODE_CYCLE  3 : 强制纯软件循环 (仅后备)
 * 若未定义 DELAY_MODE，则根据 __CORTEX_M 自动选择最佳方案。
 *===========================================================================*/
#define DELAY_MODE_DWT     1
#define DELAY_MODE_SYSTICK 2
#define DELAY_MODE_CYCLE   3

    /*===========================================================================
     * 初始化 (必须在使用任何延时/阻塞函数前调用一次)
     * 调用时机：系统时钟配置完成、SystemCoreClock 赋值之后
     *===========================================================================*/
    void delay_init(void);

    /*===========================================================================
     * 基础精确延时 (阻塞)
     *===========================================================================*/
    void delay_us(uint32_t nus); // 微秒延时 (0 ~ 2^32-1)
    void delay_ms(uint32_t nms); // 毫秒延时 (0 ~ 2^32-1)
    void delay_s(uint32_t ns);   // 秒级延时 (0 ~ 2^32-1)

    /*===========================================================================
     * 条件等待 (带超时) - 等待某个条件变为真
     *===========================================================================*/

    /**
     * @brief  阻塞等待条件成立，带微秒超时
     * @param  cond       : 条件函数，返回 true 表示成立
     * @param  timeout_us : 超时微秒数
     * @retval true  条件在超时前成立
     * @retval false 超时
     */
    bool delay_wait_until_us(bool (*cond)(void), uint32_t timeout_us);

    /**
     * @brief  阻塞等待条件成立，带毫秒超时
     * @param  cond       : 条件函数
     * @param  timeout_ms : 超时毫秒数
     */
    bool delay_wait_until_ms(bool (*cond)(void), uint32_t timeout_ms);

    /**
     * @brief  阻塞等待条件变为假 (即 cond() 返回 false)，带微秒超时
     * @param  cond       : 条件函数
     * @param  timeout_us : 超时微秒数
     * @retval true  条件在超时前变为假
     * @retval false 超时
     */
    bool delay_wait_while_us(bool (*cond)(void), uint32_t timeout_us);

    /**
     * @brief  阻塞等待条件变为假，带毫秒超时
     */
    bool delay_wait_while_ms(bool (*cond)(void), uint32_t timeout_ms);

    /*===========================================================================
     * 标志位等待 (带超时) - 常用于等待外设状态、通信完成等
     *===========================================================================*/

    /**
     * @brief  等待一个 volatile uint32_t 变量等于指定值，带微秒超时
     * @param  flag   : 指向标志变量的指针
     * @param  value  : 期望的值
     * @param  timeout_us : 超时微秒数
     * @retval true  标志等于指定值
     * @retval false 超时
     */
    bool delay_wait_flag_us(volatile uint32_t *flag, uint32_t value, uint32_t timeout_us);

    /**
     * @brief  等待标志位等于指定值，带毫秒超时
     */
    bool delay_wait_flag_ms(volatile uint32_t *flag, uint32_t value, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
