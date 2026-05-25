/**
 * @file    at24cxx.c
 * @brief   AT24Cxx 系列 EEPROM 通用驱动实现
 * @author  XuChengShuo
 * @date    2026-05-25
 * @note    基于 soft_i2c_simple (SoftI2C_Obj) 层实现
 *          与项目中 OLED 驱动共用同一 I2C 抽象层
 *
 *          引脚配置统一在 board_i2c_pins.h 中定义
 *          换开发板只需改 board_i2c_pins.h 中的 4 行宏
 *
 *          调试方法 (参考 OLED 驱动调试方式):
 *          1. 调用 I2C_ScanBus 确认器件地址是否应答
 *          2. 检查上拉电阻 (4.7kΩ)
 *          3. 在 WaitReady 中打印 ACK 状态
 */

#include <stddef.h> /* NULL */
#include "at24cxx.h"

/*===========================================================================
 * 内部变量
 *===========================================================================*/

/** 当前 EEPROM 配置 (由 AT24Cxx_Init 设置) */
static AT24Cxx_Config_t s_cfg = {
    .i2c       = NULL,
    .dev_addr  = AT24C_DEFAULT_ADDR,
    .page_size = AT24C02_PAGE_SIZE,
    .addr_size = AT24C_ADDR_8BIT,
    .capacity  = 0,
};

/* 快捷引用总线对象 */
#define I2C s_cfg.i2c

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief  检查地址是否越界
 */
static inline bool addr_valid(uint16_t addr, uint16_t len)
{
    if (s_cfg.capacity == 0)
        return true;
    return ((uint32_t) addr + len) <= s_cfg.capacity;
}

/**
 * @brief  发送 EEPROM 字节地址 (8-bit 或 16-bit)
 * @param  addr  16位字节地址
 * @retval 0: 成功, 1: NACK
 *
 * AT24C01~16: 只需发送低 8 位
 * AT24C32+:   先发高 8 位，再发低 8 位
 */
static uint8_t send_address(uint16_t addr)
{
    if (s_cfg.addr_size == AT24C_ADDR_16BIT)
    {
        SoftI2C_SendByte(I2C, (uint8_t) (addr >> 8)); /* 高字节 */
        if (SoftI2C_WaitAck(I2C))
            return 1;
    }
    SoftI2C_SendByte(I2C, (uint8_t) (addr & 0xFF)); /* 低字节 */
    if (SoftI2C_WaitAck(I2C))
        return 1;
    return 0; /* 成功 */
}

/**
 * @brief  发送器件地址 + 写位，并等待 ACK
 */
static uint8_t start_write_session(void)
{
    SoftI2C_Start(I2C);
    SoftI2C_SendByte(I2C, (uint8_t) (s_cfg.dev_addr << 1) | SOFT_I2C_WRITE);
    return SoftI2C_WaitAck(I2C); /* 返回 0=ACK, 1=NACK */
}

/*===========================================================================
 * API 实现
 *===========================================================================*/

AT24Cxx_Status_t AT24Cxx_Init(const AT24Cxx_Config_t *cfg)
{
    if (cfg == NULL || cfg->i2c == NULL)
    {
        return AT24CXX_ERR_PARAM;
    }

    s_cfg.i2c       = cfg->i2c;
    s_cfg.dev_addr  = cfg->dev_addr;
    s_cfg.page_size = cfg->page_size;
    s_cfg.addr_size = cfg->addr_size;
    s_cfg.capacity  = cfg->capacity;

    /* 验证页大小合法性 */
    if (s_cfg.page_size != 8 && s_cfg.page_size != 16 && s_cfg.page_size != 32 && s_cfg.page_size != 64)
    {
        s_cfg.page_size = AT24C02_PAGE_SIZE;
    }

    /* 验证地址宽度合法性 */
    if (s_cfg.addr_size != AT24C_ADDR_8BIT && s_cfg.addr_size != AT24C_ADDR_16BIT)
    {
        s_cfg.addr_size = AT24C_ADDR_8BIT;
    }

    /* 给 EEPROM 一点上电稳定时间 */
    delay_ms(10);

    return AT24CXX_OK;
}

/*-------------------------------------------------------------------
 * 等待写周期完成
 *
 * 原理：发送 START + 器件地址 + 写位
 *       - 收到 ACK → 内部写周期已结束，器件就绪
 *       - 收到 NACK → 仍在写，等待 1ms 后重试
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_WaitReady(void)
{
    uint32_t timeout = AT24C_WR_CYCLE_POLL_MS;

    while (timeout > 0)
    {
        SoftI2C_Start(I2C);
        SoftI2C_SendByte(I2C, (uint8_t) (s_cfg.dev_addr << 1) | SOFT_I2C_WRITE);

        if (SoftI2C_WaitAck(I2C) == 0) /* 0 = ACK */
        {
            SoftI2C_Stop(I2C);
            return AT24CXX_OK;
        }

        /* NACK → 器件仍在写周期 */
        SoftI2C_Stop(I2C);
        delay_ms(1);
        timeout--;
    }

    return AT24CXX_ERR_BUSY;
}

