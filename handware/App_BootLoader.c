#include "App_BootLoader.h"
uint8_t app_rec_start_buf[64]={0};
uint16_t app_rec_start_len;
uint32_t app_rec_alldata_len;
void App_BootLoader_Init(void){

    printf("bootlader start\r\n");
    printf("wait user send data\r\n");
    printf("send 'start:len' to start\r\n");

    while (1)
    {
        HAL_UARTEx_ReceiveToIdle(&huart1,app_rec_start_buf,64,&app_rec_start_len,0xffffff);
        char *start_str = strstr((char *)app_rec_start_buf,"start:");
        if(app_rec_start_len>0){
            if(start_str !=NULL){
                app_rec_alldata_len = atoi((char*)start_str+6);
                if(app_rec_alldata_len){
                    printf("start len:%d\r\n",app_rec_alldata_len);
                    break;
                }else{
                    printf("len err\r\n");
                }
            }else{
                printf("data err\r\n");
            }
        }

    }
    
}
