#ifndef __BSP_SPIFLASH_H__
#define __BSP_SPIFLASH_H__

#include "stm32f4xx_hal.h"
#include "main.h"

//#define  SPI_FLASH_ID                       0x3015     //W25X16   2MB
//#define  SPI_FLASH_ID                       0x4015	   //W25Q16   4MB
//#define  SPI_FLASH_ID                       0X4017     //W25Q64   8MB
#define  SPI_FLASH_ID                       0X4018     //W25Q128  16MB

#define FLASH_SPIx                                 SPI1
#define FLASH_SPIx_RCC_CLK_ENABLE()                __HAL_RCC_SPI1_CLK_ENABLE()
#define FLASH_SPIx_RCC_CLK_DISABLE()               __HAL_RCC_SPI1_CLK_DISABLE()

#define FLASH_SPI_GPIO_ClK_ENABLE()                __HAL_RCC_GPIOA_CLK_ENABLE() 
#define FLASH_SPI_GPIO_PORT                        GPIOA
#define FLASH_SPI_SCK_PIN                          GPIO_PIN_5

#define FLASH_SPI_MOSI_PIN                         GPIO_PIN_7
    
#define FLASH_SPI_CS_PIN                           GPIO_PIN_4

#define FLASH_SPI_CS_ENABLE()                      HAL_GPIO_WritePin(FLASH_SPI_GPIO_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_RESET)
#define FLASH_SPI_CS_DISABLE()                     HAL_GPIO_WritePin(FLASH_SPI_GPIO_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_SET)

#define FLASH_SPI_MISO_PORT                        GPIOA
#define FLASH_SPI_MISO_PIN                         GPIO_PIN_6


#define FS_SHOOTCNT_ADD    0x00000
#define FS_ID_ADD            0x01000
#define FS_SET_ADD           0x02000
#define FS_CHK_ADD           0x03000     //检测已录入的检测值为 0xCC
#define FS_CHK_VAL           0xCC


extern device spiflash;
extern SPI_HandleTypeDef hspiflash;


void MX_SPIFlash_Init(void);
void SPI_FLASH_SectorErase(uint32_t SectorAddr);
void SPI_FLASH_BulkErase(void);
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t SPI_FLASH_ReadID(void);
uint32_t SPI_FLASH_ReadDeviceID(void);
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);

uint8_t SPI_FLASH_ReadByte(void);
uint8_t SPI_FLASH_SendByte(uint8_t byte);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);

uint8_t spiflash_open(const char * pathname , int flags);
uint8_t spiflash_read(int fd,void*buf,int count);
uint8_t spiflash_write(int fd,void*buf,int count);
uint8_t spiflash_erase(int fd,void*pos,void *buf);

#endif  /* __BSP_SPIFLASH_H__ */


