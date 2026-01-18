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
#include "multikey.h"
#include <stdbool.h>
#include "Motor_AT4950.h"
#include "grayscale.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
MulitKey_t key1;
MulitKey_t key2;
MotorAT4950 motor1;
MotorAT4950 motor2;

uint8_t OLED_Display_Mode = 0;

int16_t ax_raw,ay_raw,az_raw;
int16_t gx_raw,gy_raw,gz_raw;
float ax,ay,az,gx,gy,gz;
float yaw,pitch,roll;

float ax_offset = 0, ay_offset = 0, az_offset = 0;
float gx_offset = 0, gy_offset = 0, gz_offset = 0;

// ï¿½Þ¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ç²ï¿½ï¿½ï¿½
const float GYRO_SCALE = 16.4f;    // ï¿½ï¿½2000dpsï¿½ï¿½ï¿½ï¿½È·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
const float ACCEL_SCALE = 16384.0f; // ï¿½ï¿½2gï¿½Ä±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
const float GYRO_AMPLIFY = 0.47f;   // ï¿½ï¿½ï¿½ï¿½Å´ï¿½ï¿½ï¿½ï¿½ï¿?
uint32_t last_update = 0;

extern tPid pidAngularVelocity;  // ½ÇËÙ¶È»·
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern tPid pidMotor1Speed;
extern tPid pidMotor2Speed;
extern tPid pid_yaw;
extern tPid pid_grayscale;

uint8_t gray_byte = 0;
float gray_error = 0.0f;

// Ôö¼Ó×´Ì¬»ú
typedef enum {
    STATE_LINE_FOLLOWING = 0,    // Õý³£Ñ°¼£
    STATE_90_DEGREE_TURN = 1,    // Ö±½Ç×ªÍäÖÐ
    STATE_TURN_COMPLETE = 2      // ×ªÍäÍê³É
} RobotState;

RobotState currentState = STATE_LINE_FOLLOWING;
uint32_t turn_start_time = 0;
float target_turn_angle = 0.0f;

// Ö±½ÇÍä¼ì²âÏà¹Ø
#define TURN_THRESHOLD 4.0f      // ¼ì²âµ½¼«¶ËÆ«²îÊ±µÄãÐÖµ
float last_gray_error = 0.0f;
uint8_t consecutive_extreme_errors = 0;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define ENCODER_PPR 13                  // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿??
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

// PIDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½ï¿½ï¿½ï¿½Êµï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿??
#define KP_SPEED 3.0f            // ï¿½Ù¶È»ï¿½ï¿½ï¿½ï¿½ï¿½Ïµï¿½ï¿½
#define KI_SPEED 0.8f            // ï¿½Ù¶È»ï¿½ï¿½ï¿½ï¿½ï¿½Ïµï¿½ï¿½
#define KD_SPEED 0.2f            // ï¿½Ù¶È»ï¿½Î¢ï¿½ï¿½Ïµï¿½ï¿½

// PIDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿??
float A_Error_Integral = 0.0;    // Aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
float A_Last_Error = 0.0;        // Aï¿½ï¿½ï¿½Ï´ï¿½ï¿½ï¿½ï¿??
float B_Error_Integral = 0.0;    // Bï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½  
float B_Last_Error = 0.0;        // Bï¿½ï¿½ï¿½Ï´ï¿½ï¿½ï¿½ï¿??

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
    char buffer[20];
    // Ð£×¼Ê±ï¿½ï¿½ï¿½Ö´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½È«ï¿½ï¿½Ö¹
