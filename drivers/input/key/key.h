#ifndef __KEY_H__
#define __KEY_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* 按键事件类型 */
    typedef enum
    {
        KEY_EVENT_NONE       = 0,
        KEY_EVENT_PRESSED    = 1, /* 按键按下（消抖确认） */
        KEY_EVENT_RELEASED   = 2, /* 按键释放 */
        KEY_EVENT_LONG_PRESS = 3, /* 长按（可配置时间） */
        KEY_EVENT_CLICK      = 4  /* 点击（可扩展双击等） */
    } KeyEvent_t;

    /* 单个按键的硬件配置 */
    typedef struct
    {
        void    *port;          /* GPIO端口，如 GPIOC，可抽象为 void* */
        uint16_t pin;           /* 引脚号，如 GPIO_PIN_0 */
        uint8_t  active_level;  /* 按下时的有效电平，0或1 */
        uint16_t debounce_ms;   /* 消抖时间（毫秒） */
        uint16_t long_press_ms; /* 长按判定时间（毫秒），0表示禁用长按检测 */
    } KeyHardwareConfig_t;

    /* 内部状态（用户无需直接操作） */
    typedef struct
    {
        uint8_t    state;         /* 当前状态机状态 */
        uint32_t   last_tick;     /* 上次电平变化时刻（毫秒） */
        uint8_t    last_stable;   /* 消抖后的稳定电平 */
        bool       event_pending; /* 是否有未处理的事件 */
        KeyEvent_t event;         /* 待处理的事件类型 */
        bool       long_pressed;  /* 是否已触发长按 */
    } KeyState_t;

    /* 初始化按键模块，传入配置数组和按键数量 */
    void Key_Init(const KeyHardwareConfig_t *configs, uint8_t count);

    /*
     * 扫描函数，需在主循环或定时中断中周期性调用（建议每 5~10ms 一次）
     * 非阻塞，内部使用系统时基 HAL_GetTick()
     */
    void Key_Scan(void);

    /*
     * 获取并清除一个按键事件（FIFO，防止事件丢失）
     * 返回 true 表示获取到有效事件，通过参数传出按键ID和事件类型
     * 返回 false 表示无事件
     */
    bool Key_GetEvent(uint8_t *key_id, KeyEvent_t *event);

    /*
     * 简化接口：仅获取按下的按键ID（单击），适合替换原 Key_Scan
     * 返回 0 表示无按键按下，否则返回按键编号（从1开始）
     */
    uint8_t Key_GetPressedKey(void);
    uint8_t Key_GetLongPressedKey(void); // 获取长按的按键ID，返回0表示无长按

    // 使用封装1：直接查询按键状态，适合简单场景
    bool Key_IsPressed(uint8_t key_id);     // 返回 true 表示该键正在被按下（消抖后）
    bool Key_IsLongPressed(uint8_t key_id); // 返回 true 表示该键处于长按状态

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */
