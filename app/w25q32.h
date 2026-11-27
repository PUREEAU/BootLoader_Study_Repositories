/*
*********************************************************************************************************
*                                                 MODULE
*********************************************************************************************************
*/
#ifndef __W25Q32_H__
#define __W25Q32_H__

/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/
#ifdef DEF_APP_W25Q32_MODULE
    #define DEF_APP_W25Q32_EXT
#else
    #define DEF_APP_W25Q32_EXT extern
#endif

/*
*********************************************************************************************************
*                                          INCLUDE HEADER FILES
*********************************************************************************************************
*/
#include "drv_spi.h"

/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/
/* ========== W25Q 指令集 ========== */
/* 读类 */
#define W25X_ReadData               0x03    /* 读数据 */
#define W25X_FastRead               0x0B    /* 快速读 */
#define W25X_FastReadDual           0x3B    /* 双线快速读 */
#define W25X_FastReadQuad           0x6B    /* 四线快速读 */
#define W25X_FastReadDualIO         0xBB    /* 双线 I/O 快速读 */
#define W25X_FastReadQuadIO         0xEB    /* 四线 I/O 快速读 */

/* 写类 */
#define W25X_WriteEnable            0x06    /* 写使能 */
#define W25X_WriteDisable           0x04    /* 写禁止 */
#define W25X_PageProgram            0x02    /* 页编程 */
#define W25X_QuadPageProgram        0x32    /* 四线页编程 */

/* 擦除类 */
#define W25X_SectorErase            0x20    /* 4KB 扇区擦除 */
#define W25X_BlockErase32K          0x52    /* 32KB 块擦除 */
#define W25X_BlockErase64K          0xD8    /* 64KB 块擦除 */
#define W25X_ChipErase              0xC7    /* 整片擦除（也可用 0x60） */
#define W25X_ChipEraseAlt           0x60    /* 整片擦除备用命令 */

/* 状态寄存器 */
#define W25X_ReadStatusReg1         0x05    /* 读状态寄存器 1 */
#define W25X_ReadStatusReg2         0x35    /* 读状态寄存器 2 */
#define W25X_WriteStatusReg         0x01    /* 写状态寄存器 */

/* 电源 / 模式 */
#define W25X_PowerDown              0xB9    /* 掉电 */
#define W25X_ReleasePowerDown       0xAB    /* 释放掉电 / 读 Device ID */
#define W25X_HighPerformanceMode    0xA3    /* 高性能模式 */

/* 擦除/写入挂起与恢复 */
#define W25X_EraseSuspend           0x75    /* 擦除挂起 */
#define W25X_EraseResume            0x7A    /* 擦除恢复 */

/* ID 读取 */
#define W25X_ManufacturerDeviceID   0x90    /* 读厂商/设备 ID */
#define W25X_JedecDeviceID          0x9F    /* 读 JEDEC ID */
#define W25X_UniqueID               0x4B    /* 读 64-bit 唯一 ID */

/* 其他 */
#define W25X_EnableReset            0x66    /* 使能复位 */
#define W25X_ResetDevice            0x99    /* 复位芯片 */
#define W25X_ModeBitReset           0xFF    /* 复位模式位 */

/* 状态寄存器位 */
#define W25X_STATUS_BUSY            (1 << 0)
#define W25X_STATUS_WEL             (1 << 1)
#define W25X_STATUS_BP0             (1 << 2)
#define W25X_STATUS_BP1             (1 << 3)
#define W25X_STATUS_BP2             (1 << 4)
#define W25X_STATUS_TB              (1 << 5)
#define W25X_STATUS_SEC             (1 << 6)
#define W25X_STATUS_SRP0            (1 << 7)

/* ========== W25Q32 容量参数 ========== */
#define W25Q32_SIZE                 0x400000UL
#define W25Q32_PAGE_SIZE            0x100UL
#define W25Q32_SECTOR_SIZE          0x1000UL
#define W25Q32_BLOCK32_SIZE         0x8000UL
#define W25Q32_BLOCK64_SIZE         0x10000UL

/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/
/* ========== 擦除类型 ========== */
typedef enum {
    W25Q_ERASE_SECTOR_4K = 0,
    W25Q_ERASE_BLOCK_32K = 1,
    W25Q_ERASE_BLOCK_64K = 2,
    W25Q_ERASE_CHIP      = 3
} W25Q_EraseType;

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/
DEF_APP_W25Q32_EXT uint16_t W25Q64_ReadMID(void);
DEF_APP_W25Q32_EXT uint32_t W25Q_ReadJEDEC(void);
DEF_APP_W25Q32_EXT uint8_t W25Q_Erase_Range(uint32_t addr, uint32_t len);
/*
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*/
#endif /* __W25Q32_H__ */