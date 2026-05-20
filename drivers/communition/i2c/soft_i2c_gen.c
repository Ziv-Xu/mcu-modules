/**
 * @file    soft_i2c_gen.c
 * @brief   企业级软件 I²C 主机驱动实现
 * @note    严格遵循标准 I²C
 * 时序，支持多实例，全异步错误返回
 */

#include "soft_i2c_gen.h"

/*-----------------------------------------------------------------------
 * 内部辅助宏
 *-----------------------------------------------------------------------*/
#define SOFT_I2C_CHECK_BUS(bus)        \
    do                                 \
    {                                  \
        if ((bus) == NULL)             \
            return SOFT_I2C_ERR_PARAM; \
    } while (0)

#define SOFT_I2C_CHECK_NULL(ptr)       \
    do                                 \
    {                                  \
        if ((ptr) == NULL)             \
            return SOFT_I2C_ERR_PARAM; \
    } while (0)

/*-----------------------------------------------------------------------
 * 内部函数: 释放总线(置高)
 *-----------------------------------------------------------------------*/
static inline void i2c_scl_h(soft_i2c_bus_t *bus)
{
    bus->scl_write(true);
}
static inline void i2c_scl_l(soft_i2c_bus_t *bus)
{
    bus->scl_write(false);
}
static inline void i2c_sda_h(soft_i2c_bus_t *bus)
{
    bus->sda_write(true);
}
static inline void i2c_sda_l(soft_i2c_bus_t *bus)
{
    bus->sda_write(false);
}
static inline bool i2c_sda_in(soft_i2c_bus_t *bus)
{
    return bus->sda_read();
}

/* 延时函数 */
static inline void i2c_delay_half(soft_i2c_bus_t *bus)
{
    bus->delay_us(bus->delay_half);
}

/*-----------------------------------------------------------------------
 * 初始化
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_init(soft_i2c_bus_t *bus, soft_i2c_scl_write_t scl_write,
                             soft_i2c_sda_write_t sda_write, soft_i2c_sda_read_t sda_read,
                             soft_i2c_delay_us_t delay_us, uint32_t delay_half, uint32_t timeout_ms)
{
    SOFT_I2C_CHECK_BUS(bus);
    SOFT_I2C_CHECK_NULL(scl_write);
    SOFT_I2C_CHECK_NULL(sda_write);
    SOFT_I2C_CHECK_NULL(sda_read);
    SOFT_I2C_CHECK_NULL(delay_us);

    bus->scl_write  = scl_write;
    bus->sda_write  = sda_write;
    bus->sda_read   = sda_read;
    bus->delay_us   = delay_us;
    bus->delay_half = delay_half;
    bus->timeout_ms = timeout_ms;

    /* 初始状态: 总线空闲，SDA/SCL 均拉高 */
    i2c_sda_h(bus);
    i2c_scl_h(bus);
    i2c_delay_half(bus);

    return SOFT_I2C_OK;
}

/*-----------------------------------------------------------------------
 * 起始条件: SCL 高电平时 SDA 下降沿
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_start(soft_i2c_bus_t *bus)
{
    SOFT_I2C_CHECK_BUS(bus);

    i2c_sda_h(bus);
    i2c_delay_half(bus);
    i2c_scl_h(bus);
    i2c_delay_half(bus);
    i2c_sda_l(bus);
    i2c_delay_half(bus);
    i2c_scl_l(bus);
    i2c_delay_half(bus);

    return SOFT_I2C_OK;
}

/*-----------------------------------------------------------------------
 * 停止条件: SCL 高电平时 SDA 上升沿
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_stop(soft_i2c_bus_t *bus)
{
    SOFT_I2C_CHECK_BUS(bus);

    i2c_sda_l(bus);
    i2c_delay_half(bus);
    i2c_scl_h(bus);
    i2c_delay_half(bus);
    i2c_sda_h(bus);
    i2c_delay_half(bus);
    /* 可选的额外延时确保总线空闲 */
    i2c_delay_half(bus);

    return SOFT_I2C_OK;
}

