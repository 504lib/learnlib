/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "pid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern tPid pidMotor1Speed;
extern tPid pidMotor2Speed;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 编码器和机械参数
#define ENCODER_PPR 13                  // 编码器线数（电机轴）
#define GEAR_RATIO 20                   // 减速比 1:20
#define OUTPUT_SHAFT_PPR 260            // 输出轴等效脉冲数 = 13 × 20
#define EFFECTIVE_PPR 1040              // 四倍频后脉冲数 = 260 × 4
#define WHEEL_DIAMETER 0.048            // 轮子直径48mm = 0.048m
#define WHEEL_CIRCUMFERENCE 0.1508      // 轮子周长 = π × 0.048 ≈ 0.1508m
#define SAMPLING_TIME 0.1              // 采样时间20ms = 0.02s

// 速度计算系数
#define SPEED_CALC_FACTOR (WHEEL_CIRCUMFERENCE / (EFFECTIVE_PPR * SAMPLING_TIME))
// 简化计算: 速度 = 脉冲数 × 0.000145

// 速度环全局变量
float Target_Speed_A = 0.0;      // A轮目标速度 (m/s)
float Target_Speed_B = 0.0;      // B轮目标速度 (m/s)
float Actual_Speed_A = 0.0;      // A轮实际速度 (m/s)  
float Actual_Speed_B = 0.0;      // B轮实际速度 (m/s)
int32_t A_Count = 0;             // A编码器计数
int32_t B_Count = 0;             // B编码器计数

// PID参数（需要根据实际电机调整）
#define KP_SPEED 3.0f            // 速度环比例系数
#define KI_SPEED 0.8f            // 速度环积分系数
#define KD_SPEED 0.2f            // 速度环微分系数

// PID计算变量
float A_Error_Integral = 0.0;    // A轮误差积分
float A_Last_Error = 0.0;        // A轮上次误差
float B_Error_Integral = 0.0;    // B轮误差积分  
float B_Last_Error = 0.0;        // B轮上次误差

// PWM限制
#define MAX_PWM 100              // 最大PWM值
#define MIN_PWM 0                // 最小PWM值
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
//	HAL_Delay(100);
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  HAL_Delay(100);
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
  HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin,GPIO_PIN_SET);  
  HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin,GPIO_PIN_SET);  
  HAL_TIM_Encoder_Start(&htim2,TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);
  __HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,0);
  __HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,0);
  HAL_TIM_Base_Start_IT(&htim4);
  PID_init();
  
  // HAL_TIM_Base_Start_IT(&htim4);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)  
  {
	  HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
	  HAL_Delay(200);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint32_t count = 0;
	if (htim->Instance == TIM4)
  {
    count++;
	if(count % 2 == 0)
	{

	  __HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,PID_realize(&pidMotor1Speed,Actual_Speed_A));
	  __HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,PID_realize(&pidMotor2Speed,Actual_Speed_B));
			      printf("%.2f,%.2f,%.2f,%.2f,%lu,%lu\r\n", 
           Target_Speed_A, Target_Speed_B, 
           Actual_Speed_A, Actual_Speed_B,
           __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1),
           __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2));

	}
    if (count >= 50)
    {
      A_Count =  (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
      B_Count =  (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
      __HAL_TIM_SetCounter(&htim2, 0);
      __HAL_TIM_SetCounter(&htim3, 0);
      Actual_Speed_A = A_Count * SPEED_CALC_FACTOR;
      Actual_Speed_B = B_Count * SPEED_CALC_FACTOR;
      
      // 速度滤波（减少噪声）
      static float filtered_A = 0.0, filtered_B = 0.0;
      filtered_A = 0.6f * filtered_A + 0.4f * Actual_Speed_A;
      filtered_B = 0.6f * filtered_B + 0.4f * Actual_Speed_B;
      Actual_Speed_A = filtered_A;
      Actual_Speed_B = filtered_B;
	 
		
    }
    if (count >= 50)
    {
      count = 0;
      // __HAL_TIM_SET_COUNTER(&htim2, 0);
      // __HAL_TIM_SET_COUNTER(&htim3, 0);

    }
    
    
  }
  
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
