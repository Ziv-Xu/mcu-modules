#ifndef __MyCAN1_H
#define __MyCAN1_H


#include <stdint.h>

void MyCAN_Init(void);
void MyCAN_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data);
uint8_t MyCAN_ReceiveFlag(void);
void MyCAN_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data);


#endif /* __MyCAN1_H */
