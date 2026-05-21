/**
 * @file    soft_i2c.h
 * @brief   企业级软件 I²C 主机驱动头文件
 * @note    支持多总线实例，通过回调函数实现硬件无关
 */

#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*===========================================================================
     * 错误码定义
     *===========================================================================*/
    typedef enum
    {
        SOFT_I2C_OK              = 0,  /**< 操作成功 */
        SOFT_I2C_ERR_TIMEOUT     = -1, /**< 等待应答超时 */
        SOFT_I2C_ERR_ARBITRATION = -2, /**< 仲裁丢失 (暂未实现检测) */
        SOFT_I2C_ERR_NACK        = -3, /**< 从机返回 NACK */
        SOFT_I2C_ERR_BUSY        = -4, /**< 总线忙 (暂未实现) */
        SOFT_I2C_ERR_PARAM       = -5, /**< 参数错误 */
    } soft_i2c_err_t;

    /*===========================================================================
     * 引脚操作回调类型定义
     *===========================================================================*/

    /** SCL 引脚写操作: level 为 true 输出高, false 输出低
     */
    typedef void (*soft_i2c_scl_write_t)(bool level);

    /** SDA 引脚写操作: level 为 true 输出高, false 输出低
     */
    typedef void (*soft_i2c_sda_write_t)(bool level);

    /** SDA 引脚读操作: 返回 true 表示高电平, false
     * 表示低电平 */
    typedef bool (*soft_i2c_sda_read_t)(void);

    /** 微秒延时 */
    typedef void (*soft_i2c_delay_us_t)(uint32_t us);

    /*===========================================================================
     * 软件 I²C 总线对象
     *===========================================================================*/
    typedef struct
    {
        soft_i2c_scl_write_t scl_write; /**< SCL 写回调 */
        soft_i2c_sda_write_t sda_write; /**< SDA 写回调 */
        soft_i2c_sda_read_t  sda_read;  /**< SDA 读回调 */
        soft_i2c_delay_us_t  delay_us;  /**< 延时回调 */

        uint32_t timeout_ms; /**< 单次操作超时阈值(ms) */
        uint32_t delay_half; /**< 半周期延时(us)，决定速率:
                                   标准模式(100k): 5us
                                   快速模式(400k): 1.25us，建议
                                2us 用户自定义即可 */
    } soft_i2c_bus_t;

    /*===========================================================================
     * API 函数
     *===========================================================================*/

    /**
     * @brief  初始化软件 I²C 总线实例
     * @param  bus        总线对象指针
     * @param  scl_write  SCL 写回调
     * @param  sda_write  SDA 写回调
     * @param  sda_read   SDA 读回调
     * @param  delay_us   微秒延时回调
     * @param  delay_half 半周期延时(us)
     * @param  timeout_ms 超时阈值(ms), 0 表示永不超时
     * @return 总是返回 SOFT_I2C_OK
     */
    soft_i2c_err_t soft_i2c_init(soft_i2c_bus_t *bus, soft_i2c_scl_write_t scl_write,
                                 soft_i2c_sda_write_t sda_write, soft_i2c_sda_read_t sda_read,
                                 soft_i2c_delay_us_t delay_us, uint32_t delay_half,
                                 uint32_t timeout_ms);

    /**
     * @brief  发送起始条件
     * @param  bus 总线对象指针
     * @return 执行结果
     */
    soft_i2c_err_t soft_i2c_start(soft_i2c_bus_t *bus);

    /**
     * @brief  发送停止条件
     * @param  bus 总线对象指针
     * @return 总是 SOFT_I2C_OK
     */
    soft_i2c_err_t soft_i2c_stop(soft_i2c_bus_t *bus);

    /**
     * @brief  发送一个字节并等待应答
     * @param  bus  总线对象指针
     * @param  data 要发送的字节
     * @return SOFT_I2C_OK 或错误码(超时/NACK)
     */
    soft_i2c_err_t soft_i2c_write_byte(soft_i2c_bus_t *bus, uint8_t data);

    /**
     * @brief  读取一个字节并发送应答/非应答
     * @param  bus  总线对象指针
     * @param  ack  true: 主机应答(ACK), false:
     * 主机非应答(NACK)
     * @param  data 读取到的字节数据(输出)
     * @return 执行结果
     */
    soft_i2c_err_t soft_i2c_read_byte(soft_i2c_bus_t *bus, bool ack, uint8_t *data);

    /**
     * @brief  向指定从机地址写入多个字节(含寄存器地址)
     * @param  bus        总线对象指针
     * @param  dev_addr   7位从机地址
     * @param  reg_addr   寄存器地址(8位)，可为 NULL
     * 表示无寄存器地址
     * @param  reg_len    寄存器地址字节数(0~4)
     * @param  data       数据缓冲区
     * @param  len        数据长度
     * @return 执行结果
     */
    soft_i2c_err_t soft_i2c_master_write(soft_i2c_bus_t *bus, uint8_t dev_addr,
                                         const uint8_t *reg_addr, uint8_t reg_len,
                                         const uint8_t *data, uint16_t len);

    /**
     * @brief  从指定从机地址读取多个字节(含寄存器地址)
     * @param  bus        总线对象指针
     * @param  dev_addr   7位从机地址
     * @param  reg_addr   寄存器地址(8位)，可为 NULL
     * @param  reg_len    寄存器地址字节数(0~4)
     * @param  data       数据缓冲区(输出)
     * @param  len        数据长度
     * @return 执行结果
     */
    soft_i2c_err_t soft_i2c_master_read(soft_i2c_bus_t *bus, uint8_t dev_addr,
                                        const uint8_t *reg_addr, uint8_t reg_len, uint8_t *data,
                                        uint16_t len);

    /**
     * @brief  扫描 I2C 总线，检测所有应答设备的 7 位地址
     * @param  bus 已初始化的软件 I2C 总线对象指针
     * @return 找到的设备数量
     * @note   需要已配置好串口或 OLED 等输出设备以显示地址（可选）
     */
    uint8_t I2C_ScanBus(soft_i2c_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* __SOFT_I2C_H__ */
