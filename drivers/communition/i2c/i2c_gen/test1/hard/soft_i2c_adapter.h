/**
 * @file soft_i2c_adapter.h
 * @brief Software I2C adapter interface
 * @author Ziv-Xu
 * @date 2026-05-20
 * @note 有两种方式使用此适配器：
 *       1.外设的软件I2C函数名都统一的时候，直接使用如I2C_Start(void){soft_i2c_start(&i2c_bus);}的方式适配但是很多时候外设库函数名不统一，适配麻烦
 *       2.将外设使用到的软件I2C函数在适配器中实现，外设直接调用适配器函数，如OLED_I2C_Start(void){soft_i2c_adapter_start();}
 *                                                                       MPU6050_I2C_Start(void){soft_i2c_adapter_start();}
 *       这种方式适配更简单，外设库函数名不统一也不影响适配，缺点是增加了一层函数调用开销，可以应对不同的外设库有时候会返回不同的值。就是每增加一个外设都需要加入一些函数
 *       以下以第二种方式为例，实现OLED的适配，MPU6050的适配同理
 */

#ifndef __SOFT_I2C_ADAPTER_H__
#define __SOFT_I2C_ADAPTER_H__

#include <stdint.h>

void soft_i2c_adapter_init(void);

/*=============添加OLED示例=================*/

void Soft_I2C_Start();
void Soft_I2C_SendByte(uint8_t byte);
void Soft_I2C_WaitAck();
void Soft_I2C_Stop();

/*============= 添加 MPU6050 函数声明 =============*/
uint8_t mpu_I2C_Write_One_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data);
uint8_t mpu_I2C_Read_One_Byte(uint8_t dev_addr, uint8_t reg);
void mpu_I2C_Read_Buf(uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf);

#endif /* __SOFT_I2C_ADAPTER_H__ */