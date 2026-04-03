/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "PID_Node.h"
#include "oled.h"
#include "mpu6050.h"
#include "MadgwickAHRS.h"
#include "multikey.h"
#include <stdbool.h>
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "Log.h"
#include "mpu6050_user.h"
#include "key_control.h"
#include "tasks.h"
//#include "static_queue.h"
#include <stdbool.h>
#include "control.h"
#include "Protothreads.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
MulitKey_t key1;
MulitKey_t key2;
MotorAT4950 motor1;
MotorAT4950 motor2;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
MPU6050_Data_t* mpu = NULL;   // 全局指针，用于整个文件访问
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 电机驱动底层函数实现（放在 main 函数外）
void SetMotor1PWM(uint16_t ccr) {
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, ccr);
}
void SetMotor2PWM(uint16_t ccr) {
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, ccr);
}
void SetMotor1IN1(uint8_t level) {
    HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, (GPIO_PinState)level);
}
void SetMotor2IN1(uint8_t level) {
    HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, (GPIO_PinState)level);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  OLED_ShowString(0, 0, "Calibrating...", 16, 1);
  OLED_ShowString(0, 16, "Keep Still!", 16, 1);
  OLED_Refresh();   // 立即刷新，否则可能不显示
	// 初始化电机
  MotorInit_AT46950(&motor1, SetMotor1PWM, SetMotor1IN1, 1000-1); // ARR = 999
  MotorInit_AT46950(&motor2, SetMotor2PWM, SetMotor2IN1, 1000-1);
  SetDefaultDirection(&motor1, High_Level);
  SetDefaultDirection(&motor2, High_Level);

	// 启动 PWM 输出
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

	// 启动编码器模式
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

	// 启动 TIM4 中断（周期已在 MX_TIM4_Init 中配置为 10ms）
  Control_Init();
  MPU6050_Calibrate(200);
  KeyControl_Init();
  Tasks_Init();
  mpu = MPU6050_GetHandle();   // 获取句柄
  HAL_TIM_Base_Start_IT(&htim4);    
	OLED_ShowString(0, 0, "test", 16, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  KeyControl_Scan();   // 扫描按键
	  // 获取显示模式
    // uint8_t mode = KeyControl_GetDisplayMode();
	  task1(&task1_pt);
	  task2(&task2_pt);
	  SerialTask(&SerialTask_pt);
	  OLED_Task(&oled_pt); 
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
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
    if (htim->Instance == TIM4)
    {
        static uint32_t last_tick = 0;
        uint32_t now = HAL_GetTick();
        if (now - last_tick > 1000)
        {
          printf("1000 ms elapsed\n");
        }
        float dt = (now - last_tick) / 1000.0f;
        last_tick = now;

        // 读取 MPU6050 原始数据（已封装在 MPU6050_Update 中）
        MPU6050_Update();   // 该函数会读原始值、校准、转换并更新欧拉角

        // 读取灰度传感器并计算误差
        gray_byte = GPIOE->IDR & 0xFF;
        gray_error = CalculateGrayError_Advanced(gray_byte);

        // 读取编码器差值（注意：编码器计数器可能为负数，需根据实际接线调整符号）
        static int32_t last_A = 0, last_B = 0;
        int32_t current_A = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
        int32_t current_B = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
        int32_t diff_A = current_A - last_A;
        int32_t diff_B = current_B - last_B;
        last_A = current_A;
        last_B = current_B;
        Control_UpdateSpeedFeedback(diff_A, diff_B, dt);

        // 执行控制
        Control_Update(dt);

        // 定期清零编码器（防止溢出）
        static uint8_t cnt = 0;
        if (++cnt >= 50) {
            __HAL_TIM_SetCounter(&htim2, 0);
            __HAL_TIM_SetCounter(&htim3, 0);
            cnt = 0;
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
