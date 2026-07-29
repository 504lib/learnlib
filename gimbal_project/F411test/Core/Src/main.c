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
#include <stdbool.h>
#include "oled.h"
#include "Log.h"
#include "mpu6050.h"
#include "mpu6050_user.h"
#include "MadgwickAHRS.h"
#include "ZDT_Motor_Serial.h"
#include "multikey.h"
#include "PID_Node.h"
#include "app_protocol.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define IMU_UPDATE_PERIOD_MS 2u        // TIM2中断周期 2ms
#define PID_DT              0.002f     // 控制周期(秒)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t rx_byte = 0;
MPU6050_Data_t* mpu_data = NULL;
ZDT_Motor_Handle_t x_asix_motor;
MulitKey_t key2;
MulitKey_t key3;
PID_Node x_axis_pid;
volatile float x_axis_target_angle = 0.0f;  // 目标角度，单位：度
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void ZDT_Send_Tx_callback(uint8_t *pData, uint16_t Size)
{
    HAL_UART_Transmit(&huart2, pData, Size, HAL_MAX_DELAY);
}

uint8_t Key2_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET ? 1 : 0;
}

uint8_t Key3_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET ? 1 : 0;
}

void Key2_PressedCallback(MulitKey_t* key)
{
  x_axis_target_angle += 10.0f;  // 每次按下增加10度
  PID_Node_SetSetpoint(&x_axis_pid, x_axis_target_angle);
}

void Key3_PressedCallback(MulitKey_t* key)
{
  x_axis_target_angle -= 10.0f;  // 每次按下减少10度
  PID_Node_SetSetpoint(&x_axis_pid, x_axis_target_angle);
}

void Key2_LongPressedCallback(MulitKey_t* key)
{
  x_axis_target_angle += 10.0f;  // 每次按下增加10度
  PID_Node_SetSetpoint(&x_axis_pid, x_axis_target_angle);
}


void Key3_LongPressedCallback(MulitKey_t* key)
{
  x_axis_target_angle -= 10.0f;  // 每次按下减少10度
  PID_Node_SetSetpoint(&x_axis_pid, x_axis_target_angle);
}

/* -------- 云台控制 -------- */
static float Gimbal_Error(float setpoint, float measured)
{
    float err = setpoint - measured;
    if      (err >  180.0f) err -= 360.0f;
    else if (err < -180.0f) err += 360.0f;
    return err;
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
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_I2C3_Init();
  MX_SPI3_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  char buffer[32] = {0};
  uint32_t last_tick = HAL_GetTick();
  LOG_Snprintf(buffer, sizeof(buffer), "Hello, World!\n");
  OLED_Init();
  OLED_Clear(); 
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  OLED_DisPlay_On();
  OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
  OLED_Refresh();
  MPU_Init();
  MPU6050_Calibrate(200);
  mpu_data = MPU6050_GetHandle();
  MadgwickAHRSsetSampleFreq(1000.0f / IMU_UPDATE_PERIOD_MS);
  ZDT_Init(&x_asix_motor,0x00,ZDT_Send_Tx_callback);
  HAL_TIM_Base_Start_IT(&htim2);

  /* PID初始化: kp=角度→RPM, ki=消除静差, kd=微分预判减速 */
  PID_Node_Init(&x_axis_pid, "gimbal_x", 3.0f, 0.05f, 0.8f);
  PID_Node_SetSetpoint(&x_axis_pid, 0.0f);
  PID_Custom_Functions custom = { .custom_error_calculation = Gimbal_Error };
  PID_Node_SetCustomCallback(&x_axis_pid, custom);
  PID_Node_SetLimit(&x_axis_pid,(PID_Limit){
    .setpoint_min   = -180.0f,
    .setpoint_max   =  180.0f,
    .input_min      = -180.0f,
    .input_max      =  180.0f,
    .output_min     = -500.0f,
    .output_max     =  500.0f,
    .integral_max   =  200.0f,
    .derivative_max =  200.0f,
    .deadband       =    0.1f,
  });
  // ZDT_VelMode(&x_asix_motor, ZDT_DIR_CW, 0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (HAL_GetTick() - last_tick >= 20)
    {
      LOG_Snprintf(buffer, sizeof(buffer), "yaw = %.2f", mpu_data->yaw);
      OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
      LOG_Snprintf(buffer, sizeof(buffer), "target = %.2f", x_axis_target_angle);
      OLED_ShowString(0,16, (uint8_t*)buffer, 16, 1);
      OLED_Refresh();
    }
    MulitKey_Scan(&key2);
    MulitKey_Scan(&key3);
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
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint32_t counter = 0;
  if (htim->Instance == TIM2) 
	{
    MPU6050_Update();

		/* 角度误差 → PID算目标速率 */
		PID_Node_UpdateMeasurement(&x_axis_pid, mpu_data->yaw);
		PID_ExecuteNode(&x_axis_pid, PID_DT);

		float vel_cmd = x_axis_pid.output;


		/* 发速度指令 */
		uint8_t  dir = (vel_cmd >= 0) ? ZDT_DIR_CCW : ZDT_DIR_CW;
		uint16_t vel = (uint16_t)(vel_cmd >= 0 ? vel_cmd : -vel_cmd);
		if (counter % 10 == 0)
		{
			ZDT_VelMode(&x_asix_motor, dir, vel);
		}
    counter++;
	}

}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    App_Protocol_FeedByte(rx_byte);
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
