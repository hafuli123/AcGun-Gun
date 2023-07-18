/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define KEY_ON_GPIO GPIOB
#define KEY_ON_PIN GPIO_PIN_8
#define KEY_ON_DOWN_LEVEL GPIO_PIN_SET

#define KEY2_GPIO GPIOB
#define KEY2_PIN GPIO_PIN_9
#define KEY2_DOWN_LEVEL GPIO_PIN_RESET
#define KEY_READ(pin)   HAL_GPIO_ReadPin(GPIOB,pin)
#define KEY_ON_RLS      (HAL_GPIO_ReadPin(KEY_ON_GPIO,KEY_ON_PIN)==GPIO_PIN_RESET)
#define KEY_ON_PRESS    (HAL_GPIO_ReadPin(KEY_ON_GPIO,KEY_ON_PIN)==GPIO_PIN_SET)
#define KEY2_RLS        (HAL_GPIO_ReadPin(KEY2_GPIO,KEY2_PIN)==GPIO_PIN_SET)
#define KEY2_PRESS      (HAL_GPIO_ReadPin(KEY2_GPIO,KEY2_PIN)==GPIO_PIN_RESET)

#define PWR_EN_GPIO GPIOB
#define PWR_EN_PIN GPIO_PIN_3
#define SYS_ON()   HAL_GPIO_WritePin(PWR_EN_GPIO, PWR_EN_PIN, GPIO_PIN_SET);
#define SYS_OFF()   HAL_GPIO_WritePin(PWR_EN_GPIO, PWR_EN_PIN, GPIO_PIN_RESET);

#define MOTOR_GPIO GPIOB
#define MOTOR_PIN GPIO_PIN_13
#define MOT_ON()  HAL_GPIO_WritePin(MOTOR_GPIO, MOTOR_PIN, GPIO_PIN_SET);
#define MOT_OFF()  HAL_GPIO_WritePin(MOTOR_GPIO, MOTOR_PIN, GPIO_PIN_RESET);

#define REDL_GPIO GPIOB
#define REDL_PIN GPIO_PIN_14
#define REDL_ON() HAL_GPIO_WritePin(REDL_GPIO, REDL_PIN, GPIO_PIN_SET);
#define REDL_OFF() HAL_GPIO_WritePin(REDL_GPIO, REDL_PIN, GPIO_PIN_RESET);

#define UVL_GPIO GPIOB
#define UVL_PIN GPIO_PIN_15
#define UVL_ON() HAL_GPIO_WritePin(UVL_GPIO, UVL_PIN, GPIO_PIN_SET);
#define UVL_OFF() HAL_GPIO_WritePin(UVL_GPIO, UVL_PIN, GPIO_PIN_RESET);

#define MRS_GPIO GPIOB
#define MRS_PIN GPIO_PIN_4
#define MRS_RELEASE (HAL_GPIO_ReadPin(MRS_GPIO,MRS_PIN)==GPIO_PIN_SET)
#define MRS_LOAD    (HAL_GPIO_ReadPin(MRS_GPIO,MRS_PIN)==GPIO_PIN_RESET)

#define SOUNDCARD_GPIO  GPIOB
#define SOUNDCARD_PIN   GPIO_PIN_10
#define SND_ON()  HAL_GPIO_WritePin(SOUNDCARD_GPIO, SOUNDCARD_PIN, GPIO_PIN_SET)
#define SND_OFF()  HAL_GPIO_WritePin(SOUNDCARD_GPIO, SOUNDCARD_PIN, GPIO_PIN_RESET)

#define MY_IO1_GPIO GPIOC
#define MY_IO1_PIN GPIO_PIN_2
#define MY_IO1_HIGH()   HAL_GPIO_WritePin(MY_IO1_GPIO, MY_IO1_PIN, GPIO_PIN_SET)
#define MY_IO1_LOW()   HAL_GPIO_WritePin(MY_IO1_GPIO, MY_IO1_PIN, GPIO_PIN_RESET)

#define MY_IO2_GPIO GPIOC
#define MY_IO2_PIN GPIO_PIN_3
#define MY_IO2_HIGH()   HAL_GPIO_WritePin(MY_IO2_GPIO, MY_IO2_PIN, GPIO_PIN_SET)
#define MY_IO2_LOW()   HAL_GPIO_WritePin(MY_IO2_GPIO, MY_IO2_PIN, GPIO_PIN_RESET)

#define MY_IO3_GPIO GPIOC
#define MY_IO3_PIN GPIO_PIN_5
#define MY_IO3_HIGH()   HAL_GPIO_WritePin(MY_IO3_GPIO, MY_IO3_PIN, GPIO_PIN_SET)
#define MY_IO3_LOW()   HAL_GPIO_WritePin(MY_IO3_GPIO, MY_IO3_PIN, GPIO_PIN_RESET)

#define MY_IO4_GPIO GPIOC
#define MY_IO4_PIN GPIO_PIN_4
#define MY_IO4_HIGH()   HAL_GPIO_WritePin(MY_IO4_GPIO, MY_IO4_PIN, GPIO_PIN_SET)
#define MY_IO4_LOW()   HAL_GPIO_WritePin(MY_IO4_GPIO, MY_IO4_PIN, GPIO_PIN_RESET)

#define MY_IO5_GPIO GPIOA
#define MY_IO5_PIN GPIO_PIN_1
#define MY_IO5_HIGH()   HAL_GPIO_WritePin(MY_IO5_GPIO, MY_IO5_PIN, GPIO_PIN_SET)
#define MY_IO5_LOW()   HAL_GPIO_WritePin(MY_IO5_GPIO, MY_IO5_PIN, GPIO_PIN_RESET)

#define BAT_DONE_GPIO   GPIOA
#define BAT_DONE_PIN   GPIO_PIN_15
#define BAT_CHRG_GPIO   GPIOB
#define BAT_CHRG_PIN   GPIO_PIN_5

#define BAT_DONE_Y        (HAL_GPIO_ReadPin(BAT_DONE_GPIO,BAT_DONE_PIN)==GPIO_PIN_RESET)
#define BAT_DONE_N        (HAL_GPIO_ReadPin(BAT_DONE_GPIO,BAT_DONE_PIN)==GPIO_PIN_SET)

#define BAT_CHRG_Y       (HAL_GPIO_ReadPin(BAT_CHRG_GPIO,BAT_CHRG_PIN)==GPIO_PIN_SET)
#define BAT_CHRG_N       (HAL_GPIO_ReadPin(BAT_CHRG_GPIO,BAT_CHRG_PIN)==GPIO_PIN_RESET)


/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
