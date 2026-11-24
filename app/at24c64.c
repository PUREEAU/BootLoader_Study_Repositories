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

void AT_WriteDatas(uint16_t addr, uint8_t *data, uint32_t len){
    uint32_t i = 0;
    
    while(len > 0) {
        // 1. 计算当前页还能写多少
        uint8_t page_remain = AT24C_PAGE_SIZE - (addr % AT24C_PAGE_SIZE);
        uint32_t write_len = (len > page_remain) ? page_remain : len;
        
        // 2. 发起写操作
        IIC_Start();
        IIC_WriteByte(0xA0);
        IIC_ReadAck();
        
        IIC_WriteByte((addr >> 8) & 0xFF);
        IIC_ReadAck();
        IIC_WriteByte(addr & 0xFF);
        IIC_ReadAck();
        
        for(i = 0; i < write_len; i++) {
            IIC_WriteByte(data[i]);
            IIC_ReadAck();
        }
        IIC_Stop();
        
        // 3. 等待 EEPROM 内部烧录完成 (ACK 轮询)
        // 取代 HAL_Delay(5) 的死等，更加可靠
        do {
            IIC_Start();
            IIC_WriteByte(0xA0);
        } while (IIC_ReadAck() != 0); // 如果没有回 ACK，说明还在忙，继续循环
        IIC_Stop();
        
        // 4. 更新地址、数据指针和剩余长度
        addr += write_len;
        data += write_len;
        len  -= write_len;
    }
}

/**
 * @brief 连续读取 AT24Cxx 数据
 * @param addr 起始地址
 * @param data 接收数据缓冲区指针
 * @param len  读取长度（字节数）
 */
void AT_ReadDatas(uint16_t addr, uint8_t *data, uint32_t len){
    uint32_t i = 0;
    
    if(len == 0 || data == NULL) {
        return;
    }

    IIC_Start();
    IIC_WriteByte(0xA0);              // 发送写设备地址
    IIC_ReadAck();
    IIC_WriteByte((addr >> 8) & 0xFF);// 发送地址高八位
    IIC_ReadAck();
    IIC_WriteByte(addr & 0xFF);       // 发送地址低八位
    IIC_ReadAck();

    IIC_Start();                      // 再次发送 Start 信号 (有些库叫 IIC_Restart)
    IIC_WriteByte(0xA1);              // 发送读设备地址
    IIC_ReadAck();

    for(i = 0; i < len; i++) {
        data[i] = IIC_ReadByte();
        
        if(i == (len - 1)) {
            IIC_SendNoAck();          // 告诉 EEPROM 读取结束
        } else {
            IIC_SendAck();            // 告诉 EEPROM 继续发送下一个字节 (注意：请确保你底层有这个函数)
        }
    }

    IIC_Stop();
}