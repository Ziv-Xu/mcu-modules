/******************************************************************************
 * @file    delay.c
 * @brief   高可移植 Cortex-M 阻塞延时与条件等待完整模块
 * @note    仅依赖 SystemCoreClock 变量 (CMSIS 标准)
 *          自动选择 DWT / SysTick 轮询 / 软件循环
 ******************************************************************************/

#include "delay.h"

/* ----------------------------- 外部依赖 ----------------------------- */
extern uint32_t SystemCoreClock;

/* ----------------- 编译器适配 (用于纯软件循环的 __NOP) ---------------- */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define DELAY_NOP() __nop()
#elif defined(__GNUC__)
#define DELAY_NOP() __asm volatile("nop")
#elif defined(__ICCARM__)
#define DELAY_NOP() __no_operation()
#else
#define DELAY_NOP() ((void) 0)
#endif

/* ------------------ 自动选择延时模式 ----------------- */
#if !defined(DELAY_MODE)
#if defined(__CORTEX_M)
#if (__CORTEX_M >= 3U)
#define DELAY_MODE DELAY_MODE_DWT // M3/M4/M7/M33 拥有 DWT
#else
#define DELAY_MODE DELAY_MODE_SYSTICK // M0/M0+ 使用 SysTick 轮询
#endif
#else
#define DELAY_MODE DELAY_MODE_SYSTICK // 未知内核假设为 Cortex-M
#endif
#endif

/* ---------------- 内核外设寄存器自声明 (纯地址，不依赖器件头文件) ---------------- */
#define SysTick_BASE   (0xE000E010UL)
#define DWT_BASE       (0xE0001000UL)
#define CoreDebug_BASE (0xE000EDF0UL)

typedef struct
{
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

typedef struct
{
    volatile uint32_t CTRL;
    volatile uint32_t CYCCNT;
    volatile uint32_t CPICNT;
    volatile uint32_t EXCCNT;
    volatile uint32_t SLEEPCNT;
    volatile uint32_t LSUCNT;
    volatile uint32_t FOLDCNT;
    volatile uint32_t PCSR;
} DWT_Type;

typedef struct
{
    volatile uint32_t DHCSR;
    volatile uint32_t DCRSR;
    volatile uint32_t DCRDR;
    volatile uint32_t DEMCR;
} CoreDebug_Type;

#define SysTick   ((SysTick_Type *) SysTick_BASE)
#define DWT       ((DWT_Type *) DWT_BASE)
#define CoreDebug ((CoreDebug_Type *) CoreDebug_BASE)

/* 寄存器控制位 */
#define SysTick_CTRL_ENABLE_Msk    (1UL << 0)
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2)
#define SysTick_CTRL_COUNTFLAG_Msk (1UL << 16)
#define SysTick_LOAD_MAX           0xFFFFFFUL

#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk     (1UL << 0)

/* ----------------- 内部辅助：获取当前计时器计数值 (用于超时检测) ----------------- */
static inline uint32_t delay_get_ticks(void)
{
#if DELAY_MODE == DELAY_MODE_DWT
    return DWT->CYCCNT;
#elif DELAY_MODE == DELAY_MODE_SYSTICK
    // 返回 SysTick 当前值 (递减计数，倒计时)，用于计算已流逝时间
    return SysTick->VAL;
#else
    return 0; // 软件循环模式不支持精确超时检测，外部将通过循环近似
#endif
}

static inline uint32_t delay_ticks_per_us(void)
{
#if DELAY_MODE == DELAY_MODE_DWT
    return SystemCoreClock / 1000000UL;
#elif DELAY_MODE == DELAY_MODE_SYSTICK
    return SystemCoreClock / 1000000UL;
#else
    return 1; // 软件循环近似
#endif
}

/* 使用 SysTick 完成一段精确的微秒延时 (内部复用) */
static void delay_systick_us(uint32_t us)
{
    uint32_t ticks_per_us = SystemCoreClock / 1000000UL;
    while (us > 0)
    {
        uint32_t max_us = SysTick_LOAD_MAX / ticks_per_us;
        uint32_t step   = (us > max_us) ? max_us : us;
        uint32_t ticks  = step * ticks_per_us;
        if (ticks == 0)
            ticks = 1;

        SysTick->LOAD = ticks - 1;
        SysTick->VAL  = 0;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
        {
            ;
        }
        SysTick->CTRL = 0;
        us -= step;
    }
}