/*-------------------------------------------------------------------
 * 读取一个字节
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_ReadByte(uint16_t addr, uint8_t *data)
{
    if (data == NULL)
        return AT24CXX_ERR_PARAM;
    if (!addr_valid(addr, 1))
        return AT24CXX_ERR_ADDR;

    if (s_cfg.addr_size == AT24C_ADDR_8BIT)
    {
        /* SoftI2C_ReadBuf 内部处理了 8-bit reg + repeated start + read */
        SoftI2C_ReadBuf(I2C, s_cfg.dev_addr, (uint8_t) (addr & 0xFF), 1, data);
        return AT24CXX_OK;
    }
    else
    {
        /* 16-bit 地址需要手动发送两个地址字节 */
        if (start_write_session())
            return AT24CXX_ERR_I2C;
        if (send_address(addr))
            return AT24CXX_ERR_I2C;

        /* 重复起始 → 读 */
        SoftI2C_Start(I2C);
        SoftI2C_SendByte(I2C, (uint8_t) (s_cfg.dev_addr << 1) | SOFT_I2C_READ);
        if (SoftI2C_WaitAck(I2C))
            return AT24CXX_ERR_I2C;

        *data = SoftI2C_ReadByte(I2C, 0); /* NACK (最后一个字节) */
        SoftI2C_Stop(I2C);
        return AT24CXX_OK;
    }
}

/*-------------------------------------------------------------------
 * 写入一个字节
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_WriteByte(uint16_t addr, uint8_t data)
{
    if (!addr_valid(addr, 1))
        return AT24CXX_ERR_ADDR;

    if (s_cfg.addr_size == AT24C_ADDR_8BIT)
    {
        SoftI2C_WriteBuf(I2C, s_cfg.dev_addr, (uint8_t) (addr & 0xFF), 1, &data);
    }
    else
    {
        /* 16-bit 地址手动发送 */
        if (start_write_session())
            return AT24CXX_ERR_I2C;
        if (send_address(addr))
            return AT24CXX_ERR_I2C;

        SoftI2C_SendByte(I2C, data);
        if (SoftI2C_WaitAck(I2C))
            return AT24CXX_ERR_I2C;
        SoftI2C_Stop(I2C);
    }

    return AT24Cxx_WaitReady();
}

/*-------------------------------------------------------------------
 * 页写入 (不跨页)
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_WritePage(uint16_t page_start, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return AT24CXX_ERR_PARAM;
    if (!addr_valid(page_start, len))
        return AT24CXX_ERR_ADDR;

    /* 计算页内剩余空间，截断防止跨页 */
    uint16_t page_boundary  = (page_start / s_cfg.page_size + 1) * s_cfg.page_size;
    uint16_t remain_in_page = page_boundary - page_start;
    if (len > remain_in_page)
    {
        len = remain_in_page;
    }

    if (s_cfg.addr_size == AT24C_ADDR_8BIT)
    {
        SoftI2C_WriteBuf(I2C, s_cfg.dev_addr, (uint8_t) (page_start & 0xFF), (uint8_t) len, (uint8_t *) data);
    }
    else
    {
        if (start_write_session())
            return AT24CXX_ERR_I2C;
        if (send_address(page_start))
            return AT24CXX_ERR_I2C;

        for (uint16_t i = 0; i < len; i++)
        {
            SoftI2C_SendByte(I2C, data[i]);
            if (SoftI2C_WaitAck(I2C))
                return AT24CXX_ERR_I2C;
        }
        SoftI2C_Stop(I2C);
    }

    return AT24Cxx_WaitReady();
}

/*-------------------------------------------------------------------
 * 连续写入 (自动处理页边界)
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_Write(uint16_t addr, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return AT24CXX_ERR_PARAM;
    if (!addr_valid(addr, len))
        return AT24CXX_ERR_ADDR;

    uint16_t remaining    = len;
    uint16_t current_addr = addr;

    while (remaining > 0)
    {
        uint16_t page_boundary = (current_addr / s_cfg.page_size + 1) * s_cfg.page_size;
        uint16_t chunk         = page_boundary - current_addr;
        if (chunk > remaining)
            chunk = remaining;

        AT24Cxx_Status_t status = AT24Cxx_WritePage(current_addr, data, chunk);
        if (status != AT24CXX_OK)
            return status;

        current_addr += chunk;
        data += chunk;
        remaining -= chunk;
    }

    return AT24CXX_OK;
}

/*-------------------------------------------------------------------
 * 连续读取多字节
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_Read(uint16_t addr, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return AT24CXX_ERR_PARAM;
    if (!addr_valid(addr, len))
        return AT24CXX_ERR_ADDR;

    if (s_cfg.addr_size == AT24C_ADDR_8BIT)
    {
        SoftI2C_ReadBuf(I2C, s_cfg.dev_addr, (uint8_t) (addr & 0xFF), (uint8_t) len, data);
        return AT24CXX_OK;
    }

    /* 16-bit 地址手动处理 */
    if (start_write_session())
        return AT24CXX_ERR_I2C;
    if (send_address(addr))
        return AT24CXX_ERR_I2C;

    /* 重复起始 → 读 */
    SoftI2C_Start(I2C);
    SoftI2C_SendByte(I2C, (uint8_t) (s_cfg.dev_addr << 1) | SOFT_I2C_READ);
    if (SoftI2C_WaitAck(I2C))
        return AT24CXX_ERR_I2C;

    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t ack = (i < len - 1) ? 1 : 0; /* 最后一个字节 NACK */
        data[i]     = SoftI2C_ReadByte(I2C, ack);
    }
    SoftI2C_Stop(I2C);

    return AT24CXX_OK;
}

/*-------------------------------------------------------------------
 * 擦除整个芯片
 *-------------------------------------------------------------------*/
AT24Cxx_Status_t AT24Cxx_EraseChip(void)
{
    if (s_cfg.capacity == 0)
        return AT24CXX_ERR_PARAM;

    uint8_t page_buf[64]; /* 最大页大小 64 字节 */
    for (uint16_t i = 0; i < s_cfg.page_size; i++) page_buf[i] = 0xFF;

    for (uint16_t addr = 0; addr < s_cfg.capacity; addr += s_cfg.page_size)
    {
        AT24Cxx_Status_t status = AT24Cxx_WritePage(addr, page_buf, s_cfg.page_size);
        if (status != AT24CXX_OK)
            return status;
    }

    return AT24CXX_OK;
}
