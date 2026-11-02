#include "at24c64.h"
#include "stdio.h"
/*
AT24C64存储器可存储8192个字节数据，
分为256页，每页32字节，随机字寻需要13位数据字
*/
void AT_WriteData(uint16_t addr, uint8_t Data){
    IIC_Start();
    IIC_WriteByte(0xA0);
    IIC_ReadAck();               
    IIC_WriteByte((addr>>8)&0xFF);
    IIC_ReadAck();               
    IIC_WriteByte(addr & 0xFF);
    IIC_ReadAck();               
    IIC_WriteByte(Data);
    IIC_ReadAck();               
    IIC_Stop();
}

uint8_t AT_Read_Data(uint16_t addr){
    IIC_Start();
    IIC_WriteByte(0xA0);
    IIC_ReadAck();
    IIC_WriteByte((addr >> 8) & 0xFF);
    IIC_ReadAck();
    IIC_WriteByte(addr & 0xFF);
    IIC_ReadAck();
    IIC_Stop();

    IIC_Start();
    IIC_WriteByte(0xA1);
    IIC_ReadAck();
    uint8_t data = IIC_ReadByte();
    IIC_SendNoAck();
    IIC_Stop();
    return data;
}
