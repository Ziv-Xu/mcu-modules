/**
 * @file soft_i2c_simple.c
 * @brief 软件软件I2C简单模板
 * @author Ziv-Xu
 * @date 2026-05-20
 * @note 使用的时候，应当注意进行i2c的初始化。
 * @note 如果需要新增时序，需要在适配器中添加函数，如下面的SoftI2c_DefaultDelay_track，并在结构体中为delay成员赋值
 * @note 这个文件是一个简单的但是测试可用的模板，而且对于SCL和SDA的引脚配置不严格
 */
#include "soft_i2c.h"
#include "gpio.h"
#include "oled.h"
/*-------------------- 内部辅助 --------------------*/

/** @brief 默认延时函数，用户可通过 obj->delay 覆盖 */
static void SoftI2C_DefaultDelay(void)
{
    //    uint16_t i = 800;        /* 72MHz 下约 10µs，可根据主频调整 */
    //    while (i--);
}

// 用于项目中的一个示例，展示如何在适配器中添加一个新的时序函数，使用模板的时候可以删除或者改名后再使用这个函数
static void SoftI2C_DefaultDelay_track(void)
{
    uint16_t i = 800; /* 72MHz 下约 10µs，可根据主频调整 */
    while (i--);
}

/** @brief 将 SDA 切换为开漏输出（带上拉） */
static void SDA_OutputMode(SoftI2C_Obj *obj)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = obj->sda_pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(obj->sda_port, &GPIO_InitStruct);
}

/** @brief 将 SDA 切换为输入（带上拉，读取数据/ACK） */
static void SDA_InputMode(SoftI2C_Obj *obj)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = obj->sda_pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    HAL_GPIO_Init(obj->sda_port, &GPIO_InitStruct);
}

/* 根据端口使能 GPIO 时钟（支持常见 STM32 端口） */
static void SoftI2C_EnablePortClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
    /* 若使用 STM32F1 以外的系列，请按需补充 */
}

/*====================== 初始化 ======================*/
/**
 * @brief  初始化 I2C 总线
 * @param  obj 对象指针，其中的延时函数若为 NULL 则采用默认延时
 */
void SoftI2C_Init(SoftI2C_Obj *obj)
{
    /* 使能相关 GPIO 时钟 */
    SoftI2C_EnablePortClock(obj->scl_port);
    SoftI2C_EnablePortClock(obj->sda_port);

    /* 配置 SCL 为开漏输出 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = obj->scl_pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(obj->scl_port, &GPIO_InitStruct);

    /* 配置 SDA 为开漏输出 */
    GPIO_InitStruct.Pin = obj->sda_pin;
    HAL_GPIO_Init(obj->sda_port, &GPIO_InitStruct);

    /* 总线空闲状态：SCL=1, SDA=1 */
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET);

    /* 若用户未提供延时函数，填入默认 */
    if (obj->delay == NULL)
    {
        obj->delay = SoftI2C_DefaultDelay;
    }
}

/*====================== 基础时序 ======================*/
void SoftI2C_Start(SoftI2C_Obj *obj)
{
    SDA_OutputMode(obj);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    obj->delay();
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_RESET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    obj->delay();
}

void SoftI2C_Stop(SoftI2C_Obj *obj)
{
    SDA_OutputMode(obj);
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_RESET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    obj->delay();
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET);
    obj->delay();
}

uint8_t SoftI2C_WaitAck(SoftI2C_Obj *obj)
{
    uint8_t ucErrTime = 0;

    SDA_InputMode(obj);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET); /* 释放总线 */
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    obj->delay();

    while (HAL_GPIO_ReadPin(obj->sda_port, obj->sda_pin))
    {
        if (++ucErrTime > 250)
        {
            SoftI2C_Stop(obj); /* Stop 内部会将 SDA 切为输出 */
            return 1;          /* 无应答 */
        }
    }

    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    obj->delay();
    SDA_OutputMode(obj); /* 恢复 SDA 为输出，准备后续操作 */
    return 0;            /* 应答正常 */
}

void SoftI2C_Ack(SoftI2C_Obj *obj)
{
    SDA_OutputMode(obj);
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_RESET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    obj->delay();
}

void SoftI2C_NAck(SoftI2C_Obj *obj)
{
    SDA_OutputMode(obj);
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
    obj->delay();
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    obj->delay();
}

void SoftI2C_SendByte(SoftI2C_Obj *obj, uint8_t data)
{
    uint8_t i;
    SDA_OutputMode(obj);
    HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(obj->sda_port, obj->sda_pin, GPIO_PIN_RESET);
        data <<= 1;
        obj->delay();
        HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
        obj->delay();
        HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
        obj->delay();
    }
}

uint8_t SoftI2C_ReadByte(SoftI2C_Obj *obj, uint8_t ack)
{
    uint8_t i, receive = 0;
    SDA_InputMode(obj);

    for (i = 0; i < 8; i++)
    {
        HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_RESET);
        obj->delay();
        HAL_GPIO_WritePin(obj->scl_port, obj->scl_pin, GPIO_PIN_SET);
        /* 若主频高可加小等待确保数据稳定，此处沿用原逻辑 */
        receive <<= 1;
        if (HAL_GPIO_ReadPin(obj->sda_port, obj->sda_pin))
            receive |= 0x01;
        obj->delay();
    }

    SDA_OutputMode(obj); /* 准备发送 ACK/NACK */
    if (ack)
        SoftI2C_Ack(obj);
    else
        SoftI2C_NAck(obj);
    return receive;
}

