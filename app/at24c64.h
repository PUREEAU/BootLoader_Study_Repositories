#ifndef __AT24C64_H__
#define __AT24C64_H__

#define AT24C_PAGE_SIZE 32

#include "drv_iic.h"

void AT_WriteData(uint16_t addr, uint8_t Data);
uint8_t AT_Read_Data(uint16_t addr);

void AT_WriteDatas(uint16_t addr, uint8_t *data, uint32_t len);
void AT_ReadDatas(uint16_t addr, uint8_t *data, uint32_t len);

#endif
