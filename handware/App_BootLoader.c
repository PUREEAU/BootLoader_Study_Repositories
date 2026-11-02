#include "App_BootLoader.h"
#include "stdbool.h"
uint8_t app_rec_start_buf[64]={0};
uint16_t app_rec_start_len;
uint32_t app_rec_alldata_len;
static bool Jump_flah = 0;
typedef enum{
    BOOTLOADER_STATUS_INIT,
    BOOTLOADER_STATUS_RUN,
    BOOTLOADER_STATUS_RECDATE,
    BOOTLOADER_STATUS_CHECKDATE,
    BOOTLOADER_STATUS_JUMPAPP
}bootloader_status;
bootloader_status bootloaderstatus = BOOTLOADER_STATUS_INIT;
void App_BootLoader_Init(void){
    static bool initflag=0;
    if(!initflag){
        printf("bootlader start\r\n");
        printf("wait user send data\r\n");
        printf("send 'start:len' to start\r\n");
        initflag = 1;
    }
    HAL_UARTEx_ReceiveToIdle(&huart1,app_rec_start_buf,64,&app_rec_start_len,0xffffff);
    char *start_str = strstr((char *)app_rec_start_buf,"start:");
    if(app_rec_start_len>0){
        if(start_str !=NULL){
            app_rec_alldata_len = atoi((char*)start_str+6);
            if(app_rec_alldata_len){
                printf("start len:%d\r\n",app_rec_alldata_len);
                bootloaderstatus = BOOTLOADER_STATUS_RUN;
            }else{
                printf("len err\r\n");
            }
        }else{
            printf("data err\r\n");
        }
    }
}

bool App_Bootloader_CheckDate(void){
    if(app_rec_alldata_len == bootLoaderReceiverBuffFullLength){
        printf("bootLoaderReceiverBuffFullLength:%d \r\n",bootLoaderReceiverBuffFullLength);
        bootloaderstatus = BOOTLOADER_STATUS_JUMPAPP;
    }else{
        printf("bootLoaderReceiverBuffFullLength error \r\n");
        return 1;
    }
    return 0;
}

void App_BootLoader_Work(void){
    switch (bootloaderstatus)
    {
    case BOOTLOADER_STATUS_INIT:{
        App_BootLoader_Init();
    }
        break;
    case BOOTLOADER_STATUS_RUN:{
        printf("Bootloader run OK\r\n");
        BootLoader_Inin();
        bootloaderstatus = BOOTLOADER_STATUS_RECDATE;
    }
    case BOOTLOADER_STATUS_RECDATE:{
        BootLoader_Process();
        if ((bootLoaderReportPending != 0U) &&
            (bootLoaderRxActive == 0U) &&
            (bootLoaderFlashBusy == 0U))
        {
            bootLoaderReportPending = 0U;
            printf("buuf:%u ore:%lu overflow:%u\r\n",
                bootLoaderReceiverBuffFullLength,
                (unsigned long)bootLoaderUartOverrunCount,
                bootLoaderRxQueueOverflow);
            bootloaderstatus = BOOTLOADER_STATUS_CHECKDATE;
            Jump_flah = 0;
        }
    }
        break;
    case BOOTLOADER_STATUS_CHECKDATE:{
       if(App_Bootloader_CheckDate()){
            printf("NVIC_SystemReset\r\n");
            NVIC_SystemReset();
       }
    }
        break;
    case BOOTLOADER_STATUS_JUMPAPP:{
        if(Jump_flah){
            printf("Jump App \r\n");
            if(Int_BootLoader_JumpTo_Appliaction()){
                printf("BOOTLOADER_STATUS_JUMPAPP error\r\n");
                NVIC_SystemReset();
            }
        }
    }
        break;
    default:
        break;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == GPIO_PIN_0){
        Jump_flah = 1;
    }
}