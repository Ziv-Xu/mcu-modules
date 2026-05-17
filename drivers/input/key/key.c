#include "button.h"
#include <string.h> // memset

/* 使用 HAL 库读取引脚，保证可移植性只需修改这两个宏 */
#ifndef BUTTON_READ_PIN
#define BUTTON_READ_PIN(port, pin) HAL_GPIO_ReadPin((GPIO_TypeDef *) port, pin)
#endif

#ifndef BUTTON_GET_TICK
#define BUTTON_GET_TICK() HAL_GetTick()
#endif

/* 最大支持按键数量，可根据需要调整 */
#define BUTTON_MAX_COUNT 16
/* 事件队列深度 */
#define BUTTON_EVENT_QUEUE_SIZE 8

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
static ButtonState_t          button_states[BUTTON_MAX_COUNT];
static ButtonHardwareConfig_t button_configs[BUTTON_MAX_COUNT];
static uint8_t                button_count = 0;

/* 事件环形队列 */
static struct
{
    uint8_t       key_id;
    ButtonEvent_t event;
} event_queue[BUTTON_EVENT_QUEUE_SIZE];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;

/* 内部函数：将事件加入队列 */
static inline bool EventQueue_Push(uint8_t key_id, ButtonEvent_t event)
{
    uint8_t next = (queue_head + 1) % BUTTON_EVENT_QUEUE_SIZE;
    if (next == queue_tail)
    {
        return false; /* 队列满，丢弃（实际可考虑记录丢失标志） */
    }
    event_queue[queue_head].key_id = key_id;
    event_queue[queue_head].event  = event;
    queue_head                     = next;
    return true;
}

/* 内部函数：从队列取出事件 */
static inline bool EventQueue_Pop(uint8_t *key_id, ButtonEvent_t *event)
{
    if (queue_head == queue_tail)
    {
        return false;
    }
    *key_id    = event_queue[queue_tail].key_id;
    *event     = event_queue[queue_tail].event;
    queue_tail = (queue_tail + 1) % BUTTON_EVENT_QUEUE_SIZE;
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
        button_configs[i] = configs[i];
        memset(&button_states[i], 0, sizeof(ButtonState_t));
    }
    button_count = count;
    queue_head = queue_tail = 0;
}

/* 扫描所有按键 */
void Button_Scan(void)
{
    uint32_t now = BUTTON_GET_TICK();

    for (uint8_t i = 0; i < button_count; i++)
    {
        const ButtonHardwareConfig_t *cfg = &button_configs[i];
        ButtonState_t                *st  = &button_states[i];

        /* 读取当前原始电平 */
        uint8_t raw_level = (BUTTON_READ_PIN(cfg->port, cfg->pin) == cfg->active_level) ? 1 : 0;

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
                    st->state = STATE_IDLE;
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
                        EventQueue_Push(i + 1, BUTTON_EVENT_PRESSED);
                    }
                    else
                    {
                        /* 释放，返回空闲 */
                        st->state = STATE_IDLE;
                        EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
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
                    EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
                    EventQueue_Push(i + 1, BUTTON_EVENT_CLICK); /* 点击事件 */
                }
                else if (cfg->long_press_ms > 0 && (now - st->last_tick) >= cfg->long_press_ms && !st->long_pressed)
                {
                    /* 达到长按时间 */
                    st->long_pressed = true;
                    st->state        = STATE_LONG_PRESS;
                    EventQueue_Push(i + 1, BUTTON_EVENT_LONG_PRESS);
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
                    EventQueue_Push(i + 1, BUTTON_EVENT_RELEASED);
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
    return EventQueue_Pop(key_id, event);
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
