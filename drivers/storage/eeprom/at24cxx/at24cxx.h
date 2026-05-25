/**
 * @file    at24cxx.h
 * @brief   AT24Cxx 系列 EEPROM 通用驱动头文件
 * @author  XuChengShuo1107
 * @date    2026-05-25
 * @note    基于 soft_i2c_simple (SoftI2C_Obj) 层实现
 *          与项目中 OLED 驱动使用同一 I2C 层，可共享总线实例
 *
 *          通用性设计：
 *          - 引脚配置集中放在 board_i2c_pins.h，换板只需改那里
 *          - EEPROM 和 OLED 共用同一总线时，共用同一个 SoftI2C_Obj
 *          - 支持 AT24C01~AT24C256 全系列
 *          - 8-bit / 16-bit 地址宽度自适应
 */

#ifndef __AT24CXX_H__
#define __AT24CXX_H__

#include <stdint.h>
#include <stdbool.h>
#include "soft_i2c_simple.h" /* SoftI2C_Obj / SoftI2C_WriteBuf / SoftI2C_ReadBuf */
#include "board_i2c_pins.h"  /* I2C_SHARED_BUS 宏 */
#include "delay.h"           /* delay_ms */

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================
 * 常用型号页大小宏定义（供配置时直接使用）
 *===========================================================================*/
#define AT24C01_PAGE_SIZE  8
#define AT24C02_PAGE_SIZE  8
#define AT24C04_PAGE_SIZE  16
#define AT24C08_PAGE_SIZE  16
#define AT24C16_PAGE_SIZE  16
#define AT24C32_PAGE_SIZE  32
#define AT24C64_PAGE_SIZE  32
#define AT24C128_PAGE_SIZE 64
#define AT24C256_PAGE_SIZE 64

/*===========================================================================
 * 地址宽度宏
 *===========================================================================*/
#define AT24C_ADDR_8BIT  1 /**< 8位地址 (AT24C01~16) */
#define AT24C_ADDR_16BIT 2 /**< 16位地址 (AT24C32+)  */

/*===========================================================================
 * 默认参数
 *===========================================================================*/