/* ------------------------- 基础延时实现 ------------------------- */
void delay_us(uint32_t nus)
{
#if DELAY_MODE == DELAY_MODE_DWT
    if (SystemCoreClock == 0)
        return;
    uint32_t cycles = (uint32_t) (((uint64_t) nus * SystemCoreClock) / 1000000UL);
    uint32_t start  = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
        ;
    }

#elif DELAY_MODE == DELAY_MODE_SYSTICK
    if (SystemCoreClock == 0)
        return;
    delay_systick_us(nus);

#elif DELAY_MODE == DELAY_MODE_CYCLE
    if (SystemCoreClock == 0)
        return;
    volatile uint32_t cnt = nus * (SystemCoreClock / 4000000UL);
    while (cnt--)
    {
        DELAY_NOP();
    }
#endif
}

void delay_ms(uint32_t nms)
{
#if DELAY_MODE == DELAY_MODE_DWT
    if (SystemCoreClock == 0)
        return;
    while (nms > 0)
    {
        uint32_t step   = (nms > 800UL) ? 800UL : nms; // 防止乘法溢出
        uint32_t cycles = (uint32_t) (((uint64_t) step * SystemCoreClock) / 1000UL);
        uint32_t start  = DWT->CYCCNT;
        while ((DWT->CYCCNT - start) < cycles)
        {
            ;
        }
        nms -= step;
    }

#elif DELAY_MODE == DELAY_MODE_SYSTICK
    // 毫秒通过多次微秒实现，避免 SysTick 24 位限制
    while (nms > 0)
    {
        uint32_t step = (nms > 1000UL) ? 1000UL : nms; // 每次最多 1000ms
        delay_us(step * 1000UL);
        nms -= step;
    }

#elif DELAY_MODE == DELAY_MODE_CYCLE
    delay_us(nms * 1000UL);
#endif
}

void delay_s(uint32_t ns)
{
    while (ns > 0)
    {
        uint32_t step = (ns > 1000UL) ? 1000UL : ns; // 单次最多 1000 秒
        delay_ms(step * 1000UL);
        ns -= step;
    }
}

/* ------------------------- 条件等待实现 (核心) ------------------------- */

/**
 * @brief  通用超时等待条件函数 (微秒精度)
 * @note   在 DWT 模式下精准，SysTick 模式通过分段轮询实现，会有微小误差
 */
static bool delay_wait_condition_us(bool (*cond)(void), uint32_t timeout_us, bool wait_true)
{
    if (cond == NULL)
        return false;
    if (timeout_us == 0)
    {
        // 超时为 0，仅检查一次当前状态
        return (cond() == wait_true);
    }

#if DELAY_MODE == DELAY_MODE_DWT
    if (SystemCoreClock == 0)
        return false;
    uint32_t ticks_per_us = SystemCoreClock / 1000000UL;
    uint32_t cycles       = timeout_us * ticks_per_us;
    uint32_t start        = DWT->CYCCNT;

    do
    {
        if (cond() == wait_true)
        {
            return true;
        }
    } while ((DWT->CYCCNT - start) < cycles);
    return false;

#elif DELAY_MODE == DELAY_MODE_SYSTICK
    if (SystemCoreClock == 0)
        return false;
    // 使用分段微秒延时作为轮询时钟，粗略超时
    uint32_t       elapsed       = 0;
    const uint32_t poll_interval = 100; // 每 100us 检查一次条件

    while (elapsed < timeout_us)
    {
        if (cond() == wait_true)
        {
            return true;
        }
        uint32_t step = (timeout_us - elapsed > poll_interval) ? poll_interval : (timeout_us - elapsed);
        delay_systick_us(step);
        elapsed += step;
    }
    // 最后一次机会检查
    return (cond() == wait_true);

#elif DELAY_MODE == DELAY_MODE_CYCLE
    // 纯软件循环：使用粗略延时等待
    if (SystemCoreClock == 0)
        return false;
    uint32_t step_us   = 1000; // 每 1ms 检查一次
    uint32_t remaining = timeout_us;
    while (remaining > 0)
    {
        if (cond() == wait_true)
            return true;
        uint32_t us_now = (remaining > step_us) ? step_us : remaining;
        delay_us(us_now);
        remaining -= us_now;
    }
    return (cond() == wait_true);
#else
    return false;
#endif
}

