#include "button.h"
#include <string.h> // memset

/* 使用 HAL 库读取引脚，保证可移植性只需修改这两个宏 */
#ifndef BUTTON_READ_PIN
#define BUTTON_READ_PIN(port, pin) HAL_GPIO_ReadPin((GPIO_TypeDef *) port, pin)
#endif

#ifndef BUTTON_GET_TICK
#define BUTTON_GET_TICK() HAL_GetTick()
#endif

#define BUTTON_MAX_COUNT        16 // 最大支持按键数量
#define BUTTON_EVENT_QUEUE_SIZE 8  // 事件队列深度,但是是加上只能存入7个是队列，因为后面的函数s_EventQueue_Push会丢弃一个事件以区分满和空

/* 状态机定义 */
enum
{
    STATE_IDLE = 0,    /* 空闲，等待电平变化 */
    STATE_DEBOUNCE,    /* 消抖中 */
    STATE_PRESSED,     /* 已确认按下 */
    STATE_LONG_PRESS,  /* 长按状态 */
    STATE_WAIT_RELEASE /* 等待释放（用于触发释放事件） */
};

/* 全局数据 */
static ButtonState_t          s_button_states[BUTTON_MAX_COUNT];
static ButtonHardwareConfig_t s_button_configs[BUTTON_MAX_COUNT];
static uint8_t                s_button_count = 0;

/* 事件环形队列 */
static struct
{
    uint8_t       key_id;
    ButtonEvent_t event;
} s_event_queue[BUTTON_EVENT_QUEUE_SIZE];
static uint8_t s_queue_head = 0;
static uint8_t s_queue_tail = 0;

/* 内部函数：将事件加入队列 */
static inline bool s_EventQueue_Push(uint8_t key_id, ButtonEvent_t event)
{
    uint8_t next = (s_queue_head + 1) % BUTTON_EVENT_QUEUE_SIZE;
    if (next == s_queue_tail)
    {
        return false; /* 队列满，丢弃（实际可考虑记录丢失标志） */
    }
    s_event_queue[s_queue_head].key_id = key_id;
    s_event_queue[s_queue_head].event  = event;
    s_queue_head                       = next;
    return true;
}

/* 内部函数：从队列取出事件 */
static inline bool s_EventQueue_Pop(uint8_t *key_id, ButtonEvent_t *event)
{
    if (s_queue_head == s_queue_tail)
    {
        return false;
    }
    *key_id      = s_event_queue[s_queue_tail].key_id;
    *event       = s_event_queue[s_queue_tail].event;
    s_queue_tail = (s_queue_tail + 1) % BUTTON_EVENT_QUEUE_SIZE;
    return true;
}

/* 初始化 */
void Button_Init(const ButtonHardwareConfig_t *configs, uint8_t count)
{
    if (configs == NULL || count == 0)
        return;
    if (count > BUTTON_MAX_COUNT)
        count = BUTTON_MAX_COUNT;

    for (uint8_t i = 0; i < count; i++)
    {
        s_button_configs[i] = configs[i];
        memset(&s_button_states[i], 0, sizeof(ButtonState_t));
    }
    s_button_count = count;
    s_queue_head = s_queue_tail = 0;
}

