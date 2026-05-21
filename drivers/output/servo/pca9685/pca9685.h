#ifndef __PCA9685_H
#define __PCA9685_H

#include "stm32f1xx_hal.h"
#include "soft_i2c_simple.h"

// PCA9685 的 I2C 地址（7位地址 0x40，左移1位为 0x80）
#define PCA9685_ADDR 0x80

// 寄存器地址
#define PCA9685_MODE1      0x00
#define PCA9685_MODE2      0x01
#define PCA9685_PRE_SCALE  0xFE
#define PCA9685_LED0_ON_L  0x06 // 通道0 ON 低字节
#define PCA9685_LED0_ON_H  0x07
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_LED0_OFF_H 0x09
// 每个通道间隔4个寄存器

extern SoftI2C_Obj i2c_pca9685; // 你在 soft_i2c_simple.c 中已定义

// 宏定义：直接使用你的软I2C函数
#define IIC_Start()      SoftI2C_Start(&i2c_pca9685)
#define IIC_Send_Byte(d) SoftI2C_SendByte(&i2c_pca9685, (d))
#define IIC_Wait_Ack()   SoftI2C_WaitAck(&i2c_pca9685)
#define IIC_Stop()       SoftI2C_Stop(&i2c_pca9685)

// 公开函数
void pca9685_Init(void);                                    // 初始化：设置50Hz，唤醒芯片
void pca9685_SetPWMFreq(float freq_hz);                     // 设置PWM频率（如50Hz）
void pca9685_SetPWM(uint8_t channel, uint16_t off);         // 设置某通道PWM占空比（12位，0~4095）
void pca9685_SetServoAngle(uint8_t channel, uint8_t angle); // 舵机角度（0~180°）
void pca9685_Reset(void);                                   // 软件复位（正确方式）
void pca9685_AllOff(void);                                  // 所有通道输出0（舵机归零）

#endif
