/*
*********************************************************************************************************
*                                                  W25Q32
*                                               W25Q32 Driver
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
*                                               W25Q32 Driver
* Filename      : w25q32.c
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
#define DEF_APP_W25Q32_MODULE

#include "w25q32.h"
#include "stdio.h"
#include "stdbool.h"
#include <string.h>

/*
*********************************************************************************************************
*                                              LOCAL DEFINES
*********************************************************************************************************
*/
// W25Q64指令集定义
#define W25X_WriteEnable        0x06    // 写使能
#define W25X_WriteDisable       0x04    // 写失能
#define W25X_ReadStatusReg1     0x05    // 读状态寄存器1
#define W25X_ReadStatusReg2     0x35    // 读状态寄存器2
#define W25X_WriteStatusReg     0x01    // 写状态寄存器
#define W25X_PageProgram        0x02    // 页编程
#define W25X_QuadPageProgram    0x32    // 四线页编程
#define W25X_BlockErase         0xD8    // 块擦除（64KB）
#define W25X_SectorErase        0x20    // 扇区擦除（4KB）
#define W25X_ChipErase          0xC7    // 整片擦除
#define W25X_PowerDown          0xB9    // 掉电模式
#define W25X_ReleasePowerDown   0xAB    // 唤醒
#define W25X_DeviceID           0xAB    // 设备ID
#define W25X_ManufactDeviceID   0x90    // 制造商ID
#define W25X_JedecDeviceID      0x9F    // JEDEC ID
#define W25X_ReadData           0x03    // 读取数据
#define W25X_FastRead           0x0B    // 快速读取
#define W25X_FastReadDual       0x3B    // 双线快速读取
#define W25X_FastReadQuad       0x6B    // 四线快速读取

#define W25Q32_SIZE            0x400000UL   /* 4MByte */
#define W25Q32_PAGE_SIZE       0x100UL      /* 256B   */
#define W25Q32_SECTOR_SIZE     0x1000UL     /* 4KB    */
#define W25Q32_BLOCK32_SIZE    0x8000UL     /* 32KB   */
#define W25Q32_BLOCK64_SIZE    0x10000UL    /* 64KB   */
#define W25Q32_PAGE_NUM        16384UL
#define W25Q32_SECTOR_NUM      1024UL
#define W25Q32_BLOCK64_NUM     64UL

/*
*********************************************************************************************************
*                                             LOCAL CONSTANTS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL DATA_TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              LOCAL TABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

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
uint16_t W25Q64_ReadMID(void){
    uint8_t man, dev;

    SPI_Start();
    SPI_WriteByte(W25X_ManufactDeviceID);
    SPI_WriteByte(0x00);
    SPI_WriteByte(0x00);
    SPI_WriteByte(0x00);

    man = SPI_ReadByte();
    dev = SPI_ReadByte();

    SPI_Stop();

    return ((uint16_t)man << 8) | dev;   // W25Q32
}

uint32_t W25Q_ReadJEDEC(void){
    uint8_t a, b, c;

    SPI_Start();
    SPI_WriteByte(W25X_JedecDeviceID);

    a = SPI_ReadByte();
    b = SPI_ReadByte();
    c = SPI_ReadByte();

    SPI_Stop();

    printf("JEDEC = %02X %02X %02X\r\n", a, b, c);

    return ((uint32_t)a << 16) | ((uint32_t)b << 8) | c;
}

static bool W25Q_Read_WaitBusy(void)
{
    uint32_t timeout = 0x1000UL;
    uint8_t  status;

    do
    {
        SPI_Start();
        SPI_WriteByte(W25X_ReadStatusReg1);   /* 0x05 */
        status = SPI_ReadByte();
        SPI_Stop();

        if (timeout-- == 0)
            return 0;                         /* 超时退出 */

    } while (status & 0x01);                  /* BUSY = bit0 */

    return 1;                                 /* 空闲，成功 */
}

/*
    功能：等待空闲 + 写使能（0x06）
    说明：
        1. 先等待芯片 BUSY 位清零；
        2. 再发送 0x06 置 WEL；
        3. 任何 Page Program / Erase / Write Status Register 之前调用。
    返回：
        1 = 成功
        0 = 等待空闲超时
*/
static uint8_t W25Q_Write_Enable(void)
{
    /* 必须先等空闲，否则 0x06 会被芯片忽略 */
    if (!W25Q_Read_WaitBusy())
        return 0;

    SPI_Start();
    SPI_WriteByte(W25X_WriteEnable);   /* 0x06 */
    SPI_Stop();

    return 1;
}

