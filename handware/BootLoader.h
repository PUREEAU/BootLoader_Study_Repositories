#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "usart.h"


#define BOOTLOADER_UART_RECEIVE_BUFF_SIZE 512

//设置B区大小 16K 0x4000 ; 则A区大小为 0xFC000
#define BOOTLOADER_APP_ADDER 0x08004000

extern uint16_t bootLoaderReceiverBuffFullLength;

void BootLoader_Inin(void);



#endif
