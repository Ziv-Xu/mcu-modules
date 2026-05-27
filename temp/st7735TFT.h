#ifndef _SPI_H
#define _SPI_H

#include "stm32f1xx_hal.h"

/* 接线定义（同原注释）*/
// GND   电源地
// VCC   接5V或3.3v电源
// SCL   接PA5
// SDA   接PA7
// RES   接PB0
// DC    接PB1
// CS    接PA4 
// BL    接PB10   若不控制背光，可不接

// 引脚宏定义（便于修改）
#define BDR_PORT      GPIOB  
#define BLK_PIN       GPIO_PIN_10
#define DC_PIN        GPIO_PIN_1
#define RST_PIN       GPIO_PIN_0

#define CDS_PORT      GPIOA 
#define SDA_PIN       GPIO_PIN_7
#define CS_PIN        GPIO_PIN_4 
#define SCL_PIN       GPIO_PIN_5

// 引脚电平控制（直接操作寄存器，保证高速 SPI）
#define CS_H          ((CDS_PORT)->ODR |= (CS_PIN))
#define CS_L          ((CDS_PORT)->ODR &= ~(CS_PIN))
#define SCL_H         ((CDS_PORT)->ODR |= (SCL_PIN))
#define SCL_L         ((CDS_PORT)->ODR &= ~(SCL_PIN))
#define SDA_H         ((CDS_PORT)->ODR |= (SDA_PIN))
#define SDA_L         ((CDS_PORT)->ODR &= ~(SDA_PIN))
#define RST_H         ((BDR_PORT)->ODR |= (RST_PIN))
#define RST_L         ((BDR_PORT)->ODR &= ~(RST_PIN))
#define DC_H          ((BDR_PORT)->ODR |= (DC_PIN))
#define DC_L          ((BDR_PORT)->ODR &= ~(DC_PIN))
#define BLK_H         ((BDR_PORT)->ODR |= (BLK_PIN))
#define BLK_L         ((BDR_PORT)->ODR &= ~(BLK_PIN))

// 颜色定义
#define RED     0xf800
#define GREEN   0x07e0
#define BLUE    0x001f
#define BLUE2   0x1c9f
#define PINK    0xd8a7
#define ORANGE  0xfa20
#define WHITE   0xffff
#define BLACK   0x0000
#define YELLOW  0xFFE0
#define CYAN    0x07ff
#define PURPLE  0xf81f
#define PURPLE2 0xdb92
#define PURPLE3 0x8811
#define GRAY0   0xEF7D
#define GRAY1   0x8410
#define GRAY2   0x4208



// 函数声明
void Spi_Init(void);
void Spi_SendData(uint8_t data);
void TFT_SendData(uint8_t Data);
void TFT_Send16Bit(uint16_t Data);
void TFT_SendIndex(uint8_t reg);
void TFT_SendReg(uint8_t adress, uint8_t data);
void TFT_Init(void);
void TFT_Reset(void);
void TFT_TurnOff(uint8_t io);
void TFt_SpinScreen(uint8_t locate);
void TFT_SetCursor(uint16_t x, uint16_t y);
void TFT_Clear(uint16_t color);
void TFT_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void TFT_FullScreen(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t Data);
void TFT_DrawCircle(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc);
void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Color);
void TFT_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bc);
void TFT_box2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t mode);
void ButtonDown(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ButtonUp(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void TFT_ShowImage(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const unsigned char *p);
void TFT_ShowChar(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char c);
void TFT_ShowString(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char *c);
void TFT_ShowNumber(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, long long num);
int map(char *c);
void TFT_ShowChinese(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char *c);

#endif