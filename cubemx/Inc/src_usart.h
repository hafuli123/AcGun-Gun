/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
extern device mp3;
extern device gc24;

extern UART_HandleTypeDef husartx;
/* USER CODE BEGIN Private defines */
/* MP3模块 */
#define usart_tx_GPIO_Port  GPIOA
#define usart_tx_Pin        GPIO_PIN_9

/* GC2400模块 */
#define USARTx                                 USART2
#define USARTx_BAUDRATE                        115200
#define USART_RCC_CLK_ENABLE()                 __HAL_RCC_USART2_CLK_ENABLE()
#define USART_RCC_CLK_DISABLE()                __HAL_RCC_USART2_CLK_DISABLE()

#define USARTx_GPIO_ClK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_Tx_GPIO_PIN                     GPIO_PIN_2
#define USARTx_Tx_GPIO                         GPIOA
#define USARTx_Rx_GPIO_PIN                     GPIO_PIN_3
#define USARTx_Rx_GPIO                         GPIOA

#define USARTx_AFx                             GPIO_AF7_USART2

#define USARTx_IRQHANDLER                      USART2_IRQHandler
#define USARTx_IRQn                            USART2_IRQn
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void SoftUART_GPIO_Init(void);
void MX_USARTx_Init(void);
void USART_Send(uint8_t *buf,uint8_t len);

uint8_t mp3_open(const char * pathname , int flags);
uint8_t mp3_write(int fd,void*buf,int count);
uint8_t gc24_open(const char * pathname , int flags);
uint8_t gc24_write(int fd,void*buf,int count);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