/* 扫描所有按键 */
void Button_Scan(void)
{
    uint32_t now = BUTTON_GET_TICK();

    for (uint8_t i = 0; i < s_button_count; i++)
    {
        const ButtonHardwareConfig_t *cfg = &s_button_configs[i];
        ButtonState_t                *st  = &s_button_states[i];

        /* 读取当前原始电平 */
        uint8_t raw_level = (BUTTON_READ_PIN(cfg->port, cfg->pin) == cfg->active_level) ? 1 : 0; // 1表示按下，0表示未按下

        switch (st->state)
        {
            case STATE_IDLE:
                if (raw_level != st->last_stable)
                {
                    /* 电平变化，进入消抖 */
                    st->state     = STATE_DEBOUNCE;
                    st->last_tick = now;
                }
                break;

            case STATE_DEBOUNCE:
                if (raw_level == st->last_stable)
                {
                    /* 电平恢复，返回空闲 */
                    st->state = STATE_IDLE; // 这里回到空闲，会重置时间戳，使比真实的消抖时间更长
                }
                else if ((now - st->last_tick) >= cfg->debounce_ms)
                {
                    /* 消抖时间到，确认新状态 */
                    st->last_stable = raw_level;
                    if (raw_level == 1)
                    {
                        /* 有效按下 */
                        st->state        = STATE_PRESSED;
                        st->last_tick    = now;
                        st->long_pressed = false;
                        s_EventQueue_Push(i + 1, BUTTON_EVENT_PRESSED);
                    }
                    else
                    {
                        /* 释放，返回空闲 */
                        st->state = STATE_IDLE;
                        s_EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
                    }
                }
                break;

            case STATE_PRESSED:
                if (raw_level == 0)
                {
                    /* 按键提前释放（未达到长按） */
                    st->last_tick = now;
                    st->state     = STATE_DEBOUNCE;
                    // 注意：这里跳转到消抖处理释放，可也产生释放事件
                    // 为了统一，直接转入等待释放或再次消抖，简化：直接产生释放事件并重置
                    st->last_stable = 0;
                    st->state       = STATE_IDLE;
                    s_EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
                    s_EventQueue_Push(i + 1, BUTTON_EVENT_CLICK); /* 点击事件 */
                }
                else if (cfg->long_press_ms > 0 && (now - st->last_tick) >= cfg->long_press_ms && !st->long_pressed) //! st->long_pressed用于防止重复触发长按事件
                {
                    /* 达到长按时间 */
                    st->long_pressed = true;
                    st->state        = STATE_LONG_PRESS;
                    s_EventQueue_Push(i + 1, BUTTON_EVENT_LONG_PRESS);
                }
                /* 否则继续保持按下状态 */
                break;

            case STATE_LONG_PRESS:
                /* 长按中，只需检测释放 */
                if (raw_level == 0)
                {
                    st->last_tick   = now;
                    st->last_stable = 0;
                    st->state       = STATE_IDLE;
                    s_EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
                }
                break;

            default:
                st->state = STATE_IDLE;
                break;
        }
    }
}

/* 获取并清除事件 */
bool Button_GetEvent(uint8_t *key_id, ButtonEvent_t *event)
{
    return s_EventQueue_Pop(key_id, event);
}

/* 简化获取按下按键ID（仅响应按下事件） */
uint8_t Button_GetPressedKey(void)
{
    uint8_t       key_id;
    ButtonEvent_t event;
    /* 遍历队列，寻找第一个按下事件并丢弃其他 */
    while (Button_GetEvent(&key_id, &event))
    {
        if (event == BUTTON_EVENT_PRESSED)
        {
            return key_id;
        }
        /* 否则丢弃（可缓存其他事件，此处简化处理） */
    }
    return 0;
}

uint8_t Button_GetLongPressedKey(void)
{
    uint8_t       key_id;
    ButtonEvent_t event;
    /* 遍历队列，寻找第一个按下事件并丢弃其他 */
    while (Button_GetEvent(&key_id, &event))
    {
        if (event == BUTTON_EVENT_LONG_PRESS)
        {
            return key_id;
        }
        /* 否则丢弃（可缓存其他事件，此处简化处理） */
    }
    return 0;
}

// 使用封装1：直接查询按键状态，适合简单场景
//  button.c 新增
bool Button_IsPressed(uint8_t key_id)
{
    if (key_id == 0 || key_id > s_button_count)
        return false;
    uint8_t idx = key_id - 1;
    return s_button_states[idx].last_stable == 1; // 稳定按下
}

bool Button_IsLongPressed(uint8_t key_id)
{
    if (key_id == 0 || key_id > s_button_count)
        return false;
    uint8_t idx = key_id - 1;
    return s_button_states[idx].state == STATE_LONG_PRESS;
}
