#include "drv_iic.h"

void IIC_Init(void){
    IIC_SCL(1);
    IIC_SDA(1);
}

void IIC_Start(void){
    IIC_SDA(1);
    HAL_Delay(1);
    IIC_SCL(1);
    HAL_Delay(1);
    IIC_SDA(0);
    HAL_Delay(1);
    IIC_SCL(0);
    HAL_Delay(1);
}

void IIC_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        IIC_SDA((byte & (0x80 >> i)) ? 1 : 0);
        HAL_Delay(1);
        IIC_SCL(1);
        HAL_Delay(1);
        IIC_SCL(0);
        HAL_Delay(1);
    }
}

void IIC_Stop(void){
    IIC_SDA(0);
    HAL_Delay(1);
    IIC_SCL(1);
    HAL_Delay(1);
    IIC_SDA(1);
    HAL_Delay(1);
}

uint8_t IIC_ReadByte(void){
    uint8_t byte = 0;
    IIC_SDA(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        IIC_SCL(1);
        if(IIC_R_SDA){
            byte |= (0x80 >> i);
        }
        IIC_SCL(0);
    }
    return byte;
}

uint8_t IIC_ReadAck(void)
{
    uint8_t ack;
    IIC_SDA(1);
    IIC_SCL(1);
    ack = IIC_R_SDA;
    IIC_SCL(0);
    return ack;
}

void IIC_SendAck(void){
    IIC_SDA(0);
    IIC_SCL(1);
    IIC_SCL(0);
}

void IIC_SendNoAck(void){
    IIC_SDA(1);
    IIC_SCL(1);
    IIC_SCL(0);
}