/*
    W25Q32 4M flash（32Mbit / 4MByte）
    总容量：0x400000 = 4MByte = 32Mbit
    地址范围：0x000000 ~ 0x3FFFFF（最大 22 位地址）

    区块大小（64KB Block）：
        共 64 个 Block，编号 0x00 ~ 0x3F
        每个 64KB = 0x10000
        最大 Block 起始地址 0x3F0000，即 0x3Fxxxx

    扇区大小（4KB Sector）：
        共 1024 个 Sector，编号 0x000 ~ 0x3FF
        每个 4KB = 0x1000
        最大 Sector 起始地址 0x3FF000，即 0x3FFxxx

    页大小（256B Page）：
        共 16384 个 Page，编号 0x0000 ~ 0x3FFF
        每个 256B = 0x100
        最大 Page 起始地址 0x3FFF00，即 0x3FFFxx

    最小擦除：
        4KB Sector Erase（0x20），最小擦除单位 = 0x1000
        32KB Block Erase（0x52）
        64KB Block Erase（0xD8）
        整片擦除（0xC7 / 0x60）

    读写限制：
        读：无页边界限制，可连续跨页/跨扇区
        写：Page Program（0x02），单次最多 256B，且不能跨页
        擦：最小 4KB 扇区
*/
uint8_t W25Q_Read_Data(uint32_t addr, uint8_t *buf, uint32_t len){
    uint32_t i;

    if(!W25Q_Read_WaitBusy())
        return 0;

    if (buf == 0)
        return 0;

    if (len == 0)
        return 1;

    /* 地址范围检查：W25Q32 最大地址 0x3FFFFF */
    if (addr >= W25Q32_SIZE)
        return 0;

    /* 防止 addr + len 溢出芯片范围 */
    if (len > (W25Q32_SIZE - addr))
        return 0;

    SPI_Start();

    /* 读数据命令 0x03，后面直接跟 24 位地址，无需 dummy */
    SPI_WriteByte(W25X_ReadData);

    /* 发送 24 位地址，高字节在前 */
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8)  & 0xFF);
    SPI_WriteByte(addr        & 0xFF);

    /* 连续读出 len 字节，读操作无页/扇区边界限制 */
    for (i = 0; i < len; i++)
    {
        buf[i] = SPI_ReadByte();
    }

    SPI_Stop();

    return 1;
}

/*
    功能：读取 W25Q32 一个字节
    参数：addr 地址 0x000000 ~ 0x3FFFFF
    返回：读到的数据；地址非法时返回 0xFF
*/
uint8_t W25Q_ReadByte(uint32_t addr){
    uint8_t data = 0xFF;

    if (W25Q_Read_Data(addr, &data, 1) == 0)
        return 0xFF;

    return data;
}

/*
    功能：页编程一页（最多 256 字节，不能跨页）
    参数：
        addr：页内任意起始地址（只需保证 addr + len <= 页尾）
        buf ：数据
        len ：1 ~ 256
    返回：1 成功，0 失败
*/
static uint8_t W25Q_PageProgram(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (buf == 0 || len == 0 || len > W25Q32_PAGE_SIZE)
        return 0;

    if (addr >= W25Q32_SIZE)
        return 0;

    if (len > (W25Q32_SIZE - addr))
        return 0;

    /* 等待空闲 + 写使能，合并成一步 */
    if (!W25Q_Write_Enable())
        return 0;

    SPI_Start();
    SPI_WriteByte(W25X_PageProgram);        /* 0x02 */
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8)  & 0xFF);
    SPI_WriteByte(addr        & 0xFF);

    for (i = 0; i < len; i++)
        SPI_WriteByte(buf[i]);

    SPI_Stop();

    /* 等待编程完成 */
    if (!W25Q_Read_WaitBusy())
        return 0;

    return 1;
}

