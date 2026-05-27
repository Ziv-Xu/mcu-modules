/*
*author RanXin
*date 2026-05-27
*description 软件SPI接口定义，实现软件SPI
*硬件SPI目前还未尝试。
*除了MISO引脚，其他引脚都需要设置为推挽输出，MISO引脚设置为输入（一般配有上拉输入）。CS引脚的初始电平应该为HIGH。
*SPI设备在使用前需要先进行复位，复位方法是将RST引脚拉低至少10ms，然后再拉高。
warning 在使用SPI总线时，确保没有其他设备同时访问总线，以避免冲突和数据损坏。
        读指令还没有测试过，需谨慎。
*/

#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#include "stm32f1xx_hal.h"

#define BDR_PORT GPIOB
#define DC_PIN   GPIO_PIN_1
#define RST_PIN  GPIO_PIN_0

#define CDS_PORT GPIOA
#define SDA_PIN  GPIO_PIN_7 // MOSI引脚
#define MISO_PIN GPIO_PIN_6
#define CS_PIN   GPIO_PIN_4
#define SCL_PIN  GPIO_PIN_5

// 引脚电平控制（直接操作寄存器，保证高速 SPI）
#define CS_H  ((CDS_PORT)->ODR |= (CS_PIN))
#define CS_L  ((CDS_PORT)->ODR &= ~(CS_PIN))
#define SCL_H ((CDS_PORT)->ODR |= (SCL_PIN))
#define SCL_L ((CDS_PORT)->ODR &= ~(SCL_PIN))
#define SDA_H ((CDS_PORT)->ODR |= (SDA_PIN))
#define SDA_L ((CDS_PORT)->ODR &= ~(SDA_PIN))
#define RST_H ((BDR_PORT)->ODR |= (RST_PIN))
#define RST_L ((BDR_PORT)->ODR &= ~(RST_PIN))
#define DC_H  ((BDR_PORT)->ODR |= (DC_PIN))
#define DC_L  ((BDR_PORT)->ODR &= ~(DC_PIN))

// 读 MISO 引脚电平
#define MISO_READ ((CDS_PORT->IDR & MISO_PIN) ? 1 : 0)

void SPI_SendData(uint8_t data);
void SPI_SendIndex(uint8_t reg);
void SPI_SendData(uint8_t Data);
void SPI_SendReg(uint8_t adress, uint8_t data);
void SPI_Reset(void);

#endif