//    OLED_Clear();
    OLED_ShowString(0, 0, "Calibrating...", 16,1);
    OLED_ShowString(0, 16, "Keep Still!", 16,1);
    for(int i = 0; i < samples; i++) {
        MPU_Get_Accelerometer(&ax_raw, &ay_raw, &az_raw);
        MPU_Get_Gyroscope(&gx_raw, &gy_raw, &gz_raw);
        
        ax_sum += ax_raw;
        ay_sum += ay_raw;
        az_sum += az_raw;
        gx_sum += gx_raw;
        gy_sum += gy_raw;
        gz_sum += gz_raw;
		snprintf(buffer,sizeof(buffer),"try %d times",i);
		OLED_ShowString(0, 32, buffer, 16,1);
		OLED_Refresh();
        HAL_Delay(10);
    }
    
    // ï¿½ï¿½ï¿½ï¿½Æ«ï¿½ï¿½ï¿½ï¿½
    ax_offset = (ax_sum / (float)samples) / ACCEL_SCALE;
    ay_offset = (ay_sum / (float)samples) / ACCEL_SCALE;
    az_offset = ((az_sum / (float)samples) / ACCEL_SCALE) - 1.0f; // ï¿½ï¿½È¥ï¿½ï¿½ï¿½ï¿½
    
    gx_offset = (gx_sum / (float)samples) / GYRO_SCALE * (M_PI / 180.0f);
    gy_offset = (gy_sum / (float)samples) / GYRO_SCALE * (M_PI / 180.0f);
    gz_offset = (gz_sum / (float)samples) / GYRO_SCALE ;
    
    OLED_Clear();
    OLED_ShowString(0, 0, "Calibration", 16,1);
    OLED_ShowString(0, 16, "Complete!", 16,1);
    OLED_Refresh();
    HAL_Delay(1000);
    OLED_Clear();
}

void Key1PressdCallback(MulitKey_t* key)
{
  OLED_Display_Mode = (OLED_Display_Mode + 1) % 3;
  OLED_Clear();
}

uint8_t Key1ReadPinCallback(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin);
}

void Key1LongPressdCallback(MulitKey_t* key)
{
  HAL_GPIO_TogglePin(LED3_GPIO_Port,LED3_Pin);
}

void SetMotor1ComparePWM(uint16_t ccr_value)
{
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, ccr_value);
}

void SetMotor2ComparePWM(uint16_t ccr_value)
{
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, ccr_value);
}

void SetMotor1IN1Level(uint8_t level)
{
  HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, level);
}

void SetMotor2IN1Level(uint8_t level)
{
  HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin,level);
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
  // ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ï¿½PWM
  MulitKey_Init(&key1, Key1ReadPinCallback, Key1PressdCallback, Key1LongPressdCallback, RISE_BORDER_TRIGGER);
  MotorInit_AT46950(&motor1 ,SetMotor1ComparePWM,SetMotor1IN1Level,100);
  MotorInit_AT46950(&motor2 ,SetMotor2ComparePWM,SetMotor2IN1Level,100);

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // ï¿½ï¿½ï¿?1 PWM
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // ï¿½ï¿½ï¿?2 PWM

  // ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
  // HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, GPIO_PIN_SET);  // ï¿½ï¿½ï¿?1ï¿½ï¿½ï¿½ï¿½
  // HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, GPIO_PIN_SET);  // ï¿½ï¿½ï¿?2ï¿½ï¿½ï¿½ï¿½

  // // ï¿½ï¿½ï¿½Ã½ÏµÍµÄ³ï¿½Ê¼PWMÖµï¿½ï¿½ï¿½ï¿½Ö¹ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ì£©
  // __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 80);
  // __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, 80);
  SetDefaultDirection(&motor1,High_Level);
  SetDefaultDirection(&motor2,High_Level);
  Motor_setSpeed(&motor1, 0);
  Motor_setSpeed(&motor2, 0);

  // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);  // ï¿½ï¿½ï¿?1ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);  // ï¿½ï¿½ï¿?2ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½

  // ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½PIDï¿½ï¿½ï¿½ï¿½

  // ï¿½ï¿½Ê¼ï¿½ï¿½PIDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
  PID_init();
  
  // ï¿½ï¿½ï¿½Ã½ÏµÍµï¿½Ä¿ï¿½ï¿½ï¿½Ù¶È£ï¿½ï¿½ï¿½ï¿½ï¿½Ä£Ê½ï¿½ï¿½
