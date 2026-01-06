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
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "pid.h"
#include "oled.h"
#include "mpu6050.h"
#include "MadgwickAHRS.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int16_t ax_raw,ay_raw,az_raw;
int16_t gx_raw,gy_raw,gz_raw;
float ax,ay,az,gx,gy,gz;
float yaw,pitch,roll;

float ax_offset = 0, ay_offset = 0, az_offset = 0;
float gx_offset = 0, gy_offset = 0, gz_offset = 0;

const float GYRO_SCALE = 65.5f;    // ï¿½ï¿½2000dpsï¿½ï¿½ï¿½ï¿½È·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
const float ACCEL_SCALE = 16384.0f; // ï¿½ï¿½2gï¿½Ä±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
const float GYRO_AMPLIFY = 0.05f;   // ï¿½ï¿½ï¿½ï¿½Å´ï¿½ï¿½ï¿½ï¿½ï¿½
uint32_t last_update = 0;
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
#define ENCODER_PPR 13                  // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½?
#define GEAR_RATIO 20                   // ï¿½ï¿½ï¿½Ù±ï¿½ 1:20
#define OUTPUT_SHAFT_PPR 260            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð§ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ = 13 ï¿½ï¿½ 20
#define EFFECTIVE_PPR 1040              // ï¿½Ä±ï¿½Æµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ = 260 ï¿½ï¿½ 4
#define WHEEL_DIAMETER 0.048            // ï¿½ï¿½ï¿½ï¿½Ö±ï¿½ï¿½48mm = 0.048m
#define WHEEL_CIRCUMFERENCE 0.1508      // ï¿½ï¿½ï¿½ï¿½ï¿½Ü³ï¿½ = ï¿½ï¿½ ï¿½ï¿½ 0.048 ï¿½ï¿½ 0.1508m
#define SAMPLING_TIME 0.1              // ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½20ms = 0.02s

// ï¿½Ù¶È¼ï¿½ï¿½ï¿½Ïµï¿½ï¿½
#define SPEED_CALC_FACTOR (WHEEL_CIRCUMFERENCE / (EFFECTIVE_PPR * SAMPLING_TIME))
// ï¿½ò»¯¼ï¿½ï¿½ï¿½: ï¿½Ù¶ï¿½ = ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ 0.000145

// ï¿½Ù¶È»ï¿½È«ï¿½Ö±ï¿½ï¿½ï¿½
float Target_Speed_A = 0.0;      // Aï¿½ï¿½Ä¿ï¿½ï¿½ï¿½Ù¶ï¿½ (m/s)
float Target_Speed_B = 0.0;      // Bï¿½ï¿½Ä¿ï¿½ï¿½ï¿½Ù¶ï¿½ (m/s)
float Actual_Speed_A = 0.0;      // Aï¿½ï¿½Êµï¿½ï¿½ï¿½Ù¶ï¿½ (m/s)  
float Actual_Speed_B = 0.0;      // Bï¿½ï¿½Êµï¿½ï¿½ï¿½Ù¶ï¿½ (m/s)
int32_t A_Count = 0;             // Aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
int32_t B_Count = 0;             // Bï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½

// PIDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½ï¿½ï¿½ï¿½Êµï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½?
#define KP_SPEED 3.0f            // ï¿½Ù¶È»ï¿½ï¿½ï¿½ï¿½ï¿½Ïµï¿½ï¿½
#define KI_SPEED 0.8f            // ï¿½Ù¶È»ï¿½ï¿½ï¿½ï¿½ï¿½Ïµï¿½ï¿½
#define KD_SPEED 0.2f            // ï¿½Ù¶È»ï¿½Î¢ï¿½ï¿½Ïµï¿½ï¿½

// PIDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½?
float A_Error_Integral = 0.0;    // Aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
float A_Last_Error = 0.0;        // Aï¿½ï¿½ï¿½Ï´ï¿½ï¿½ï¿½ï¿½?
float B_Error_Integral = 0.0;    // Bï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½  
float B_Last_Error = 0.0;        // Bï¿½ï¿½ï¿½Ï´ï¿½ï¿½ï¿½ï¿½?