/*-----------------------------------------------------------------------
 * 发送一个字节 (MSB first)
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_write_byte(soft_i2c_bus_t *bus, uint8_t data)
{
    SOFT_I2C_CHECK_BUS(bus);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            i2c_sda_h(bus);
        }
        else
        {
            i2c_sda_l(bus);
        }
        i2c_delay_half(bus);
        i2c_scl_h(bus);
        i2c_delay_half(bus);
        i2c_scl_l(bus);
        i2c_delay_half(bus);
        data <<= 1;
    }

    /* 释放 SDA 准备接收 ACK */
    i2c_sda_h(bus);
    i2c_delay_half(bus);
    i2c_scl_h(bus);
    i2c_delay_half(bus);

    /* 超时等待 ACK */
    uint32_t timeout = bus->timeout_ms;
    while (i2c_sda_in(bus))
    { /* 等待从机拉低 SDA */
        if (bus->timeout_ms != 0)
        {
            if (timeout == 0)
            {
                i2c_scl_l(bus); /* 恢复总线 */
                return SOFT_I2C_ERR_TIMEOUT;
            }
            /* 以 1ms
             * 为单位递减，实际项目中可根据需要调整粒度 */
            bus->delay_us(1000);
            timeout--;
        }
    }

    /* 检查是否为 ACK (低电平) */
    bool ack = !i2c_sda_in(bus);
    i2c_scl_l(bus);
    i2c_delay_half(bus);

    return ack ? SOFT_I2C_OK : SOFT_I2C_ERR_NACK;
}

/*-----------------------------------------------------------------------
 * 读取一个字节并发送应答位
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_read_byte(soft_i2c_bus_t *bus, bool ack, uint8_t *data)
{
    SOFT_I2C_CHECK_BUS(bus);
    SOFT_I2C_CHECK_NULL(data);

    uint8_t value = 0;

    /* 释放 SDA (设置为输入模式已在硬件层由 sda_read
     * 体现，这里只需确保 SDA 为高阻) */
    i2c_sda_h(bus); /* 主机释放 SDA，准备接收 */

    for (uint8_t i = 0; i < 8; i++)
    {
        value <<= 1;
        i2c_scl_h(bus);
        i2c_delay_half(bus);
        if (i2c_sda_in(bus))
        {
            value |= 1;
        }
        i2c_scl_l(bus);
        i2c_delay_half(bus);
    }

    /* 发送 ACK / NACK */
    if (ack)
    {
        i2c_sda_l(bus);
    }
    else
    {
        i2c_sda_h(bus);
    }
    i2c_delay_half(bus);
    i2c_scl_h(bus);
    i2c_delay_half(bus);
    i2c_scl_l(bus);
    i2c_delay_half(bus);

    /* 释放 SDA */
    i2c_sda_h(bus);

    *data = value;
    return SOFT_I2C_OK;
}

/*-----------------------------------------------------------------------
 * 主机写入多字节 (包含可选寄存器地址)
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_master_write(soft_i2c_bus_t *bus, uint8_t dev_addr, const uint8_t *reg_addr,
                                     uint8_t reg_len, const uint8_t *data, uint16_t len)
{
    SOFT_I2C_CHECK_BUS(bus);
    /* data 可以为 NULL 如果 len 为
     * 0，但注册地址也不能为空时... 根据实际情况判断 */
    if ((reg_len > 0 && reg_addr == NULL) || (len > 0 && data == NULL))
    {
        return SOFT_I2C_ERR_PARAM;
    }

    soft_i2c_err_t ret;

    /* 起始 */
    ret = soft_i2c_start(bus);
    if (ret != SOFT_I2C_OK)
        return ret;

    /* 从机地址 + 写位 */
    ret = soft_i2c_write_byte(bus, (uint8_t) (dev_addr << 1));
    if (ret != SOFT_I2C_OK)
    {
        soft_i2c_stop(bus);
        return ret;
    }

    /* 寄存器地址(可选) */
    for (uint8_t i = 0; i < reg_len; i++)
    {
        ret = soft_i2c_write_byte(bus, reg_addr[i]);
        if (ret != SOFT_I2C_OK)
        {
            soft_i2c_stop(bus);
            return ret;
        }
    }

    /* 数据 */
    for (uint16_t i = 0; i < len; i++)
    {
        ret = soft_i2c_write_byte(bus, data[i]);
        if (ret != SOFT_I2C_OK)
        {
            soft_i2c_stop(bus);
            return ret;
        }
    }

    /* 停止 */
    return soft_i2c_stop(bus);
}

