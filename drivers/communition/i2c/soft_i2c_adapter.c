/**
 * @file soft_i2c_adapter.c
 * @brief 软件I²C适配层实现（STM32 HAL库示例）
 */

#include "soft_i2c_gen.h"
#include "soft_i2c_adapter.h"
#include "stm32f1xx_hal.h" // 根据实际MCU更换头文件

/* 静态总线对象 */
static soft_i2c_bus_1_t i2c_bus_1;

/*===========================================================================
 * 底层引脚操作回调函数（需根据实际硬件修改）
 *===========================================================================*/
/* 定义SCL和SDA引脚 */

// 根据实际情况增删改GPIO端口和引脚号
#define I2C_SCL_PORT_1 GPIOB
#define I2C_SCL_PIN_1  GPIO_PIN_6
#define I2C_SDA_PORT_1 GPIOB
#define I2C_SDA_PIN_1  GPIO_PIN_7

static void i2c_scl_write_1(bool level)
{
    HAL_GPIO_WritePin(I2C_SCL_PORT_1, I2C_SCL_PIN_1, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void i2c_sda_write_1(bool level)
{
    HAL_GPIO_WritePin(I2C_SDA_PORT_1, I2C_SDA_PIN_1, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool i2c_sda_read_1(void)
{
    return (HAL_GPIO_ReadPin(I2C_SDA_PORT_1, I2C_SDA_PIN_1) == GPIO_PIN_SET);
}

/* 微秒延时回调（简单阻塞延时，可根据需要改为定时器） */
static void i2c_delay_us_1(uint32_t us)
{
    /* 粗略延时：适用于72MHz主频，实际应校准或使用硬件定时器 */
    volatile uint32_t count = us * 18; // 72MHz下约 1us = 18个循环
    while (count--)
    {
        __NOP();
    }
}

/*===========================================================================
 * 这个函数应在系统初始化阶段调用一次，配置GPIO并初始化软件I²C总线实例
 *===========================================================================*/

void soft_i2c_adapter_init(void)
{
    /* 初始化GPIO */
    /*
    这部分可以使用CubeMX生成。如果手动配置，就移动到注释外
    GPIO_InitTypeDef gpio_init = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    SCL: 推挽输出
    gpio_init.Pin   = I2C_SCL_PIN_1;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_SCL_PORT_1, &gpio_init);

    SDA: 开漏输出（也可设为推挽，但需注意读操作时先置高）
    gpio_init.Pin  = I2C_SDA_PIN_1;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(I2C_SDA_PORT_1, &gpio_init);
    */

    /* 初始化总线参数：
     *   delay_half = 5us  → 约100kHz (标准模式)
     *   timeout_ms = 100ms
     */
    soft_i2c_init(&i2c_bus_1, i2c_scl_write_1, i2c_sda_write_1, i2c_sda_read_1, i2c_delay_us_1,
                  5,    // 半周期5us → 100kHz
                  100); // 超时100ms
}

/*===========================================================================
 * 提供给外设的统一I²C操作函数（内部直接调用soft_i2c层）
 *===========================================================================*/
void OLED_I2C_Start(void)
{
    soft_i2c_start(&i2c_bus_1);
}

void OLED_I2C_Stop(void)
{
    soft_i2c_stop(&i2c_bus_1);
}

void OLED_I2C_SendByte(uint8_t byte)
{
    /* 发送字节并忽略错误（实际产品可记录返回值） */
    soft_i2c_write_byte(&i2c_bus_1, byte);
}

void OLED_I2C_WaitAck(void)
{
    /* soft_i2c_write_byte内部已处理ACK，此函数可留空 */
}
