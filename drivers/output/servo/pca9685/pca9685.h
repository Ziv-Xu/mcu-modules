#ifndef __PCA_H
#define __PCA_H
#include "stm32f10x.h"

#define pca_adr1 0x80

extern uint8_t data_20[20];

void pca_write20(uint8_t pca_adr);
void pca_reset(uint8_t pca_adr);
void pca_setpwm(uint8_t pca_adr, uint8_t num, uint8_t off);

#endif
