#ifndef __DRV_SPI_H__
#define __DRV_SPI_H__

#include "main.h"   /* 根据工程实际头文件调整 */

#ifdef DEF_DRV_SPI_MODULE
    #define DEF_DRV_SPI_EXT
#else
    #define DEF_DRV_SPI_EXT extern
#endif

typedef enum {
    SPI_IO_RESET = 0,
    SPI_IO_SET   = 1
} spi_io_state;

/* SPI 引脚定义 */
#define SPI_CS_PORT        GPIOH
#define SPI_CS_PIN         GPIO_PIN_12

#define SPI_CLK_PORT       GPIOB
#define SPI_CLK_PIN        GPIO_PIN_13

#define SPI_DI_PORT        GPIOB
#define SPI_DI_PIN         GPIO_PIN_15

#define SPI_DO_PORT        GPIOD
#define SPI_DO_PIN         GPIO_PIN_9

/* 软件 SPI 控制宏 */
#define SPI_CS(x)   HAL_GPIO_WritePin(SPI_CS_PORT,  SPI_CS_PIN,  ((x) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define SPI_CLK(x)  HAL_GPIO_WritePin(SPI_CLK_PORT, SPI_CLK_PIN, ((x) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define SPI_DO(x)   HAL_GPIO_WritePin(SPI_DO_PORT,  SPI_DO_PIN,  ((x) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define SPI_DI_READ (HAL_GPIO_ReadPin(SPI_DI_PORT, SPI_DI_PIN) == GPIO_PIN_SET ? 1 : 0)
DEF_DRV_SPI_EXT void SPI_Init(void);
DEF_DRV_SPI_EXT void SPI_Start(void);
DEF_DRV_SPI_EXT void SPI_Stop(void);
DEF_DRV_SPI_EXT void SPI_WriteByte(uint8_t byte);
DEF_DRV_SPI_EXT uint8_t SPI_ReadByte(void);

#endif /* __DRV_SPI_H__ */