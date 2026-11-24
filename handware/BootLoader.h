// #ifndef __BOOTLOADER_H
// #define __BOOTLOADER_H

// #include "main.h"
// #include "stdio.h"
// #include "string.h"
// #include "usart.h"
// #include "stdbool.h"

// #define BOOTLOADER_UART_RECEIVE_BUFF_SIZE 512
// #define BOOTLOADER_RX_QUEUE_SIZE 8000U
// #define BOOTLOADER_RX_IDLE_TIMEOUT_MS 1000U

// #define APP_START_ADDR 0x08008000
// #define APP_END_ADDR 0x08100000
// #define STACK_ADDR 0x20000000

// extern uint16_t bootLoaderReceiverBuffFullLength;
// extern volatile uint32_t bootLoaderUartOverrunCount;
// extern volatile uint8_t bootLoaderRxQueueOverflow;
// extern volatile uint8_t bootLoaderRxActive;
// extern volatile uint8_t bootLoaderFlashBusy;
// extern volatile uint8_t bootLoaderReportPending;

// void BootLoader_Inin(void);
// void BootLoader_Process(void);

// bool Int_BootLoader_JumpTo_Appliaction(void);

// #endif
