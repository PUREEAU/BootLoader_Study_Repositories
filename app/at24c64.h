#ifndef __AT24C64_H
#define __AT24C64_H

#include "drv_iic.h"

void AT_WriteData(uint16_t addr, uint8_t Data);
uint8_t AT_Read_Data(uint16_t addr);

#endif
