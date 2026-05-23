#ifndef __MYCAN2_H
#define __MYCAN2_H

#include "stm32f1xx_hal.h"

extern CAN_RxHeaderTypeDef MyCAN_RxHeader;  // 接收消息头
extern uint8_t MyCAN_RxData[8];             // 接收数据缓冲区
extern uint8_t MyCAN_RxFlag;                // 接收完成标志

void MyCAN_Init(void);
void MyCAN_Transmit(uint32_t Id, uint8_t IdType, uint8_t *pData, uint8_t DLC);

#endif
