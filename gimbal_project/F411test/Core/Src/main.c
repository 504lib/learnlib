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
// #define STATIC_QUEUE_ENTER_CRITICAL() __disable_irq()
// #define STATIC_QUEUE_EXIT_CRITICAL()  __enable_irq()
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
#include "app_menu.h"
#include "Task_Control.h"
#include "Task4_Control.h"
#include "ZDT_Pulse_Control.h"
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
void __uart2_tx(const char* buffer, size_t buffer_size)
{
  (void)buffer;
  (void)buffer_size;
  // HAL_UART_Transmit(&huart6, (uint8_t*)buffer, buffer_size, HAL_MAX_DELAY);
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t rx_byte = 0;
volatile uint8_t rx_byte_uart1 = 0;


size_t text_len = 0;
bool text_buffer_updated = false;
volatile uint32_t rx_total = 0;
MPU6050_Data_t* mpu_data = NULL;
ZDT_Motor_Handle_t x_asix_motor;
MulitKey_t key1;
MulitKey_t key2;
MulitKey_t key3;
PID_Node x_axis_pid;
volatile float x_axis_target_angle = 0.0f;  // 目标角度，单位：度
static uint8_t Oled_page_index = 0;
static bool Enter_Oled_page = false;
static bool Task3_excuting = false;
static float man_angle = 28.0f;   /* 打表: 按键调节的电机角度 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* ---- 打表按键: K2 +0.1° / K3 -0.1° / K1 一键校准偏移 ---- */
static uint8_t read_k1(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET ? 1 : 0; }
static void on_k1(MulitKey_t* k) { (void)k; Table_CalibrateOffset(g_ball_pos * 0.1f, man_angle); }
static uint8_t read_k2(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET ? 1 : 0; }
static void on_k2(MulitKey_t* k) { (void)k; man_angle += 0.1f; }
static uint8_t read_k3(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET ? 1 : 0; }
static void on_k3(MulitKey_t* k) { (void)k; man_angle -= 0.1f; }
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void ZDT_Send_Tx_callback(uint8_t *pData, uint16_t Size)
{
    HAL_UART_Transmit(&huart2, pData, Size, HAL_MAX_DELAY);
}


/* -------- 云台控制 -------- */

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
  MX_SPI3_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  LOG_Init(__uart2_tx);
  LOG_Set_Level(LOG_LEVEL_INFO);
  LOG_INFO("Hello, World!\n");
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
  ZDT_Enable(&x_asix_motor);
  Task3_Init();
  Task4_Init();
  Task4_Simple_Init();
  // ZDT_SetHomeOrigin(&x_asix_motor, true);
  ZDT_TriggerHome(&x_asix_motor, ZDT_HOME_NEAREST);
  HAL_Delay(1000);
  HAL_TIM_Base_Start_IT(&htim2);
  ZDT_Pulse_Init();
  ZDT_Pulse_Enable();
  App_Menu_Start();  // 默认 RUN

  // /* PID初始化: kp=角度→RPM, ki=消除静差, kd=微分预判减速 */
   App_Protocol_Init();
   App_Menu_Init();
  MulitKey_t mk1, mk2, mk3;
  MulitKey_Init(&mk1, read_k1, on_k1, on_k1, FALL_BORDER_TRIGGER);
  MulitKey_Init(&mk2, read_k2, on_k2, on_k2, FALL_BORDER_TRIGGER);
  MulitKey_Init(&mk3, read_k3, on_k3, on_k3, FALL_BORDER_TRIGGER);
	HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte_uart1, 1);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 协议优先: OLED之前处理, 降低延迟 */
    for (size_t i = 0; i < 8; i++)
    {
      App_Protocol_Loop();
    }

    App_Menu_Process();
  
			
		
		
		if (HAL_GetTick() - last_tick >= 50)   // 20ms→50ms, 减OLED负担
    {
      last_tick = HAL_GetTick();
      char buf[32] = {0};
      App_Menu_GetModeNameAndStatus(buf, sizeof(buf));
      OLED_ShowString(32, 0, (uint8_t*)buf, 8, 1);
        switch (App_Menu_GetMode())
        {
        case MENU_ZDT_TEST:
          /* 打表模式: 上排=球位置(cm), 下排=当前角度 */
          LOG_Snprintf(buffer, sizeof(buffer), "BALL:%.1f cm", g_ball_pos * 0.1f);
          OLED_ShowString(0, 8, (uint8_t*)buffer, 8, 1);
          LOG_Snprintf(buffer, sizeof(buffer), "ANG:%.1f", man_angle);
          OLED_ShowString(0, 16, (uint8_t*)buffer, 8, 1);
          LOG_Snprintf(buffer, sizeof(buffer), "OFF:%.1f cm", g_pos_offset);
          OLED_ShowString(0, 24, (uint8_t*)buffer, 8, 1);
            break;
        case MENU_TASK3:
          LOG_Snprintf(buffer, sizeof(buffer), "P:%.2f S:%u O:%.2f",
            Task3_GetCurrent(),Task3_GetStep(), Task3_GetOutput());
          OLED_ShowString(0, 8, (uint8_t*)buffer, 8, 1);
          LOG_Snprintf(buffer, sizeof(buffer), "A:%.2f", Task3_GetAngle());
          OLED_ShowString(0, 16, (uint8_t*)buffer, 8, 1);
          break;
        case MENU_TASK4:
          LOG_Snprintf(buffer, sizeof(buffer), "P:%.2f ff:%.2f", Task4_GetCurrent(), Task4_GetOutput());
          OLED_ShowString(0, 8, (uint8_t*)buffer, 8, 1);
          // LOG_Snprintf(buffer, sizeof(buffer), "ax:%.2f ay:%.2f az:%.2f", mpu_data->phys.ax, mpu_data->phys.ay, mpu_data->phys.az);
          // OLED_ShowString(0, 16, (uint8_t*)buffer, 8, 1);
            break;
        default:
          break;
        }
      OLED_Refresh();
    }
    #if LOG_USE_QUEUE == 1
   LOG_Process();
    #endif
    // LOG_DEBUG("rx:0x%2x", rx_byte);
    // HAL_Delay(300);
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
  if (htim->Instance == TIM2)
  {
    MPU6050_Update();

    MenuMode mode = App_Menu_GetMode();
    bool     run  = App_Menu_IsRunning();

    if (run && mode == MENU_TASK3) {
      if (Task4_Simple_IsRunning()) Task4_Simple_Stop();
      if (!Task3_IsRunning() && !Task3_IsDone()) Task3_Start();
      Task3_Update(2.0f);
      Task3_Control_Send();
    } else if (run && mode == MENU_TASK4) {
      if (Task3_IsRunning() || Task3_IsDone()) Task3_Stop();
      if (!Task4_Simple_IsRunning()) Task4_Simple_Start();
      Task4_Simple_Update(2.0f);
      Task4_Simple_Control_Send();
    } else if (run && mode == MENU_ZDT_TEST) {
      if (Task3_IsRunning() || Task3_IsDone()) Task3_Stop();
      if (Task4_Simple_IsRunning()) Task4_Simple_Stop();
      ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(28.0f));
    } else {
      if (Task3_IsRunning() || Task3_IsDone()) Task3_Stop();
      if (Task4_Simple_IsRunning()) Task4_Simple_Stop();
    }
  }
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        ZDT_Pulse_PeriodElapsedCallback(htim);
    }
}
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    ZDT_Pulse_EXTI_Callback(pin);
}


// void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
// {
//   if (huart->Instance == USART6)
//   {
//     APP_Protocol_FeedBuffer(rx_buffer, Size);
// 		HAL_UARTEx_ReceiveToIdle_IT(&huart6, rx_buffer, sizeof(rx_buffer));
//   }
// }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  char buffer[32];
  if (huart->Instance == USART6)
  {
    // LOG_Snprintf(buffer, sizeof(buffer), "rcv:0x%02x\n",rx_byte);
    // HAL_UART_Transmit(&huart6, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
		// LOG_INFO("rcv:0x%02x",rx_byte);
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    App_Protocol_FeedByte(rx_byte);
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
  if (huart->Instance == USART1)
  {
    App_Protocol_FeedByte_UART1(rx_byte_uart1);
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte_uart1, 1);
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
