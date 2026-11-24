#define DEF_DRV_SPI_MODER

#include "drv_spi.h"

void SPI_Init(void){
    SPI_CS(SPI_IO_SET);
    SPI_CLK(SPI_IO_RESET);
}

void SPI_Start(void){
    SPI_CS(SPI_IO_RESET);
}

void SPI_Stop(void){
    SPI_CS(SPI_IO_SET);
}

uint8_t SPI_ReadWriteByte(uint8_t byte){
    uint8_t i,receive_data = 0;
    SPI_CLK(SPI_IO_RESET);

    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80)
            SPI_DO(SPI_IO_SET);
        else
            SPI_DO(SPI_IO_RESET);
        byte <<= 1;
        SPI_CLK(SPI_IO_SET);
        receive_data <<= 1;
        if (SPI_DI_READ)
            receive_data |= 0x01;
        SPI_CLK(SPI_IO_RESET);
    }
    return receive_data;
}

void SPI_WriteByte(uint8_t byte)
{
    SPI_ReadWriteByte(byte);
}

uint8_t SPI_ReadByte(void)
{
    return SPI_ReadWriteByte(0xFF);  // 发送哑元数据0xFF来读取从机数据
}