/* 对外接口 */
bool delay_wait_until_us(bool (*cond)(void), uint32_t timeout_us)
{
    return delay_wait_condition_us(cond, timeout_us, true);
}

bool delay_wait_until_ms(bool (*cond)(void), uint32_t timeout_ms)
{
    // 将 ms 转换为 us，但需注意乘法溢出，分段处理
    uint32_t remaining = timeout_ms;
    while (remaining > 0)
    {
        uint32_t chunk = (remaining > 1000UL) ? 1000UL : remaining;
        if (delay_wait_until_us(cond, chunk * 1000UL))
        {
            return true;
        }
        remaining -= chunk;
    }
    return false; // 完全超时
}

bool delay_wait_while_us(bool (*cond)(void), uint32_t timeout_us)
{
    return delay_wait_condition_us(cond, timeout_us, false);
}

bool delay_wait_while_ms(bool (*cond)(void), uint32_t timeout_ms)
{
    uint32_t remaining = timeout_ms;
    while (remaining > 0)
    {
        uint32_t chunk = (remaining > 1000UL) ? 1000UL : remaining;
        if (delay_wait_while_us(cond, chunk * 1000UL))
        {
            return true;
        }
        remaining -= chunk;
    }
    return false;
}

/* ------------------------- 标志位等待实现 ------------------------- */
static bool delay_wait_flag_condition_us(volatile uint32_t *flag, uint32_t value, uint32_t timeout_us, bool wait_equal)
{
    if (flag == NULL)
        return false;

#if DELAY_MODE == DELAY_MODE_DWT
    if (SystemCoreClock == 0)
        return false;
    uint32_t ticks_per_us = SystemCoreClock / 1000000UL;
    uint32_t cycles       = timeout_us * ticks_per_us;
    uint32_t start        = DWT->CYCCNT;
    do
    {
        if (wait_equal ? (*flag == value) : (*flag != value))
        {
            return true;
        }
    } while ((DWT->CYCCNT - start) < cycles);
    return false;

#elif DELAY_MODE == DELAY_MODE_SYSTICK
    if (SystemCoreClock == 0)
        return false;
    uint32_t       elapsed       = 0;
    const uint32_t poll_interval = 50; // 50us 轮询一次
    while (elapsed < timeout_us)
    {
        if (wait_equal ? (*flag == value) : (*flag != value))
        {
            return true;
        }
        uint32_t step = (timeout_us - elapsed > poll_interval) ? poll_interval : (timeout_us - elapsed);
        delay_systick_us(step);
        elapsed += step;
    }
    return wait_equal ? (*flag == value) : (*flag != value);

#elif DELAY_MODE == DELAY_MODE_CYCLE
    if (SystemCoreClock == 0)
        return false;
    uint32_t step_us   = 1000;
    uint32_t remaining = timeout_us;
    while (remaining > 0)
    {
        if (wait_equal ? (*flag == value) : (*flag != value))
            return true;
        uint32_t us_now = (remaining > step_us) ? step_us : remaining;
        delay_us(us_now);
        remaining -= us_now;
    }
    return wait_equal ? (*flag == value) : (*flag != value);
#else
    return false;
#endif
}

bool delay_wait_flag_us(volatile uint32_t *flag, uint32_t value, uint32_t timeout_us)
{
    return delay_wait_flag_condition_us(flag, value, timeout_us, true);
}

bool delay_wait_flag_ms(volatile uint32_t *flag, uint32_t value, uint32_t timeout_ms)
{
    uint32_t remaining = timeout_ms;
    while (remaining > 0)
    {
        uint32_t chunk = (remaining > 1000UL) ? 1000UL : remaining;
        if (delay_wait_flag_us(flag, value, chunk * 1000UL))
        {
            return true;
        }
        remaining -= chunk;
    }
    return false;
}

/* ------------------------- 初始化函数 ------------------------- */
void delay_init(void)
{
#if DELAY_MODE == DELAY_MODE_DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

#elif DELAY_MODE == DELAY_MODE_SYSTICK
    // 默认配置即可，无需特殊操作

#elif DELAY_MODE == DELAY_MODE_CYCLE
    if (SystemCoreClock == 0)
    {
        while (1)
        {
            ;
        } // 严重错误：时钟未就绪
    }
#endif
}