/*
    功能：写入数据，按最小写入单元（256B 页）自动拆分跨页
    参数：
        addr：起始地址，0x000000 ~ 0x3FFFFF
        buf ：源数据
        len ：写入长度
    返回：
        1 = 全部成功
        0 = 参数错误 / 超时 / 中途失败

    注意：
        1. 目标区域必须先擦除，否则写入结果可能是 0xFF & 旧数据
        2. 单页最多 256B，跨页时自动拆分为多次页编程
        3. 不处理 buf 与芯片地址重叠等逻辑
*/
uint8_t W25Q_Write_Data(uint32_t addr, const uint8_t *buf, uint32_t len){
    uint32_t page_remain;
    uint16_t chunk;

    if (buf == 0 || len == 0)
        return 0;

    /* 地址范围检查 */
    if (addr >= W25Q32_SIZE)
        return 0;

    if (len > (W25Q32_SIZE - addr))
        return 0;

    while (len > 0)
    {
        /* 当前地址到所在页尾还剩多少字节 */
        page_remain = W25Q32_PAGE_SIZE - (addr % W25Q32_PAGE_SIZE);

        /* 本次最多写一页剩余空间 */
        chunk = (len > page_remain) ? (uint16_t)page_remain : (uint16_t)len;

        if (!W25Q_PageProgram(addr, buf, chunk))
            return 0;

        addr += chunk;
        buf  += chunk;
        len  -= chunk;
    }

    return 1;
}

/*
    功能：根据输入参数选择擦除扇区 / 32KB 块 / 64KB 块 / 整片
    参数：
        type：擦除类型
        addr：擦除起始地址（Chip Erase 时忽略）
    返回：
        1 = 成功
        0 = 参数错误 / 地址越界 / 超时
*/
uint8_t W25Q_Erase_Data(W25Q_EraseType type, uint32_t addr)
{
    uint8_t  cmd;
    uint32_t mask;

    switch (type)
    {
        case W25Q_ERASE_SECTOR_4K:
            cmd  = W25X_SectorErase;
            mask = W25Q32_SECTOR_SIZE - 1;
            break;
        case W25Q_ERASE_BLOCK_32K:
            cmd  = W25X_BlockErase32K;
            mask = W25Q32_BLOCK32_SIZE - 1;
            break;
        case W25Q_ERASE_BLOCK_64K:
            cmd  = W25X_BlockErase64K;
            mask = W25Q32_BLOCK64_SIZE - 1;
            break;
        case W25Q_ERASE_CHIP:
            cmd  = W25X_ChipErase;
            mask = 0;
            break;
        default:
            return 0;
    }

    if (type != W25Q_ERASE_CHIP)
    {
        if (addr >= W25Q32_SIZE)
            return 0;
        if (addr & mask)
            return 0;
    }

    /* 等空闲 + 0x06，一步完成 */
    if (!W25Q_Write_Enable())
        return 0;

    SPI_Start();
    SPI_WriteByte(cmd);

    if (type != W25Q_ERASE_CHIP)
    {
        SPI_WriteByte((addr >> 16) & 0xFF);
        SPI_WriteByte((addr >> 8)  & 0xFF);
        SPI_WriteByte(addr        & 0xFF);
    }
    SPI_Stop();

    return W25Q_Read_WaitBusy();
}

