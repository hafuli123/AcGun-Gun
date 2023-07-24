/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <rtthread.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dev.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void shw_shootcnt(uint8_t x,uint8_t y,uint8_t itl,uint32_t msg);
void shw_bmpNum(uint8_t x,uint8_t y,uint8_t val);
uint32_t get_bat_perc(uint32_t qty);
void bat_disp(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2 ,uint32_t per);
void play_snd(uint8_t type);
void shw_menutit(uint8_t x,uint8_t y,uint8_t type,uint8_t lang);
void set_opt(uint8_t x,uint8_t y,uint8_t type,uint8_t set,uint8_t lang);
void shw_volnum(uint8_t x,uint8_t y,uint8_t itl,uint32_t vol);
void shw_msg(uint8_t x,uint8_t y,uint8_t itl,uint32_t msg ,uint8_t x2,uint8_t y2,uint8_t itl2, uint32_t msg2);
void clr_bmp(void);
float opt_p_val(int raw_val);
void set_vol(uint32_t vol);
void rt_sec_delay(int sec);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define GUNTYPE_RIFLE   1
#define GUNTYPE_HANDGUN   2
#define GUN_TYPE        GUNTYPE_HANDGUN

#define P_SEND_LIMVAL 50   //压力大于多少才算有按下 才发送压力值

#define TIMER_MOT_OFF_INTERVAL    300 //300ms停止电机振动
#define TIMER_ADC_P_INTERVAL       5   //5ms采集发送一次压力

#define RLS_CHK_PT    3
#define SHOOT_RLS_CNT     100

#define P_LIMVAL       800 //极限压力高度

#define N   0
#define Y   1

enum{
    LASER_ALWAYSOFF=0,
    LASER_ALWAYSON,
    LASER_TRIGGER,
    LASER_SHOOTON,

    MOT_DIS=0,
    MOT_E,

    SND_DIS=0,
    SND_E,

    SNDTYP_A=0,
    SNDTYP_B,
    SNDTYP_C,
    SNDTYP_D,

    LANG_ENG=0,
    LANG_CHS,
    LANG_TC
};

enum{
    SCRN_START = 0,
    SCRN_MSG,
    SCRN_SHOOTCNT,
    SCRN_LASER,
    SCRN_SHOCK,
    SCRN_SND,
    SCRN_SNDTYP,
    SCRN_VOL,
    SCRN_LANG,
    SCRN_TURNOFF,
};

enum{
    MRS_NRM = 0,
    MRS_LOADED,     //上膛
    MRS_SHOOT,      //开枪完成
    MRS_END,        //开枪后采集结束
    MRS_LGTOFF,   //关灯
};

enum{
    KEY_ON_DIS = 0,
    KEY_ON_NRM,
    KEY_ON_E,
    KEY_ON_END

};



typedef struct __Thread{
  rt_thread_t th;
  struct rt_semaphore sem;
  rt_mailbox_t mb;
  rt_timer_t tm;
}Thread;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