// PWMï¿½ï¿½ï¿½ï¿½
#define MAX_PWM 100              // ï¿½ï¿½ï¿½PWMÖµ
#define MIN_PWM 0                // ï¿½ï¿½Ð¡PWMÖµ
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SimpleCalibrate(void)
{
    int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
    int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;
    const uint16_t samples = 200;
    
    // Ð£×¼Ê±ï¿½ï¿½ï¿½Ö´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½È«ï¿½ï¿½Ö¹
//    OLED_Clear();
    OLED_ShowString(0, 0, "Calibrating...", 16,1);
    OLED_ShowString(0, 16, "Keep Still!", 16,1);
    OLED_Refresh();
    for(int i = 0; i < samples; i++) {
        MPU_Get_Accelerometer(&ax_raw, &ay_raw, &az_raw);
        MPU_Get_Gyroscope(&gx_raw, &gy_raw, &gz_raw);
        
        ax_sum += ax_raw;
        ay_sum += ay_raw;
        az_sum += az_raw;
        gx_sum += gx_raw;
        gy_sum += gy_raw;
        gz_sum += gz_raw;
        
        HAL_Delay(10);
    }
    
    // ï¿½ï¿½ï¿½ï¿½Æ«ï¿½ï¿½ï¿½ï¿½
    ax_offset = (ax_sum / (float)samples) / ACCEL_SCALE;
    ay_offset = (ay_sum / (float)samples) / ACCEL_SCALE;
    az_offset = ((az_sum / (float)samples) / ACCEL_SCALE) - 1.0f; // ï¿½ï¿½È¥ï¿½ï¿½ï¿½ï¿½
    
    gx_offset = (gx_sum / (float)samples) / GYRO_SCALE * (M_PI / 180.0f);
    gy_offset = (gy_sum / (float)samples) / GYRO_SCALE * (M_PI / 180.0f);
    gz_offset = (gz_sum / (float)samples) / GYRO_SCALE * (M_PI / 180.0f);
    
    OLED_Clear();
    OLED_ShowString(0, 0, "Calibration", 16,1);
    OLED_ShowString(0, 16, "Complete!", 16,1);
	OLED_Refresh();
    HAL_Delay(1000);
    OLED_Clear();
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // ³õÊ¼»¯µç»úPWM
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // µç»ú1 PWM
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // µç»ú2 PWM

  // ³õÊ¼»¯µç»ú·½ÏòÒý½Å
  HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, GPIO_PIN_SET);  // µç»ú1·½Ïò
  HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, GPIO_PIN_SET);  // µç»ú2·½Ïò

  // ÉèÖÃ½ÏµÍµÄ³õÊ¼PWMÖµ£¨·ÀÖ¹Æô¶¯¹ý¿ì£©
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 90);
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, 90);

  // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);  // µç»ú1±àÂëÆ÷
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);  // µç»ú2±àÂëÆ÷

  // ³õÊ¼»¯¶¨Ê±Æ÷ÓÃÓÚPID¼ÆËã
  HAL_TIM_Base_Start_IT(&htim4);

  // ³õÊ¼»¯PID¿ØÖÆÆ÷
  PID_init();
  
  // ÉèÖÃ½ÏµÍµÄÄ¿±êËÙ¶È£¨µÍËÙÄ£Ê½£©
  Target_Speed_A = 0.5f;  // 0.15 m/s
  Target_Speed_B = 0.5f;
  
  OLED_Init();
  OLED_Clear();
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  OLED_DisPlay_On();
  
  MPU_Init();
  SimpleCalibrate();  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(HAL_GetTick() - last_update >= 20) {
        MPU_Get_Accelerometer(&ax_raw, &ay_raw, &az_raw);
        MPU_Get_Gyroscope(&gx_raw, &gy_raw, &gz_raw);
        
        // ï¿½ï¿½È·ï¿½Äµï¿½Î»×ªï¿½ï¿½
        // ï¿½ï¿½ï¿½Ù¶È¼Æ£ï¿½×ªï¿½ï¿½Îªg (ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ù¶Èµï¿½Î»)
        
        // ï¿½ï¿½Ê¾ï¿½ï¿½ï¿½
//        OLED_Clear();
        char buffer[20];
        
        snprintf(buffer, sizeof(buffer), "Y:%.1f", yaw);
        OLED_ShowString(0, 16, (uint8_t*)buffer, 16,1);
        
        snprintf(buffer, sizeof(buffer), "P:%.1f", pitch);
        OLED_ShowString(0, 32, (uint8_t*)buffer, 16,1);
        
        snprintf(buffer, sizeof(buffer), "R:%.1f", roll);
        OLED_ShowString(0, 64, (uint8_t*)buffer, 16,1);
		 OLED_Refresh();
        snprintf(buffer,sizeof(buffer),"Y:%.1f,gz_raw:%d\n",yaw,gz_raw);
		HAL_UART_Transmit(&huart1,(uint8_t*)buffer,strlen(buffer),HAL_MAX_DELAY);
        last_update = HAL_GetTick();
    }
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
	if(count % 25 == 0)
		{
			        ax = (ax_raw / ACCEL_SCALE) - ax_offset;
        ay = (ay_raw / ACCEL_SCALE) - ay_offset;
        az = (az_raw / ACCEL_SCALE) - az_offset;
        
        // ï¿½ï¿½ï¿½ï¿½ï¿½Ç£ï¿½×ªï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½ (Madgwickï¿½ã·¨Òªï¿½ï¿½Äµï¿½Î»)
        // ï¿½ï¿½×ªï¿½ï¿½Îªï¿½ï¿½/ï¿½ë£¬ï¿½ï¿½×ªï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½
		gx = ((gx_raw / GYRO_SCALE) * (M_PI / 180.0f) - gx_offset) * GYRO_AMPLIFY;
        gy = ((gy_raw / GYRO_SCALE) * (M_PI / 180.0f) - gy_offset)* GYRO_AMPLIFY;
        gz = ((gz_raw / GYRO_SCALE) * (M_PI / 180.0f) - gz_offset)* GYRO_AMPLIFY;
        
        // ï¿½ï¿½ï¿½ï¿½Madgwickï¿½ã·¨
        MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az);
        
        // ï¿½ï¿½È¡Å·ï¿½ï¿½ï¿½Ç£ï¿½ï¿½Ñ¾ï¿½ï¿½Ç½Ç¶ï¿½ï¿½Æ£ï¿½
        Get_Euler_Angles(&yaw, &pitch, &roll);