void W25Q_Test_CrossPage(void)
{
    printf("Enter CrossPage\r\n");   /* 如果这行都没有，说明前面就崩了 */
    #define TEST_ADDR   0x0000F0UL
    #define TEST_LEN    0x180UL        /* 384 字节，跨 3 页 */

    static uint8_t  wr_buf[TEST_LEN];
    static uint8_t  rd_buf[TEST_LEN];
    uint32_t i;
    uint32_t ok_cnt  = 0;
    uint32_t err_cnt = 0;

    printf("\r\n===== W25Q32 Cross Page Test =====\r\n");
    printf("Start Addr = 0x%06lX, Len = %lu bytes\r\n",
           (unsigned long)TEST_ADDR, (unsigned long)TEST_LEN);
    printf("Page split:\r\n");
    printf("  Page0: 0x0000F0 ~ 0x0000FF (16B)\r\n");
    printf("  Page1: 0x000100 ~ 0x0001FF (256B)\r\n");
    printf("  Page2: 0x000200 ~ 0x00026F (112B)\r\n");

    /* ---------- 1. 准备测试数据：0x00 ~ 0x17F 循环 ---------- */
    for (i = 0; i < TEST_LEN; i++)
    {
        wr_buf[i] = (uint8_t)(i & 0xFF);
    }

    /* ---------- 2. 擦除扇区 0 ---------- */
    printf("\r\n[1] Erase Sector 0 (0x000000)...\r\n");
    if (!W25Q_Erase_Data(W25Q_ERASE_SECTOR_4K, 0x000000))
    {
        printf("    Erase FAILED!\r\n");
        return;
    }
    printf("    Erase OK\r\n");

    /* ---------- 3. 擦除后确认全为 0xFF ---------- */
    printf("\r\n[2] Read after erase (expect 0xFF)...\r\n");
    memset(rd_buf, 0, sizeof(rd_buf));
    if (!W25Q_Read_Data(TEST_ADDR, rd_buf, TEST_LEN))
    {
        printf("    Read FAILED!\r\n");
        return;
    }
    for (i = 0; i < TEST_LEN; i++)
    {
        if (rd_buf[i] != 0xFF) err_cnt++;
    }
    printf("    Non-0xFF bytes: %lu\r\n", (unsigned long)err_cnt);
    err_cnt = 0;

    /* ---------- 4. 跨页写入 ---------- */
    printf("\r\n[3] Write %lu bytes (auto split across 3 pages)...\r\n",
           (unsigned long)TEST_LEN);
    if (!W25Q_Write_Data(TEST_ADDR, wr_buf, TEST_LEN))
    {
        printf("    Write FAILED!\r\n");
        return;
    }
    printf("    Write OK\r\n");

    /* ---------- 5. 读回并逐字节对比 ---------- */
    printf("\r\n[4] Read back and verify...\r\n");
    memset(rd_buf, 0, sizeof(rd_buf));
    if (!W25Q_Read_Data(TEST_ADDR, rd_buf, TEST_LEN))
    {
        printf("    Read FAILED!\r\n");
        return;
    }

    for (i = 0; i < TEST_LEN; i++)
    {
        if (rd_buf[i] == wr_buf[i])
        {
            ok_cnt++;
        }
        else
        {
            err_cnt++;
            if (err_cnt <= 8)
            {
                printf("    [MISMATCH] idx=%lu addr=0x%06lX wr=0x%02X rd=0x%02X\r\n",
                       (unsigned long)i,
                       (unsigned long)(TEST_ADDR + i),
                       wr_buf[i], rd_buf[i]);
            }
        }
    }

    printf("\r\n[5] Result:\r\n");
    printf("    Match    : %lu\r\n", (unsigned long)ok_cnt);
    printf("    Mismatch : %lu\r\n", (unsigned long)err_cnt);

    if (err_cnt == 0)
        printf("\r\n===== TEST PASS =====\r\n");
    else
        printf("\r\n===== TEST FAIL =====\r\n");

    /* ---------- 6. 打印首尾各 32 字节，便于肉眼对照 ---------- */
    printf("\r\n[6] First 32 bytes @0x%06lX:\r\n", (unsigned long)TEST_ADDR);
    for (i = 0; i < 32; i++)
    {
        printf("%02X ", rd_buf[i]);
        if ((i & 15) == 15) printf("\r\n");
    }

    printf("\r\n[7] Last 32 bytes @0x%06lX:\r\n",
           (unsigned long)(TEST_ADDR + TEST_LEN - 32));
    for (i = TEST_LEN - 32; i < TEST_LEN; i++)
    {
        printf("%02X ", rd_buf[i]);
        if (((i - (TEST_LEN - 32)) & 15) == 15) printf("\r\n");
    }
    printf("\r\n");
}

void W25Q_Test_Mini(void)
{
    uint8_t w[4] = {0xAA, 0x55, 0x12, 0x34};
    uint8_t r[4] = {0};

    printf("Mini test start\r\n");

    printf("Erase 0x000000...\r\n");
    if (!W25Q_Erase_Data(W25Q_ERASE_SECTOR_4K, 0x000000)) {
        printf("Erase FAIL\r\n"); return;
    }
    printf("Erase OK\r\n");

    printf("Write 4 bytes @0x000000...\r\n");
    if (!W25Q_Write_Data(0x000000, w, 4)) {
        printf("Write FAIL\r\n"); return;
    }
    printf("Write OK\r\n");

    W25Q_Read_Data(0x000000, r, 4);
    printf("Read back: %02X %02X %02X %02X\r\n", r[0], r[1], r[2], r[3]);

    if (r[0]==0xAA && r[1]==0x55 && r[2]==0x12 && r[3]==0x34)
        printf("Mini TEST PASS\r\n");
    else
        printf("Mini TEST FAIL\r\n");
}

/*
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*/