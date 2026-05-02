/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
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
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PCB_LED0_Pin GPIO_PIN_4
#define PCB_LED0_GPIO_Port GPIOC
#define PCB_LED1_Pin GPIO_PIN_5
#define PCB_LED1_GPIO_Port GPIOC
#define AIN_Pin GPIO_PIN_13
#define AIN_GPIO_Port GPIOE
#define BIN_Pin GPIO_PIN_10
#define BIN_GPIO_Port GPIOB
#define PCB_KEY1_Pin GPIO_PIN_14
#define PCB_KEY1_GPIO_Port GPIOB
#define PCB_KEY0_Pin GPIO_PIN_15
#define PCB_KEY0_GPIO_Port GPIOB
#define RES_OLED_Pin GPIO_PIN_0
#define RES_OLED_GPIO_Port GPIOD
#define DC_OLED_Pin GPIO_PIN_1
#define DC_OLED_GPIO_Port GPIOD
#define CS_OLED_Pin GPIO_PIN_2
#define CS_OLED_GPIO_Port GPIOD
#define TestPin_Pin GPIO_PIN_0
#define TestPin_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
