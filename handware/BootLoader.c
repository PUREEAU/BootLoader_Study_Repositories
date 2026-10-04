#include "BootLoader.h"
//13576字节
uint8_t bootLoaderReceiverBuff[BOOTLOADER_UART_RECEIVE_BUFF_SIZE] = {'\0'};
uint16_t bootLoaderReceiverBuffLength;
uint16_t bootLoaderReceiverBuffFullLength;
uint32_t flash_WriteOffset = 0;
static int32_t last_erased_sector = -1;
static uint8_t pending_flash_byte;
static uint32_t pending_flash_addr;
static uint8_t pending_flash_byte_valid = 0;

uint32_t GetSector(uint32_t Address)
{
    uint32_t sector = 0;
    if ((Address >= 0x08000000) && (Address < 0x08004000))      sector = FLASH_SECTOR_0;
    else if ((Address >= 0x08004000) && (Address < 0x08008000)) sector = FLASH_SECTOR_1;
    else if ((Address >= 0x08008000) && (Address < 0x0800C000)) sector = FLASH_SECTOR_2;
    else if ((Address >= 0x0800C000) && (Address < 0x08010000)) sector = FLASH_SECTOR_3;
    else if ((Address >= 0x08010000) && (Address < 0x08020000)) sector = FLASH_SECTOR_4;
    else if ((Address >= 0x08020000) && (Address < 0x08040000)) sector = FLASH_SECTOR_5;
    else if ((Address >= 0x08040000) && (Address < 0x08060000)) sector = FLASH_SECTOR_6;
    else if ((Address >= 0x08060000) && (Address < 0x08080000)) sector = FLASH_SECTOR_7;
    else if ((Address >= 0x08080000) && (Address < 0x080A0000)) sector = FLASH_SECTOR_8;
    else if ((Address >= 0x080A0000) && (Address < 0x080C0000)) sector = FLASH_SECTOR_9;
    else if ((Address >= 0x080C0000) && (Address < 0x080E0000)) sector = FLASH_SECTOR_10;
    else if ((Address >= 0x080E0000) && (Address < 0x08100000)) sector = FLASH_SECTOR_11;
    return sector;
}
void BootLoader_WriteChunk(uint32_t offset, uint8_t *buf, uint16_t len)
{
    uint32_t addr     = BOOTLOADER_APP_ADDER + offset;
    uint32_t end_addr = addr + len - 1;

    uint32_t first_sector = GetSector(addr);
    uint32_t last_sector  = GetSector(end_addr);

    HAL_FLASH_Unlock();
    // 只擦除尚未擦除过的扇区
    for (int32_t s = first_sector; s <= last_sector; s++)
    {
        if (s > last_erased_sector)   // 关键判断！
        {
            FLASH_EraseInitTypeDef erase_typedef;
            erase_typedef.TypeErase    = FLASH_TYPEERASE_SECTORS;
            erase_typedef.Banks        = FLASH_BANK_1;
            erase_typedef.Sector       = s;
            erase_typedef.NbSectors    = 1;              // 一次只擦一个扇区
            erase_typedef.VoltageRange = FLASH_VOLTAGE_RANGE_3;

            uint32_t sector_error = 0;
            if (HAL_FLASHEx_Erase(&erase_typedef, &sector_error) != HAL_OK)
            {
                HAL_FLASH_Lock();
                printf("last_erased_sector error:%d",last_erased_sector);
                return;   // 擦除失败
            }
            last_erased_sector = s;   // 更新已擦除到哪个扇区
            printf("last_erased_sector:%d",last_erased_sector);
        }
    }

    // 写入数据（此时目标区域一定是 0xFF）
    /*
     * Half-word programming consumes two consecutive bytes. Keep an odd
     * trailing byte until the next chunk arrives; do not pad it with 0xFF.
     */
    uint16_t i = 0U;

    if (pending_flash_byte_valid)
    {
        if ((len == 0U) || (pending_flash_addr != (addr - 1U)))
        {
            HAL_FLASH_Lock();
            return;
        }

        uint16_t halfword = (uint16_t)pending_flash_byte |
                            ((uint16_t)buf[0] << 8U);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              pending_flash_addr,
                              halfword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return;
        }

        pending_flash_byte_valid = 0U;
        i = 1U;
    }
    else if ((addr & 0x01U) != 0U)
    {
        HAL_FLASH_Lock();
        return;
    }

    for (; (i + 1U) < len; i += 2U)
    {
        uint16_t halfword = (uint16_t)buf[i] |
                            ((uint16_t)buf[i + 1U] << 8U);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              addr + i,
                              halfword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return;
        }
    }

    if (i < len)
    {
        pending_flash_byte = buf[i];
        pending_flash_addr = addr + i;
        pending_flash_byte_valid = 1U;
    }
    HAL_FLASH_Lock();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        // 把收到的数据写进 Flash
        BootLoader_WriteChunk(flash_WriteOffset, bootLoaderReceiverBuff, Size);
        flash_WriteOffset += Size;
        memset(bootLoaderReceiverBuff, 0, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
        __HAL_UART_CLEAR_OREFLAG(&huart1);
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, bootLoaderReceiverBuff, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
    }
}

void BootLoader_Inin(void){

    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);

    HAL_UARTEx_ReceiveToIdle_IT(&huart1,bootLoaderReceiverBuff,BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
}

int fputc(int ch,FILE * f){
    HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,50);
    return ch;
}