/*=================== 高级读写接口 ====================*/
uint8_t SoftI2C_WriteOneByte(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_WRITE);
    if (SoftI2C_WaitAck(obj))
    {
        SoftI2C_Stop(obj);
        return 1;
    }
    SoftI2C_SendByte(obj, reg);
    if (SoftI2C_WaitAck(obj))
    {
        SoftI2C_Stop(obj);
        return 1;
    }
    SoftI2C_SendByte(obj, data);
    if (SoftI2C_WaitAck(obj))
    {
        SoftI2C_Stop(obj);
        return 1;
    }
    SoftI2C_Stop(obj);
    return 0;
}

uint8_t SoftI2C_ReadOneByte(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg)
{
    uint8_t temp = 0;
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_WRITE);
    SoftI2C_WaitAck(obj);
    SoftI2C_SendByte(obj, reg);
    SoftI2C_WaitAck(obj);
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_READ);
    SoftI2C_WaitAck(obj);
    temp = SoftI2C_ReadByte(obj, 0); /* 最后一个字节发送 NACK */
    SoftI2C_Stop(obj);
    return temp;
}

void SoftI2C_WriteBuf(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_WRITE);
    SoftI2C_WaitAck(obj);
    SoftI2C_SendByte(obj, reg);
    SoftI2C_WaitAck(obj);
    while (len--)
    {
        SoftI2C_SendByte(obj, *buf++);
        SoftI2C_WaitAck(obj);
    }
    SoftI2C_Stop(obj);
}

void SoftI2C_ReadBuf(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_WRITE);
    SoftI2C_WaitAck(obj);
    SoftI2C_SendByte(obj, reg);
    SoftI2C_WaitAck(obj);
    SoftI2C_Start(obj);
    SoftI2C_SendByte(obj, (dev_addr << 1) | SOFT_I2C_READ);
    SoftI2C_WaitAck(obj);
    while (len)
    {
        if (len == 1)
            *buf = SoftI2C_ReadByte(obj, 0); /* 最后一个字节 NACK */
        else
            *buf = SoftI2C_ReadByte(obj, 1); /* 中间字节 ACK */
        buf++;
        len--;
    }
    SoftI2C_Stop(obj);
}

SoftI2C_Obj i2c_oled = {
    .scl_port = GPIOB,      // 对应原来的 I2C_SCL_GPIO_PORT
    .scl_pin  = GPIO_PIN_8, // 对应原来的 I2C_SCL_PIN
    .sda_port = GPIOB,      // 对应原来的 I2C_SDA_GPIO_PORT
    .sda_pin  = GPIO_PIN_9, // 对应原来的 I2C_SDA_PIN
    .delay    = NULL        // 使用默认延时
};

SoftI2C_Obj i2c_mpu6050 = {
    .scl_port = GPIOB,      // 对应原来的 I2C_SCL_GPIO_PORT
    .scl_pin  = GPIO_PIN_4, // 对应原来的 I2C_SCL_PIN
    .sda_port = GPIOB,      // 对应原来的 I2C_SDA_GPIO_PORT
    .sda_pin  = GPIO_PIN_5, // 对应原来的 I2C_SDA_PIN
    .delay    = NULL        // 使用默认延时
};

SoftI2C_Obj i2c_track1 = {.scl_port = GPIOB,       // 对应原来的 I2C_SCL_GPIO_PORT
                          .scl_pin  = GPIO_PIN_10, // 对应原来的 I2C_SCL_PIN
                          .sda_port = GPIOB,       // 对应原来的 I2C_SDA_GPIO_PORT
                          .sda_pin  = GPIO_PIN_11, // 对应原来的 I2C_SDA_PIN
                          .delay    = SoftI2C_DefaultDelay_track};

SoftI2C_Obj i2c_track2 = {
    .scl_port = GPIOA,       // 对应原来的 I2C_SCL_GPIO_PORT
    .scl_pin  = GPIO_PIN_13, // 对应原来的 I2C_SCL_PIN
    .sda_port = GPIOA,       // 对应原来的 I2C_SDA_GPIO_PORT
    .sda_pin  = GPIO_PIN_14, // 对应原来的 I2C_SDA_PIN
    .delay    = NULL         // 使用默认延时
};

/**
 * @brief  扫描 I2C 总线，打印所有应答设备的 7 位地址
 * @param  obj : 已初始化的 SoftI2C_Obj 对象指针（例如 &i2c_mpu6050）
 * @return 找到的设备数量
 * @note   需要已配置串口打印（重定向 printf 或使用 HAL_UART_Transmit）
 */
uint8_t I2C_ScanBus(SoftI2C_Obj *obj)
{
    uint8_t devices_found = 0;
    // printf("Scanning I2C bus...\r\n");
    for (uint16_t addr = 1; addr < 128; addr++)
    {
        SoftI2C_Start(obj);
        SoftI2C_SendByte(obj, (uint8_t) (addr << 1) | SOFT_I2C_WRITE);
        if (SoftI2C_WaitAck(obj) == 0)
        { // 收到 ACK → 设备存在
            OLED_ShowHexNum(3, 1, addr, 4);
            // printf("Device found at 0x%02X\r\n", addr);
            devices_found++;
        }
        SoftI2C_Stop(obj);
    }
    if (devices_found == 0)
        OLED_ShowFloatNum(4, 1, 1.1f, 1, 1);
    // printf("No I2C devices found.\r\n");
    else
        // printf("Found %d device(s).\r\n", devices_found);
        return devices_found;
}
