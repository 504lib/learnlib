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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "protocol.h"
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
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define CS_OLED_Pin GPIO_PIN_0
#define CS_OLED_GPIO_Port GPIOA
#define DC_OLED_Pin GPIO_PIN_1
#define DC_OLED_GPIO_Port GPIOA
#define RES_OLED_Pin GPIO_PIN_4
#define RES_OLED_GPIO_Port GPIOA
#define KEY_UP_Pin GPIO_PIN_0
#define KEY_UP_GPIO_Port GPIOB
#define KEY_DOWN_Pin GPIO_PIN_1
#define KEY_DOWN_GPIO_Port GPIOB
#define KEY_ENTER_Pin GPIO_PIN_10
#define KEY_ENTER_GPIO_Port GPIOB
#define KEY_CANCEL_Pin GPIO_PIN_11
#define KEY_CANCEL_GPIO_Port GPIOB
#define HX711_SCK_Pin GPIO_PIN_14
#define HX711_SCK_GPIO_Port GPIOB
#define HX711_DOUT_Pin GPIO_PIN_15
#define HX711_DOUT_GPIO_Port GPIOB
#define AIN1_Pin GPIO_PIN_15
#define AIN1_GPIO_Port GPIOA
#define BIN1_Pin GPIO_PIN_4
#define BIN1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
//#define KEY_UP_EVENT    (1U << 0)
//#define KEY_DOWN_EVENT  (1U << 1)
//#define KEY_ENTER_EVENT  (1U << 2)
//#define KEY_CANCEL_EVENT  (1U << 3)
//#define KEY_FUNCTION_EVENT  (1U << 4)

#define UART_RECEIVE_EVENT  (1U << 0)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
