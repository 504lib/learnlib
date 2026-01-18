/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#define PE3_Pin GPIO_PIN_2
#define PE3_GPIO_Port GPIOE
#define PE4_Pin GPIO_PIN_3
#define PE4_GPIO_Port GPIOE
#define PE5_Pin GPIO_PIN_4
#define PE5_GPIO_Port GPIOE
#define PE6_Pin GPIO_PIN_5
#define PE6_GPIO_Port GPIOE
#define PE7_Pin GPIO_PIN_6
#define PE7_GPIO_Port GPIOE
#define LED3_Pin GPIO_PIN_4
#define LED3_GPIO_Port GPIOC
#define LED4_Pin GPIO_PIN_5
#define LED4_GPIO_Port GPIOC
#define PE8_Pin GPIO_PIN_7
#define PE8_GPIO_Port GPIOE
#define AIN_Pin GPIO_PIN_13
#define AIN_GPIO_Port GPIOE
#define BIN_Pin GPIO_PIN_10
#define BIN_GPIO_Port GPIOB
#define key2_Pin GPIO_PIN_14
#define key2_GPIO_Port GPIOB
#define key1_Pin GPIO_PIN_15
#define key1_GPIO_Port GPIOB
#define RES_OLED_Pin GPIO_PIN_0
#define RES_OLED_GPIO_Port GPIOD
#define DC_OLED_Pin GPIO_PIN_1
#define DC_OLED_GPIO_Port GPIOD
#define CS_OLED_Pin GPIO_PIN_2
#define CS_OLED_GPIO_Port GPIOD
#define PE1_Pin GPIO_PIN_0
#define PE1_GPIO_Port GPIOE
#define PE2_Pin GPIO_PIN_1
#define PE2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
