/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "src_usart.h"
#include "board.h"
#include "gun_info.h"
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
UART_HandleTypeDef husartx;

device mp3={
        "mp3",
        mp3_open,
        NULL,
        mp3_write,
        NULL
};
uint8_t mp3_open(const char * pathname , int flags)
{
    SoftUART_GPIO_Init();
    return 0;
}
uint8_t mp3_write(int fd,void*buf,int count)
{
    USART_Send((uint8_t*)buf,count);
    return 0;
}

device gc24={
        "gc24",
        gc24_open,
        NULL,
        gc24_write,
        NULL
};
uint8_t gc24_open(const char * pathname , int flags)
{
    MX_USARTx_Init();
    return 0;
}
uint8_t gc24_write(int fd,void*buf,int count)
{
    GunInfo_Struct *info;
    info=(GunInfo_Struct *)buf;
    info->shootflg = count;
    guninfo_send(info);
    return 0;
}

/* USART1 init function */
void MX_USARTx_Init(void)
{
  /* 串口外设时钟使能 */
  USART_RCC_CLK_ENABLE();

  husartx.Instance = USARTx;
  husartx.Init.BaudRate = USARTx_BAUDRATE;
  husartx.Init.WordLength = UART_WORDLENGTH_8B;
  husartx.Init.StopBits = UART_STOPBITS_1;
  husartx.Init.Parity = UART_PARITY_NONE;
  husartx.Init.Mode = UART_MODE_TX_RX;
  husartx.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  husartx.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&husartx);
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USARTx)
  {
      /* 使能串口功能引脚GPIO时钟 */
      USARTx_GPIO_ClK_ENABLE();

      /* 串口外设功能GPIO配置 */
      GPIO_InitStruct.Pin = USARTx_Tx_GPIO_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = USARTx_AFx;
      HAL_GPIO_Init(USARTx_Tx_GPIO, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = USARTx_Rx_GPIO_PIN;
      HAL_GPIO_Init(USARTx_Rx_GPIO, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if(uartHandle->Instance==USARTx)
    {
      USART_RCC_CLK_DISABLE();

      /* 串口外设功能GPIO配置 */
      HAL_GPIO_DeInit(USARTx_Tx_GPIO, USARTx_Tx_GPIO_PIN);
      HAL_GPIO_DeInit(USARTx_Rx_GPIO, USARTx_Rx_GPIO_PIN);
  }
}

/* USER CODE BEGIN 1 */
void SoftUART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin(usart_tx_GPIO_Port, usart_tx_Pin, GPIO_PIN_SET);
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = usart_tx_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(usart_tx_GPIO_Port, &GPIO_InitStruct);

}

void IO_TXD(uint8_t Data)
{
    uint8_t i = 0;
     HAL_GPIO_WritePin(usart_tx_GPIO_Port, usart_tx_Pin, GPIO_PIN_RESET);
     rt_hw_us_delay(105);
     for(i = 0; i < 8; i++)
     {
         if(Data&0x01)
         HAL_GPIO_WritePin(usart_tx_GPIO_Port, usart_tx_Pin, GPIO_PIN_SET);
         else
         HAL_GPIO_WritePin(usart_tx_GPIO_Port, usart_tx_Pin, GPIO_PIN_RESET);
         rt_hw_us_delay(105);
         Data = Data>>1;
     }
     HAL_GPIO_WritePin(usart_tx_GPIO_Port, usart_tx_Pin, GPIO_PIN_SET);
     rt_hw_us_delay(105);

}

void USART_Send(uint8_t *buf,uint8_t len)
{
    uint8_t t;
    for(t = 0; t < len; t++)
    {
        IO_TXD(buf[t]);
    }
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
