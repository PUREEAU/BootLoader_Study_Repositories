#ifndef __DRV_IIC_H
#define __DRV_IIC_H

#include "main.h"

#define IIC_SCL_PORT GPIOB
#define IIC_SCL_PIN_PORT GPIO_PIN_12
#define IIC_SDA_PORT GPIOB
#define IIC_SDA_PIN_PORT GPIO_PIN_14

#define IIC_SCL(x) HAL_GPIO_WritePin(IIC_SCL_PORT,IIC_SCL_PIN_PORT,(x?GPIO_PIN_SET:GPIO_PIN_RESET));
#define IIC_SDA(x) HAL_GPIO_WritePin(IIC_SDA_PORT,IIC_SDA_PIN_PORT,(x?GPIO_PIN_SET:GPIO_PIN_RESET));
#define IIC_R_SDA (HAL_GPIO_ReadPin(IIC_SDA_PORT,IIC_SDA_PIN_PORT) == GPIO_PIN_SET ? 1:0)

void IIC_Init(void);
void IIC_Start(void);
void IIC_WriteByte(uint8_t byte);
void IIC_Stop(void);
uint8_t IIC_ReadByte(void);
uint8_t IIC_ReadAck(void);
void IIC_SendAck(void);
void IIC_SendNoAck(void);

#endif
