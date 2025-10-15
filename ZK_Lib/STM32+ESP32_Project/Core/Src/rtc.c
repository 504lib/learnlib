/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

// �???�????RTC�????坦需覝初始化
uint8_t RTC_NeedInit(void)
{
  // �???查�?�份寄存器标�????
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == 0xA5A5) {
    return 0; // 已绝初�?�化�????
  }
  return 1; // �???覝初始化
}

// 标�?�RTC已初始化
void RTC_MarkInitialized(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xA5A5);
}
void RTC_SaveDate(RTC_DateTypeDef *date)
{
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BKP_CLK_ENABLE();
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, date->Year);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, date->Month);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, date->Date);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, date->WeekDay);
}

uint8_t RTC_RestoreDate(RTC_DateTypeDef *date)
{
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0xA5A5) {
    return 0; // ???????
  }
  
  date->Year = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
  date->Month = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
  date->Date = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);
  date->WeekDay = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);
  return 1;
}
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */
 
  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};
  RTC_DateTypeDef saved_date;

  /* USER CODE BEGIN RTC_Init 1 */
  if(RTC_NeedInit() == 0)
  {
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    if(RTC_RestoreDate(&saved_date))
    {
      if(HAL_RTC_SetDate(&hrtc, &saved_date, RTC_FORMAT_BIN) != HAL_OK)
      {
        Error_Handler();
      }
      LOG_INFO("Restored date: %04d-%02d-%02d", saved_date.Year + 2000, saved_date.Month, saved_date.Date);
    }
    return;
  } 
  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
   /* code */

  
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 18;
  sTime.Minutes = 45;
  sTime.Seconds = 12;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_TUESDAY;
  DateToUpdate.Month = RTC_MONTH_SEPTEMBER;
  DateToUpdate.Date = 30;
  DateToUpdate.Year = 25;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  RTC_SaveDate(&DateToUpdate);
  RTC_MarkInitialized();

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
