/*
*********************************************************************************************************
*                                                  app_bootloader
*                                               app_bootloader Application
*                  ****************************************************************
*                  ****************************************************************
*
* LICENSING TERMS:
* ---------------
*               TBD
*               You can contact us at PUREEAU@qq.com.
*
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                               app_bootloader Application
* Filename      : app_bootloader.c
* Version       : V1.00.00
* Programmer(s) : PUREEAU
*********************************************************************************************************
* Note(s)       : TBD
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/
#define DEF_APP_BOOTLOADER_MODULE

#include "app_bootloader.h"
#include "usart.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
/*
*********************************************************************************************************
*                                              LOCAL DEFINES
*********************************************************************************************************
*/

#define BOOTLOADER_UART_RECEIVE_BUFF_SIZE 512

#define UPGRADE_KEY_START_LENGTH    "StartLength:"
#define UPGRADE_KEY_CRC32           "CRC32:"
#define UPGRADE_UNIT_BYTE           'B'
/*
*********************************************************************************************************
*                                             LOCAL CONSTANTS
*********************************************************************************************************
*/

uint8_t bootLoaderReceiverBuff[BOOTLOADER_UART_RECEIVE_BUFF_SIZE];

/*
*********************************************************************************************************
*                                            LOCAL DATA_TYPES
*********************************************************************************************************
*/

APP_BOOTLOADER_RECEIVE_STATE bootloader_state = BOOTLOADER_RECEIVE_INIT;
APP_UPGRADE_INFO_T g_upgrade_info = {0};
/*
*********************************************************************************************************
*                                              LOCAL TABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* 上位机发送数据
  StartLength:32768B;CRC32:0xAFD4A3DF
*/
static bool Parse_Upgrade_Info(const uint8_t *buf)
{
    const char *p = (const char *)buf;          /* 只转这一次 */

    const char *p_len = strstr(p, UPGRADE_KEY_START_LENGTH);
    const char *p_crc = strstr(p, UPGRADE_KEY_CRC32);
    if (p_len == NULL || p_crc == NULL) {
        return false;
    }

    char *endptr = NULL;

    p_len += strlen(UPGRADE_KEY_START_LENGTH);
    unsigned long len = strtoul(p_len, &endptr, 10);
    if (endptr == p_len || *endptr != UPGRADE_UNIT_BYTE) {
        return false;
    }

    p_crc += strlen(UPGRADE_KEY_CRC32);
    unsigned long crc = strtoul(p_crc, &endptr, 16);
    if (endptr == p_crc) {
        return false;
    }

    g_upgrade_info.firmware_len = (uint32_t)len;
    g_upgrade_info.crc_value    = (uint32_t)crc;
    return true;
}
static bool Save_Upgrade_Info(void){
    
    return true;
}
/*
*********************************************************************************************************
*                                       LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       GLOBAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            GLOBAL FUNCTIONS
*********************************************************************************************************
*/

void App_BootLoader_Receive_Work(void){

    switch (bootloader_state){
        case BOOTLOADER_RECEIVE_INIT:{
            memset(bootLoaderReceiverBuff,'\0',BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
            HAL_UARTEx_ReceiveToIdle_IT(&huart1, bootLoaderReceiverBuff, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
            bootloader_state = BOOTLOADER_RECEIVE_IDEL;
        }
            break;
        case BOOTLOADER_RECEIVE_IDEL:{
            //空闲状态，等待中断设置 BOOTLOADER_RECEIVE_UPGRADE_INFO
        }
            break;
        case BOOTLOADER_RECEIVE_UPGRADE_INFO:{
            if(Parse_Upgrade_Info(bootLoaderReceiverBuff)){
                bootloader_state = BOOTLOADER_SAVE_UPGRADE_INFO;
            }else{
                memset(bootLoaderReceiverBuff,'\0',BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
                bootloader_state = BOOTLOADER_RECEIVE_IDEL;
            }
        }
            break;
        case BOOTLOADER_SAVE_UPGRADE_INFO:{

        }
            break;
        default:
            break;
    }


}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        if(bootloader_state == BOOTLOADER_RECEIVE_IDEL){
            bootloader_state = BOOTLOADER_RECEIVE_UPGRADE_INFO;
        }
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, bootLoaderReceiverBuff, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
    }
}



/*
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*/
