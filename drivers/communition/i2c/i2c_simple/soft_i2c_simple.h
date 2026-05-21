#ifndef __SOFT_I2C_OBJ_H
#define __SOFT_I2C_OBJ_H

#include "stm32f1xx_hal.h" /* 按你实际 MCU 系列调整 */

/* I2C 方向宏，方便构建器件地址 */
#define SOFT_I2C_WRITE 0
#define SOFT_I2C_READ  1

/**
 * @brief 软 I2C 总线对象
 * @note  使用前填充 scl_port/scl_pin/sda_port/sda_pin，
 *        延时函数 delay 可置为 NULL（使用默认延时）或提供自己的忙等待函数。
 */
typedef struct
{
    GPIO_TypeDef *scl_port;
    uint16_t      scl_pin;
    GPIO_TypeDef *sda_port;
    uint16_t      sda_pin;
    void (*delay)(void); /* 若为 NULL 则使用内部默认延时 */
} SoftI2C_Obj;

/*=================== 基础时序 ====================*/
void    SoftI2C_Init(SoftI2C_Obj *obj);
void    SoftI2C_Start(SoftI2C_Obj *obj);
void    SoftI2C_Stop(SoftI2C_Obj *obj);
uint8_t SoftI2C_WaitAck(SoftI2C_Obj *obj); /* 返回0：应答，1：无应答 */
void    SoftI2C_Ack(SoftI2C_Obj *obj);
void    SoftI2C_NAck(SoftI2C_Obj *obj);
void    SoftI2C_SendByte(SoftI2C_Obj *obj, uint8_t data);
uint8_t SoftI2C_ReadByte(SoftI2C_Obj *obj, uint8_t ack); /* ack=0:NACK; ack!=0:ACK */

/*=================== 常用读写接口 ====================*/
uint8_t SoftI2C_WriteOneByte(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg,
                             uint8_t data);                                   /* 返回0成功 */
uint8_t SoftI2C_ReadOneByte(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg); /* 返回读到的值 */
void SoftI2C_WriteBuf(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf);
void SoftI2C_ReadBuf(SoftI2C_Obj *obj, uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf);

uint8_t I2C_ScanBus(SoftI2C_Obj *obj);

#endif
