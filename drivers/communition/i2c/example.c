#include "soft_i2c.h"

/* 假设硬件 GPIO 操作函数 */
static void my_scl_write(bool level)
{
    if (level)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
}
static void my_sda_write(bool level)
{
    if (level)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
}
static bool my_sda_read(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET;
}
static void my_delay_us(uint32_t us)
{
    // 使用定时器或 DWT 实现微秒延时
}

/* 定义一条总线实例 */
soft_i2c_bus_t i2c1;

int main(void)
{
    soft_i2c_init(&i2c1, my_scl_write, my_sda_write, my_sda_read, my_delay_us,
                  5,    /* 半周期 5us → 100kHz */
                  100); /* 超时 100ms */

    uint8_t reg = 0x00;
    uint8_t data[4];
    soft_i2c_master_read(&i2c1, 0x50, &reg, 1, data, 4);
    // ...
}
