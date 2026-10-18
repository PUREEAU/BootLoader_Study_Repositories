#include "BootLoader.h"
//13576瀛楄妭
uint8_t bootLoaderReceiverBuff[BOOTLOADER_UART_RECEIVE_BUFF_SIZE] = {'\0'};
uint16_t bootLoaderReceiverBuffLength;
uint16_t bootLoaderReceiverBuffFullLength;
static uint8_t bootLoaderRxQueue[BOOTLOADER_RX_QUEUE_SIZE];
static volatile uint16_t bootLoaderRxQueueWrite;
static volatile uint16_t bootLoaderRxQueueRead;
volatile uint32_t bootLoaderUartOverrunCount;
volatile uint8_t bootLoaderRxQueueOverflow;
volatile uint8_t bootLoaderRxActive;
volatile uint8_t bootLoaderFlashBusy;
volatile uint8_t bootLoaderReportPending;
static volatile uint32_t bootLoaderLastRxTick;
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
static uint16_t BootLoader_RxQueueCount(void)
{
    uint16_t write = bootLoaderRxQueueWrite;
    uint16_t read = bootLoaderRxQueueRead;

    return (write >= read) ? (uint16_t)(write - read)
                           : (uint16_t)(BOOTLOADER_RX_QUEUE_SIZE - read + write);
}

static uint16_t BootLoader_RxQueueFree(void)
{
    return (uint16_t)((BOOTLOADER_RX_QUEUE_SIZE - 1U) - BootLoader_RxQueueCount());
}

HAL_StatusTypeDef BootLoader_WriteChunk(uint32_t offset, uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return HAL_ERROR;
    }

    uint32_t addr     = APP_START_ADDR + offset;
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
                return HAL_ERROR;
            }
            last_erased_sector = s;
            // printf("last_erased_sector:%d",last_erased_sector);
        }
    }

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
            return HAL_ERROR;
        }

        uint16_t halfword = (uint16_t)pending_flash_byte |
                            ((uint16_t)buf[0] << 8U);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              pending_flash_addr,
                              halfword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        pending_flash_byte_valid = 0U;
        i = 1U;
    }
    else if ((addr & 0x01U) != 0U)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
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
            return HAL_ERROR;
        }
    }

    if (i < len)
    {
        pending_flash_byte = buf[i];
        pending_flash_addr = addr + i;
        pending_flash_byte_valid = 1U;
    }
    HAL_FLASH_Lock();
    return HAL_OK;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        uint16_t free_count = BootLoader_RxQueueFree();

        if (Size > free_count)
        {
            bootLoaderRxQueueOverflow = 1U;
            Size = free_count;
        }

        for (uint16_t i = 0U; i < Size; i++)
        {
            bootLoaderRxQueue[bootLoaderRxQueueWrite] = bootLoaderReceiverBuff[i];
            bootLoaderRxQueueWrite++;
            if (bootLoaderRxQueueWrite >= BOOTLOADER_RX_QUEUE_SIZE)
            {
                bootLoaderRxQueueWrite = 0U;
            }
        }

        if (Size > 0U)
        {
            bootLoaderLastRxTick = HAL_GetTick();
            bootLoaderRxActive = 1U;
        }

        HAL_UARTEx_ReceiveToIdle_IT(&huart1, bootLoaderReceiverBuff, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((huart != NULL) && (huart->Instance == USART1))
    {
        if ((huart->ErrorCode & HAL_UART_ERROR_ORE) != 0U)
        {
            bootLoaderUartOverrunCount++;
        }

        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, bootLoaderReceiverBuff, BOOTLOADER_UART_RECEIVE_BUFF_SIZE);
    }
}

void BootLoader_Process(void)
{
    static uint8_t flash_chunk[256];
    uint16_t bytes_to_process;
    uint16_t bytes_processed = 0U;

    if (BootLoader_RxQueueCount() == 0U)
    {
        return;
    }

    if ((bootLoaderRxActive != 0U) &&
        ((HAL_GetTick() - bootLoaderLastRxTick) < BOOTLOADER_RX_IDLE_TIMEOUT_MS))
    {
        return;
    }

    bootLoaderRxActive = 0U;
    bytes_to_process = BootLoader_RxQueueCount();
    bootLoaderFlashBusy = 1U;

    while (bytes_processed < bytes_to_process)
    {
        uint16_t read = bootLoaderRxQueueRead;
        uint16_t write = bootLoaderRxQueueWrite;
        uint16_t available = BootLoader_RxQueueCount();
        uint16_t contiguous = (write > read) ? (uint16_t)(write - read)
                                             : (uint16_t)(BOOTLOADER_RX_QUEUE_SIZE - read);
        uint16_t chunk_len = contiguous;

        if (available > (uint16_t)(bytes_to_process - bytes_processed))
        {
            available = (uint16_t)(bytes_to_process - bytes_processed);
        }

        if (chunk_len > available)
        {
            chunk_len = available;
        }
        if (chunk_len > sizeof(flash_chunk))
        {
            chunk_len = sizeof(flash_chunk);
        }

        memcpy(flash_chunk, &bootLoaderRxQueue[read], chunk_len);

        if (BootLoader_WriteChunk(flash_WriteOffset, flash_chunk, chunk_len) != HAL_OK)
        {
            bootLoaderFlashBusy = 0U;
            return;
        }

        read = (uint16_t)(read + chunk_len);
        if (read >= BOOTLOADER_RX_QUEUE_SIZE)
        {
            read = 0U;
        }
        bootLoaderRxQueueRead = read;
        flash_WriteOffset += chunk_len;
        bootLoaderReceiverBuffFullLength += chunk_len;
        bytes_processed = (uint16_t)(bytes_processed + chunk_len);
    }

    bootLoaderFlashBusy = 0U;
    if ((BootLoader_RxQueueCount() == 0U) && (bootLoaderRxActive == 0U))
    {
        bootLoaderReportPending = 1U;
    }
}

void BootLoader_Inin(void){

    bootLoaderRxQueueWrite = 0U;
    bootLoaderRxQueueRead = 0U;
    bootLoaderUartOverrunCount = 0U;
    bootLoaderRxQueueOverflow = 0U;
    bootLoaderRxActive = 0U;
    bootLoaderFlashBusy = 0U;
    bootLoaderReportPending = 0U;
    bootLoaderLastRxTick = HAL_GetTick();
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    HAL_UARTEx_ReceiveToIdle_IT(&huart1,bootLoaderReceiverBuff,BOOTLOADER_UART_RECEIVE_BUFF_SIZE);

}

int fputc(int ch,FILE * f){

    HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,50);
    return ch;

}

void Int_BootLoader_JumpTo_Appliaction(void)
{
        __disable_irq();
        __DSB(); 
        __ISB();
        SysTick->CTRL = 0;
        SysTick->VAL  = 0;
        SysTick->LOAD = 0;
        __DSB();
        __ISB();

        for(uint32_t i=0; i<8; i++)
        {
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }
        uint32_t appStack = *(__IO uint32_t*)APP_START_ADDR;
        __set_MSP(appStack);
        SCB->VTOR = APP_START_ADDR;
        typedef void (*AppResetHandler)(void);
        uint32_t appResetAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);
        AppResetHandler JumpToApp = (AppResetHandler)appResetAddr;
        
        JumpToApp();
}