//  Target_Speed_A = 0.40f;  // 0.15 m/s
//  Target_Speed_B = 0.40f;
//  
//  // ÉèÖÃPIDÄ¿±êÖµ
//  pidMotor1Speed.target_val = Target_Speed_A;
//  pidMotor2Speed.target_val = Target_Speed_B;
  
  OLED_Init();
  OLED_Clear();
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  OLED_DisPlay_On();
  
  MPU_Init();
  SimpleCalibrate();  
//  pid_yaw.target_val = yaw;
  HAL_TIM_Base_Start_IT(&htim4);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      MulitKey_Scan(&key1);
//	  HAL_GPIO_TogglePin(LED3_GPIO_Port,LED3_Pin);
//	  HAL_Delay(500);
	  if(HAL_GetTick() - last_update >= 20) {
        MPU_Get_Accelerometer(&ax_raw, &ay_raw, &az_raw);
        MPU_Get_Gyroscope(&gx_raw, &gy_raw, &gz_raw);
        
        // ï¿½ï¿½È·ï¿½Äµï¿½Î»×ªï¿½ï¿½
        // ï¿½ï¿½ï¿½Ù¶È¼Æ£ï¿½×ªï¿½ï¿½Îªg (ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ù¶Èµï¿½Î»)
        
        // ï¿½ï¿½Ê¾ï¿½ï¿½ï¿?
//        OLED_Clear();
        char buffer[20];
        if (OLED_Display_Mode == 0)
        {
          snprintf(buffer, sizeof(buffer), "Y:%.1f", yaw);
          OLED_ShowString(0, 0, (uint8_t*)buffer, 16,1);
          
          snprintf(buffer, sizeof(buffer), "Y_TAR:%.2f", pid_yaw.target_val);
          OLED_ShowString(0, 16, (uint8_t*)buffer, 16,1);
          
          snprintf(buffer, sizeof(buffer), "R:%.1f", roll);
          OLED_ShowString(0, 32, (uint8_t*)buffer, 16,1);
			
          snprintf(buffer, sizeof(buffer), "Y_target:%.1f", pid_yaw.target_val);
          OLED_ShowString(0, 48, (uint8_t*)buffer, 16,1);

          // snprintf(buffer, sizeof(buffer), "mode = %d", OLED_Display_Mode);
          // OLED_ShowString(0, 80, (uint8_t*)buffer, 16,1);
        }
        else if (OLED_Display_Mode == 1)
        {

          snprintf(buffer, sizeof(buffer), "gray:");
          OLED_ShowString(0, 0, (uint8_t*)buffer, 16,1);

          
          for (size_t i = 0; i < 8; i++)
          {
              if (gray_byte & (1 << i))
              {
                  OLED_ShowString(48 + i * 8, 0, (uint8_t *)"1", 16, 1);
              }
              else
              {
                  OLED_ShowString(48 + i * 8, 0, (uint8_t *)"0", 16, 1);
              }
          }

          snprintf(buffer, sizeof(buffer), "gray_err:%.2f", gray_error);
          OLED_ShowString(0, 16, (uint8_t*)buffer, 16,1);

        }
        else if (OLED_Display_Mode == 2)
        {
          snprintf(buffer, sizeof(buffer), "val_A:%.2f",Actual_Speed_A);
          OLED_ShowString(0, 0, (uint8_t*)buffer, 16,1);

          snprintf(buffer, sizeof(buffer), "val_B:%.2f",Actual_Speed_B);
          OLED_ShowString(0, 16, (uint8_t*)buffer, 16,1);

          snprintf(buffer, sizeof(buffer), "val_A_T:%.2f",pidMotor1Speed.target_val);
          OLED_ShowString(0, 32, (uint8_t*)buffer, 16,1);

          snprintf(buffer, sizeof(buffer), "val_B_T:%.2f",pidMotor2Speed.target_val);
          OLED_ShowString(0, 48, (uint8_t*)buffer, 16,1);
        }
        
        OLED_Refresh();
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
// ¼ì²âÊÇ·ñÎªÖ±½ÇÍä
uint8_t detect90DegreeTurn(float current_error) {
    static float error_history[3] = {0};
    static uint8_t history_index = 0;
    
    // ¸üÐÂÀúÊ·Êý¾Ý
    error_history[history_index] = current_error;
    history_index = (history_index + 1) % 3;
    
    // ¼ì²éÊÇ·ñ³öÏÖ¼«¶ËÆ«²î
    if(fabsf(current_error) > TURN_THRESHOLD) {
        consecutive_extreme_errors++;
        
        // Á¬Ðø¼ì²âµ½¼«¶ËÆ«²î£¬È·ÈÏÊÇÖ±½ÇÍä
        if(consecutive_extreme_errors >= 2) {
            consecutive_extreme_errors = 0;
            return 1;
        }
    } else {
        consecutive_extreme_errors = 0;
    }
    
    return 0;
}

// Õý³£Ñ°¼£´¦Àí
void handleLineFollowing(void) {
    // ¼ì²âÖ±½ÇÍä
    if(detect90DegreeTurn(gray_error)) {
        currentState = STATE_90_DEGREE_TURN;
        turn_start_time = HAL_GetTick();
        
        // ¸ù¾Ý»Ò¶ÈÎó²î·½ÏòÈ·¶¨×ªÍä·½Ïò
        if(gray_error > 0) {
            target_turn_angle = yaw + 90.0f;  // ×ó×ª90¶È
        } else {
            target_turn_angle = yaw - 90.0f;  // ÓÒ×ª90¶È
        }
        
        // ½Ç¶È¹éÒ»»¯
        if(target_turn_angle > 180.0f) target_turn_angle -= 360.0f;
        if(target_turn_angle < -180.0f) target_turn_angle += 360.0f;
        
        return;
    }
    
    // Õý³£PID¿ØÖÆ£¨Ô­ÓÐµÄ»Ò¶È¿ØÖÆ£©
    if (fabsf(gray_error) < 0.01f) {
        gray_error = 0.0f;
    }
    
    float gray_out = PID_realize(&pid_grayscale, gray_error);
    
    // ÏÞ·ù
    const float GRAY_OUT_MAX = 0.25f;
    if (gray_out > GRAY_OUT_MAX) gray_out = GRAY_OUT_MAX;
    if (gray_out < -GRAY_OUT_MAX) gray_out = -GRAY_OUT_MAX;
    
    // ÉèÖÃÄ¿±êËÙ¶È
    float base_speed = 0.16f;
    float adjust_speedA = base_speed + 0.1 * gray_out;
    float adjust_speedB = base_speed - 0.1 * gray_out;
    
    pidMotor1Speed.target_val = fmaxf(adjust_speedA, 0);
    pidMotor2Speed.target_val = fmaxf(adjust_speedB, 0);
}

// Ö±½Ç×ªÍä´¦Àí
void handle90DegreeTurn(void) {
    // Ê¹ÓÃMPU6050µÄ½Ç¶È½øÐÐ¿ØÖÆ
    float angle_error = target_turn_angle - yaw;
    
    // ½Ç¶È¹éÒ»»¯µ½[-180, 180]
    while(angle_error > 180.0f) angle_error -= 360.0f;
    while(angle_error < -180.0f) angle_error += 360.0f;
    
    // ÉèÖÃ½Ç¶ÈPIDµÄÄ¿±êÖµ
    pid_yaw.target_val = target_turn_angle;
    float turn_output = PID_realize(&pid_yaw, yaw);
    
    // ÏÞ·ù
    const float TURN_OUTPUT_MAX = 0.4f;
    if(turn_output > TURN_OUTPUT_MAX) turn_output = TURN_OUTPUT_MAX;
    if(turn_output < -TURN_OUTPUT_MAX) turn_output = -TURN_OUTPUT_MAX;
    
    // ÉèÖÃµç»úËÙ¶È£ºÒ»¸öÂÖ×ÓÕý×ª£¬Ò»¸öÂÖ×Ó·´×ªÊµÏÖÔ­µØ×ªÍä
    float turn_speed = 0.35f;  // ×ªÍäËÙ¶È
    
    if(angle_error > 0) {  // ×ó×ª
        pidMotor1Speed.target_val = -turn_speed + turn_output;
        pidMotor2Speed.target_val = turn_speed + turn_output;
    } else {  // ÓÒ×ª
        pidMotor1Speed.target_val = turn_speed + turn_output;
        pidMotor2Speed.target_val = -turn_speed + turn_output;
    }
    
    // ¼ì²éÊÇ·ñÍê³É×ªÍä
    if(fabsf(angle_error) < 5.0f) {  // Îó²îÐ¡ÓÚ5¶ÈÈÏÎªÍê³É
        currentState = STATE_TURN_COMPLETE;
        turn_start_time = HAL_GetTick();
        
        // Í£Ö¹µç»ú
        pidMotor1Speed.target_val = 0;
        pidMotor2Speed.target_val = 0;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint32_t count = 0;
    static int32_t last_A_count = 0, last_B_count = 0;
    
    if (htim->Instance == TIM4)
    {
        count++;
        
        // *** 1. Ã¿2´ÎÖÐ¶Ï£¨Ô¼8ms£©Ö´ÐÐÖ÷¿ØÖÆ ***
        if(count % 2 == 0)
        {
            // 1.1 ¸üÐÂ½ÇËÙ¶È
            gz = ((gz_raw / GYRO_SCALE) - gz_offset) * GYRO_AMPLIFY;
            yaw += gz * 0.004f;
            
            // 1.2 ¶ÁÈ¡»Ò¶È
            gray_byte = 0;
            gray_byte = GPIOE->IDR;
            gray_error = CalculateGrayError_Advanced(gray_byte);
            if (fabsf(gray_error) < 0.01f)
            {
              gray_error = 0.0f;
            }
            // ×´Ì¬»ú´¦Àí
            switch(currentState) {
                case STATE_LINE_FOLLOWING:
                    // Õý³£Ñ°¼£Ä£Ê½
                    handleLineFollowing();
                    break;
                    
                case STATE_90_DEGREE_TURN:
                    // Ö±½Ç×ªÍäÄ£Ê½
                    handle90DegreeTurn();
                    break;
                    
                case STATE_TURN_COMPLETE:
                    // ×ªÍäÍê³É£¬¶ÌÔÝÑÓ³Ùºó»Øµ½Ñ°¼£
                    if(HAL_GetTick() - turn_start_time > 500) {
                        currentState = STATE_LINE_FOLLOWING;
                        pid_grayscale.err_sum = 0.0f;  // ÖØÖÃ»Ò¶È»ý·Ö
                    }
                    break;
            }            
            // 1.3 ¸üÐÂËÙ¶È·´À¡£¨¸ÄÎªÃ¿´Î¿ØÖÆ¶¼¸üÐÂ£¡£©
            int32_t current_A = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
            int32_t current_B = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
            
            // ¼ÆËã±àÂëÆ÷²îÖµ£¨´¦ÀíÒç³ö£©
            int32_t diff_A = current_A - last_A_count;
            int32_t diff_B = current_B - last_B_count;
            
            // ¸üÐÂÀúÊ·Öµ
            last_A_count = current_A;
            last_B_count = current_B;
            
            // ¼ÆËãÊµÊ±ËÙ¶È£¨Ê¹ÓÃÊµ¼ÊÊ±¼ä¼ä¸ô£©
            static uint32_t last_time = 0;
            uint32_t current_time = HAL_GetTick();
            float dt = (current_time - last_time) / 1000.0f; // µ¥Î»£ºÃë
            if(dt > 0)
            {
                // ×ªËÙ = (Âö³åÊý * ³µÂÖÖÜ³¤) / (Ã¿×ªÂö³åÊý * Ê±¼ä)
                Actual_Speed_A = -(diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt);
                Actual_Speed_B = (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt);
            }
            last_time = current_time;
            
            // ¼òµ¥ÂË²¨
            static float filtered_A = 0, filtered_B = 0;
            filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
            filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
            Actual_Speed_A = filtered_A;
            Actual_Speed_B = filtered_B;
            
            // 1.4 »Ò¶È¿ØÖÆ
            float gray_out = PID_realize(&pid_grayscale, gray_error);
            if (fabsf(gray_error) < 0.01f) {
                pid_grayscale.err_sum = 0.0f;  // ÖØÖÃ»ý·ÖÏî
            }
            
            // *** ÐÂÔö£ºÏÞÖÆ»ý·ÖÏîÔö³¤ËÙ¶È ***
            // static float last_gray_error = 0.0f;
            // float error_change = gray_error - last_gray_error;
            // last_gray_error = gray_error;
            
            // // Èç¹ûÎó²î±ä»¯Ì«´ó£¬¿ÉÄÜÊÇ´«¸ÐÆ÷ÔëÉù£¬²»»ý·Ö
            // if (fabsf(error_change) > 0.5f) {
            //     pid_grayscale.err_sum *= 0.8f;  // Ë¥¼õ»ý·ÖÏî
            // }
            
            // ËÀÇø´¦Àí
            if (fabsf(gray_out) < 0.05f)  // ¼õÐ¡ËÀÇø
            {
                gray_out = 0.0f;
            }
			
            // *** ÐÂÔö£º»Ò¶È»·Êä³ö×÷Îª½ÇËÙ¶È»·µÄÄ¿±êÖµ ***
			
            // ½«»Ò¶È»·Êä³ö×ª»»ÎªÆÚÍû½ÇËÙ¶È£¨¶È/Ãë£©
            float target_angular_velocity = gray_out * 0.025f; // Ëõ·ÅÏµÊýÐèÒªµ÷Õû
            
            // »ñÈ¡µ±Ç°½ÇËÙ¶È£¨´ÓMPU6050£©
            float current_angular_velocity = gz; // ¼ÙÉègzµ¥Î»ÒÑ¾­ÊÇ¶È/Ãë
            
            // *** ½ÇËÙ¶È»·¿ØÖÆ ***
            pidAngularVelocity.target_val = target_angular_velocity;
            float angular_out = PID_realize(&pidAngularVelocity, current_angular_velocity);
            
            // ½ÇËÙ¶È»·Êä³öÏÞ·ù£¨È·±£²»»á¹ý´ó£©
            if (angular_out > 0.5f) angular_out = 0.5f;
            if (angular_out < -0.5f) angular_out = -0.5f;  
			
            // ÏÞ·ù
            // const float GRAY_OUT_MAX = 0.25f;  // ¼õÐ¡×î´óµ÷ÕûÁ¿
            // if (gray_out > GRAY_OUT_MAX) gray_out = GRAY_OUT_MAX;
            // if (gray_out < -GRAY_OUT_MAX) gray_out = -GRAY_OUT_MAX;
            
            // // 1.5 ÉèÖÃÄ¿±êËÙ¶È
            float base_speed = 0.10f;  // ½µµÍ»ù´¡ËÙ¶È
            float adjust_speedA = base_speed + 0.1 * gray_out;
            float adjust_speedB = base_speed - 0.1 * gray_out;
            
            pidMotor1Speed.target_val = fmaxf(adjust_speedA, 0);  // È·±£²»Îª¸º
            pidMotor2Speed.target_val = fmaxf(adjust_speedB, 0);
            // printf("gray_out:%.2f,gray_error:%.2f,gray_byte=0x%2x\r\n",gray_out,gray_error,gray_byte);
            // printf("targetA_val:%.2f\r\n",pidMotor1Speed.target_val);
            // printf("targetB_val:%.2f\r\n",pidMotor2Speed.target_val);
            // printf("adjust_speedA:%.2f\r\n",adjust_speedA);
            // printf("adjust_speedB:%.2f\r\n",adjust_speedB);
            // 1.6 ËÙ¶ÈPID¿ØÖÆ
            float motor1_out = PID_realize(&pidMotor1Speed, Actual_Speed_A);
            float motor2_out = PID_realize(&pidMotor2Speed, Actual_Speed_B);
            
            // 1.7 ÉèÖÃµç»ú
            Motor_setSpeed(&motor1, motor1_out);
            Motor_setSpeed(&motor2, motor2_out);

            printf("%.2f,%.2f,%d,%d\n",pidMotor1Speed.actual_val
                                              ,pidMotor2Speed.actual_val
                                              // ,pidMotor1Speed.target_val
                                              // ,pidMotor2Speed.target_val
                                              ,(int16_t)__HAL_TIM_GetCompare(&htim1,TIM_CHANNEL_1)
                                              ,(int16_t)__HAL_TIM_GetCompare(&htim1,TIM_CHANNEL_2));
        }
        
        // *** 2. Ã¿25´ÎÖÐ¶Ï¸üÐÂMPU6050×ËÌ¬ ***
        if(count % 25 == 0)
        {
            // Ô­ÓÐµÄMPU6050Êý¾Ý´¦Àí...
	    ax = (ax_raw / ACCEL_SCALE) - ax_offset;
        ay = (ay_raw / ACCEL_SCALE) - ay_offset;
        az = (az_raw / ACCEL_SCALE) - az_offset;
        
        // ï¿½ï¿½ï¿½ï¿½ï¿½Ç£ï¿½×ªï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½ (Madgwickï¿½ã·¨Òªï¿½ï¿½Äµï¿½Î?)
        // ï¿½ï¿½×ªï¿½ï¿½Îªï¿½ï¿½/ï¿½ë£¬ï¿½ï¿½×ªï¿½ï¿½Îªï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½
		gx = ((gx_raw / GYRO_SCALE) * (M_PI / 180.0f) - gx_offset) * GYRO_AMPLIFY;
        gy = ((gy_raw / GYRO_SCALE) * (M_PI / 180.0f) - gy_offset) * GYRO_AMPLIFY;
        
        // ï¿½ï¿½ï¿½ï¿½Madgwickï¿½ã·¨
        MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az);
        
        // ï¿½ï¿½È¡Å·ï¿½ï¿½ï¿½Ç£ï¿½ï¿½Ñ¾ï¿½ï¿½Ç½Ç¶ï¿½ï¿½Æ£ï¿½
//        Get_Euler_Angles(&yaw, &pitch, &roll);
//        yaw_inter = yaw_inter + gz_raw  ;
        // ï¿½ï¿½ï¿½Æ½Ç¶È·ï¿½Î§ï¿½ï¿½ -180ï¿½ï¿½ ï¿½ï¿½ +180ï¿½ï¿½
        if(yaw > 180.0f) yaw -= 360.0f;
        if(yaw < -180.0f) yaw += 360.0f;
        if(pitch > 180.0f) pitch -= 360.0f;
        if(pitch < -180.0f) pitch += 360.0f;
        if(roll > 180.0f) roll -= 360.0f;
        if(roll < -180.0f) roll += 360.0f;			
        }
        
        // *** 3. Ã¿50´ÎÖÐ¶ÏÖØÖÃ±àÂëÆ÷¼ÆÊýÆ÷£¨·ÀÖ¹Òç³ö£©***
        if(count >= 50)
        {
            __HAL_TIM_SetCounter(&htim2, 0);
            __HAL_TIM_SetCounter(&htim3, 0);
            last_A_count = 0;
            last_B_count = 0;
            count = 0;
            HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
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
