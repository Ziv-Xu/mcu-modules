/**
 * @file    board_i2c_pins.h
 * @brief   开发板 I2C 引脚集中配置
 * @author  XuChengShuo
 * @date    2026-05-25
 * @note    换开发板时，只需要修改这个文件里的 4 行宏定义
 *          所有共用该 I2C 总线的外设（EEPROM、OLED 等）自动适配。
 *
 *          使用方法:
 *          1. 根据实际原理图修改 I2C_SCL_PORT / I2C_SCL_PIN / I2C_SDA_PORT / I2C_SDA_PIN
 *          2. 在 soft_i2c_simple.c 中 #include 本文件，用宏定义赋值 i2c_oled
 *          3. EEPROM 驱动自动引用本文件生成共享总线
 *
 *          当前配置（只是示例，供更改）: EEPROM + OLED 共用 PB6(SCL) / PB7(SDA)
 */

#ifndef __BOARD_I2C_PINS_H__
#define __BOARD_I2C_PINS_H__

#include "stm32f1xx_hal.h"

/*===========================================================================
 * 如果需要换开发板时，只改下面这 4 行宏定义即可
 *===========================================================================*/

#define I2C_SCL_PORT GPIOB      /**< SCL 引脚端口号 */
#define I2C_SCL_PIN  GPIO_PIN_6 /**< SCL 引脚编号   */
#define I2C_SDA_PORT GPIOB      /**< SDA 引脚端口号 */
#define I2C_SDA_PIN  GPIO_PIN_7 /**< SDA 引脚编号   */

/*===========================================================================
 * 自动生成总线对象初始化宏 (供 soft_i2c_simple.c 引用)
 *===========================================================================*/

/**
 * @brief  用于初始化 SoftI2C_Obj 结构体的复合字面量
 * @note   在 C99 中可用，用法:
 *         SoftI2C_Obj i2c_shared = I2C_PINS_INITIALIZER;
 *         或直接:
 *         SoftI2C_Obj i2c_oled = I2C_PINS_INITIALIZER;
 */
#define I2C_PINS_INITIALIZER \
    {.scl_port = I2C_SCL_PORT, .scl_pin = I2C_SCL_PIN, .sda_port = I2C_SDA_PORT, .sda_pin = I2C_SDA_PIN, .delay = NULL}

/**
 * @brief  I2C 总线对象名 — 所有共用该总线的外设都引用这个
 * @note   在 at24cxx.c / at24cxx_test.c 中 extern 引用
 */
#define I2C_SHARED_BUS i2c_shared

#endif /* __BOARD_I2C_PINS_H__ */