//        yaw_inter = yaw_inter + gz_raw  ;
        // ï¿½ï¿½ï¿½Æ½Ç¶È·ï¿½Î§ï¿½ï¿½ -180ï¿½ï¿½ ï¿½ï¿½ +180ï¿½ï¿½
        if(yaw > 180.0f) yaw -= 360.0f;
        if(yaw < -180.0f) yaw += 360.0f;
        if(pitch > 180.0f) pitch -= 360.0f;
        if(pitch < -180.0f) pitch += 360.0f;
        if(roll > 180.0f) roll -= 360.0f;
        if(roll < -180.0f) roll += 360.0f;

	}			
    if (count >= 50)
    {
      A_Count =  (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
      B_Count =  (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
      __HAL_TIM_SetCounter(&htim2, 0);
      __HAL_TIM_SetCounter(&htim3, 0);
      Actual_Speed_A = A_Count * SPEED_CALC_FACTOR;
      Actual_Speed_B = B_Count * SPEED_CALC_FACTOR;
      
      // ï¿½Ù¶ï¿½ï¿½Ë²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
      static float filtered_A = 0.0, filtered_B = 0.0;
      filtered_A = 0.6f * filtered_A + 0.4f * Actual_Speed_A;
      filtered_B = 0.6f * filtered_B + 0.4f * Actual_Speed_B;
      Actual_Speed_A = filtered_A;
      Actual_Speed_B = filtered_B;
	 
		
    }
    if (count >= 50)
    {
		HAL_GPIO_TogglePin(LED3_GPIO_Port,LED3_Pin);
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
