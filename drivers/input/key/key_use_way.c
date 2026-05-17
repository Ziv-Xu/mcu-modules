/* 按键硬件配置表，任意增减 */
/* 定义在需要使用按键的文件中 */
static const ButtonHardwareConfig_t key_configs[] = {
    {GPIOC, GPIO_PIN_0, 0, 20, 1000}, /* KEY1: PC0, 低电平有效, 消抖20ms, 长按1s */
    {GPIOC, GPIO_PIN_1, 0, 20, 0   }, /* KEY2: PC1, 无长按检测 */
    {GPIOB, GPIO_PIN_5, 0, 15, 800 }, /* KEY3: PB5, 长按800ms */
    {GPIOB, GPIO_PIN_4, 0, 20, 0   }, /* KEY4 */
    /* 再增加一个按键只需添加一行，例如 */
    // { GPIOA, GPIO_PIN_3, 1, 30, 2000 },  /* KEY5: PA3, 高电平有效 */
};
#define KEY_COUNT (sizeof(key_configs) / sizeof(key_configs[0]))

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    // 初始化按键模块
    Button_Init(key_configs, KEY_COUNT);

    while (1)
    {
        // 每10ms扫描一次（可用定时中断触发更精确）
        static uint32_t last_scan = 0;
        if (HAL_GetTick() - last_scan >= 10)
        {
            last_scan = HAL_GetTick();
            Button_Scan();
        }

        // 获取事件方式1：查询并处理所有事件，先判断key或者evt都可以
        uint8_t       key;
        ButtonEvent_t evt;
        while (Button_GetEvent(&key, &evt))
        {
            if (evt == BUTTON_EVENT_PRESSED)
            {
                // 按键按下处理
            }
            else if (evt == BUTTON_EVENT_LONG_PRESS)
            {
                // 长按处理
            }
            else if (evt == BUTTON_EVENT_CLICK)
            {
                // 点击处理
            }
        }

        // 获取事件方式2：兼容原接口，只关心一种事件（如按下或者长按），适合简单场景
        // 但是只会返回第一个按下事件，其他事件会丢弃（适合简单场景）
        uint8_t pressed_key = Button_GetPressedKey();
        if (pressed_key)
        {
            // 对应原 Key_Scan 返回值的处理
        }
        uint8_t long_pressed_key = Button_GetLongPressedKey();
        if (long_pressed_key)
        {
            // 处理长按事件
        }
    }
}
