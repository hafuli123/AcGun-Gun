#include "savflash.h"

/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/*准备写入的测试数据*/
#define DATA_32                 ((uint32_t)0x87654321)
#define DATA_8                 ((uint32_t)0x67)

/* 要擦除内部FLASH的起始地址 */
#define FLASH_USER_START_ADDR   ADDR_FLASH_SECTOR_5
/* 要擦除内部FLASH的结束地址 */
#define FLASH_USER_END_ADDR     ADDR_FLASH_SECTOR_5  +  GetSectorSize(ADDR_FLASH_SECTOR_5) -1

/* 设置存储地址  */
#define FLASH_SHOOTCOUNT_ADDR   FLASH_USER_START_ADDR
#define FLASH_SNUM_ADDR         ((uint32_t)0x08020010)
#define FLASH_TXADD_ADDR        ((uint32_t)0x08020020)

/* 私有变量 ------------------------------------------------------------------*/
uint32_t FirstSector = 0, NbOfSectors = 0, Address = 0;
uint32_t SectorError = 0;
uint32_t data32 = 0 , MemoryProgramStatus = 0;
uint8_t data8;

static FLASH_EraseInitTypeDef EraseInitStruct;

/* 扩展变量 ------------------------------------------------------------------*/
/* 私有函数原形 --------------------------------------------------------------*/
static uint32_t GetSector(uint32_t Address);
static uint32_t GetSectorSize(uint32_t Sector);

/* 函数体 --------------------------------------------------------------------*/
void InternalFlash_Write(uint32_t content)
{
  /* 解锁,删除和写入必须先解锁 */
  HAL_FLASH_Unlock();

  /* 获取要擦除的首个扇区 */
  FirstSector = GetSector(FLASH_USER_START_ADDR);

  /* 获取要擦除的扇区数目 */
  NbOfSectors = GetSector(FLASH_USER_END_ADDR) - FirstSector + 1;

  /* 初始化擦除结构体 */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;//扇区擦除
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = FirstSector;
  EraseInitStruct.NbSectors = NbOfSectors;
  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
  {
    return;
  }
  Address = FLASH_USER_START_ADDR;
  //写
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, content);

  /* 锁定，如果是读取的话，无需解锁 */
  HAL_FLASH_Lock();
  return;
}
/*
 * 写入总射击次数、编号、地址信息
 */
void InternalFlash_WriteContent(uint32_t shootcount,uint32_t serialnum, uint32_t txadd)
{
  /* 解锁,删除和写入必须先解锁 */
  HAL_FLASH_Unlock();

  /* 获取要擦除的首个扇区 */
  FirstSector = GetSector(FLASH_USER_START_ADDR);

  /* 获取要擦除的扇区数目 */
  NbOfSectors = GetSector(FLASH_USER_END_ADDR) - FirstSector + 1;

  /* 初始化擦除结构体 */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;//扇区擦除
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = FirstSector;
  EraseInitStruct.NbSectors = NbOfSectors;
  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
  {
    return;
  }

  //写
  Address = FLASH_SHOOTCOUNT_ADDR;
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, shootcount);

  Address = FLASH_SNUM_ADDR;
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, serialnum);

  Address = FLASH_TXADD_ADDR;
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, txadd);

  /* 锁定，如果是读取的话，无需解锁 */
  HAL_FLASH_Lock();
  return;
}

uint32_t InternalFlash_Read(void)
{
    Address = FLASH_USER_START_ADDR;
    data32=*(uint32_t*)Address;
    return data32;
}
/*
 * 读取设置
 */
uint32_t InternalFlash_ReadSC(void)
{
    Address = FLASH_SHOOTCOUNT_ADDR;
    data32=*(uint32_t*)Address;
    return data32;
}
uint32_t InternalFlash_ReadSnum(void)
{
    Address = FLASH_SNUM_ADDR;
    data32=*(uint32_t*)Address;
    return data32;
}
uint32_t InternalFlash_ReadTxadd(void)
{
    Address = FLASH_TXADD_ADDR;
    data32=*(uint32_t*)Address;
    return data32;
}

/**
  * 函数功能: 根据输入的地址给出它所在的sector
  * 输入参数: Address flash地址
  * 返 回 值: 无
  * 说    明: 无
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }

  return sector;
}

/**
  * 函数功能: 根据扇区编号获取扇区大小
  * 输入参数: Sector
  * 返 回 值: 无
  * 说    明: 无
  */
static uint32_t GetSectorSize(uint32_t Sector)
{
  uint32_t sectorsize = 0x00;

  if((Sector == FLASH_SECTOR_0) || (Sector == FLASH_SECTOR_1) || (Sector == FLASH_SECTOR_2) || (Sector == FLASH_SECTOR_3))
  {
    sectorsize = 16 * 1024;
  }
  else if(Sector == FLASH_SECTOR_4)
  {
    sectorsize = 64 * 1024;
  }
  else
  {
    sectorsize = 128 * 1024;
  }
  return sectorsize;
}