#define AT24C_DEFAULT_ADDR      0x50 /**< 默认7位I2C地址 (A0=A1=A2=GND) */
#define AT24C_WR_CYCLE_POLL_MS  50   /**< 写周期轮询超时 (ms) */
#define AT24C_WR_CYCLE_FIXED_MS 10   /**< 写周期固定延时 (ms)，备选 */

    /*===========================================================================
     * 配置结构体
     *===========================================================================*/

    /**
     * @brief  AT24Cxx 器件配置
     * @note   使用前填充，调用 AT24Cxx_Init 传入
     */
    typedef struct
    {
        SoftI2C_Obj *i2c;       /**< 已初始化的软 I2C 总线对象指针        */
        uint8_t      dev_addr;  /**< 7位设备地址 (如 0x50)               */
        uint16_t     page_size; /**< 页大小 (字节): 8/16/32/64           */
        uint8_t      addr_size; /**< 地址宽度: AT24C_ADDR_8BIT / _16BIT  */
        uint16_t     capacity;  /**< 总容量 (字节), 0 表示不做边界检查    */
    } AT24Cxx_Config_t;

    /*===========================================================================
     * 错误码
     *===========================================================================*/
    typedef enum
    {
        AT24CXX_OK        = 0,  /**< 操作成功                           */
        AT24CXX_ERR_I2C   = -1, /**< I2C 通信错误 (NACK/超时)            */
        AT24CXX_ERR_ADDR  = -2, /**< 地址越界                           */
        AT24CXX_ERR_BUSY  = -3, /**< 器件忙 (写周期超时)                 */
        AT24CXX_ERR_PARAM = -4, /**< 参数错误                           */
    } AT24Cxx_Status_t;

    /*===========================================================================
     * API 函数声明
     *===========================================================================*/

    /**
     * @brief  初始化 AT24Cxx 驱动
     * @param  cfg  器件配置指针，内部会拷贝
     * @retval AT24CXX_OK        成功
     * @retval AT24CXX_ERR_PARAM 参数无效
     * @note   调用前需确保 SoftI2C_Init 已完成、delay_init 已完成
     */
    AT24Cxx_Status_t AT24Cxx_Init(const AT24Cxx_Config_t *cfg);

    /**
     * @brief  等待 EEPROM 写周期完成 (轮询 ACK)
     * @retval AT24CXX_OK        器件就绪
     * @retval AT24CXX_ERR_BUSY  超时仍未就绪
     * @note   每次写操作后自动调用，通常情况下用户无需手动调用
     */
    AT24Cxx_Status_t AT24Cxx_WaitReady(void);

    /**
     * @brief  写入一个字节到指定地址
     * @param  addr  目标字节地址
     * @param  data  待写入数据
     * @retval AT24CXX_OK        写入成功
     * @retval AT24CXX_ERR_I2C   I2C 通信失败
     * @retval AT24CXX_ERR_ADDR  地址越界
     */
    AT24Cxx_Status_t AT24Cxx_WriteByte(uint16_t addr, uint8_t data);

    /**
     * @brief  从指定地址读取一个字节
     * @param  addr  目标字节地址
     * @param  data  读取到的数据 (输出)
     * @retval AT24CXX_OK        读取成功
     * @retval AT24CXX_ERR_I2C   I2C 通信失败
     * @retval AT24CXX_ERR_ADDR  地址越界
     */
    AT24Cxx_Status_t AT24Cxx_ReadByte(uint16_t addr, uint8_t *data);

    /**
     * @brief  写入一页数据 (不超过页大小，不跨页)
     * @param  page_start  页内起始地址
     * @param  data        数据缓冲区
     * @param  len         数据长度，超过页尾时自动截断
     * @retval AT24CXX_OK        写入成功
     * @retval AT24CXX_ERR_I2C   I2C 通信失败
     * @retval AT24CXX_ERR_ADDR  地址越界
     * @note   不会跨页写入，若跨页请使用 AT24Cxx_Write
     */
    AT24Cxx_Status_t AT24Cxx_WritePage(uint16_t page_start, const uint8_t *data, uint16_t len);

    /**
     * @brief  连续写入多字节 (自动处理页边界)
     * @param  addr  起始字节地址
     * @param  data  数据缓冲区
     * @param  len   数据长度
     * @retval AT24CXX_OK        全部写入成功
     * @retval AT24CXX_ERR_I2C   I2C 通信错误
     * @retval AT24CXX_ERR_ADDR  地址越界
     * @retval AT24CXX_ERR_PARAM data 为 NULL 或 len 为 0
     * @note   内部自动拆分跨页写入，每次页写后等待写周期完成
     */
    AT24Cxx_Status_t AT24Cxx_Write(uint16_t addr, const uint8_t *data, uint16_t len);

    /**
     * @brief  连续读取多字节 (自动跨页顺序读取)
     * @param  addr  起始字节地址
     * @param  data  数据缓冲区 (输出)
     * @param  len   期望读取长度
     * @retval AT24CXX_OK        读取成功
     * @retval AT24CXX_ERR_I2C   I2C 通信错误
     * @retval AT24CXX_ERR_ADDR  地址越界
     * @retval AT24CXX_ERR_PARAM data 为 NULL 或 len 为 0
     * @note   AT24Cxx 支持连续读自动地址递增，无需手动分页
     */
    AT24Cxx_Status_t AT24Cxx_Read(uint16_t addr, uint8_t *data, uint16_t len);

    /**
     * @brief  擦除整个芯片 (全部写为 0xFF)
     * @retval AT24CXX_OK        擦除完成
     * @retval AT24CXX_ERR_I2C   I2C 通信错误
     * @note   耗时较长，大容量芯片慎用
     */
    AT24Cxx_Status_t AT24Cxx_EraseChip(void);

#ifdef __cplusplus
}
#endif

#endif /* __AT24CXX_H__ */