/*-----------------------------------------------------------------------
 * 主机读取多字节 (包含可选寄存器地址)
 *-----------------------------------------------------------------------*/
soft_i2c_err_t soft_i2c_master_read(soft_i2c_bus_t *bus, uint8_t dev_addr, const uint8_t *reg_addr,
                                    uint8_t reg_len, uint8_t *data, uint16_t len)
{
    SOFT_I2C_CHECK_BUS(bus);
    if (len == 0 || data == NULL)
    {
        return SOFT_I2C_ERR_PARAM;
    }
    if (reg_len > 0 && reg_addr == NULL)
    {
        return SOFT_I2C_ERR_PARAM;
    }

    soft_i2c_err_t ret;

    /* 如果有寄存器地址，先通过写操作指定地址 */
    if (reg_len > 0)
    {
        ret = soft_i2c_start(bus);
        if (ret != SOFT_I2C_OK)
            return ret;

        ret = soft_i2c_write_byte(bus, (uint8_t) (dev_addr << 1));
        if (ret != SOFT_I2C_OK)
        {
            soft_i2c_stop(bus);
            return ret;
        }

        for (uint8_t i = 0; i < reg_len; i++)
        {
            ret = soft_i2c_write_byte(bus, reg_addr[i]);
            if (ret != SOFT_I2C_OK)
            {
                soft_i2c_stop(bus);
                return ret;
            }
        }
        /* 不发送停止，直接发起重复起始 */
    }

    /* 起始(或重复起始) */
    ret = soft_i2c_start(bus);
    if (ret != SOFT_I2C_OK)
        return ret;

    /* 从机地址 + 读位 */
    ret = soft_i2c_write_byte(bus, (uint8_t) ((dev_addr << 1) | 0x01));
    if (ret != SOFT_I2C_OK)
    {
        soft_i2c_stop(bus);
        return ret;
    }

    /* 读取 len-1 个字节，每个回复 ACK */
    for (uint16_t i = 0; i < len - 1; i++)
    {
        ret = soft_i2c_read_byte(bus, true, &data[i]);
        if (ret != SOFT_I2C_OK)
        {
            soft_i2c_stop(bus);
            return ret;
        }
    }

    /* 最后一个字节回复 NACK */
    ret = soft_i2c_read_byte(bus, false, &data[len - 1]);
    if (ret != SOFT_I2C_OK)
    {
        soft_i2c_stop(bus);
        return ret;
    }

    return soft_i2c_stop(bus);
}

/**
 * @brief  扫描 I2C 总线，检测所有应答设备的 7 位地址
 * @param  bus 已初始化的软件 I2C 总线对象指针
 * @return 找到的设备数量
 * @note   需要已配置好串口或 OLED 等输出设备以显示地址（可选）
 */
uint8_t I2C_ScanBus(soft_i2c_bus_t *bus)
{
    uint8_t devices_found = 0;

    for (uint16_t addr = 1; addr < 128; addr++) // 地址 0 保留，128 为广播地址
    {
        soft_i2c_err_t ret;

        // 发送起始条件
        ret = soft_i2c_start(bus);
        if (ret != SOFT_I2C_OK)
        {
            // 起始失败（极少发生），跳过本次循环
            continue;
        }

        // 发送 7 位从机地址 + 写位（写位 = 0）
        ret = soft_i2c_write_byte(bus, (uint8_t) (addr << 1));

        if (ret == SOFT_I2C_OK)
        {
            // 收到 ACK → 设备存在
            devices_found++;

            // 可选：显示找到的地址，例如通过 OLED 或串口
            // OLED_ShowHexNum(3, 1, addr, 4);   // 用户自定义显示函数
            // printf("Device found at 0x%02X\r\n", addr);
        }

        // 发送停止条件，释放总线（无论是否成功都要停止）
        soft_i2c_stop(bus);
    }

    return devices_found;
}
