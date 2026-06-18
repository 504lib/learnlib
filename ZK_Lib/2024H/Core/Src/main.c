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
#include "Protothreads.h"
#include "Log.h"
#include "protocol.h"
#include "static_queue.h"
#include "multikey.h"
#include "oled.h"
#include "mpu6050_user.h"
#include "MadgwickAHRS.h"
#include "mpu6050.h"
#include "tasks.h"
#include "Motor_AT4950.h"
#include "HSM_OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

int16_t test_num = 0; // 作为测试的全局变量，观察协议发送和接收的正确性
uint8_t protocol_value_dirty = 0;

bool Uart_Protocol_Tranmsit_ForHal(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart3, (uint8_t*)data, len, 100) == HAL_OK;
}


void frame_received_handler_Fucntion(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len)
{
    LOG_INFO("Received frame of type 0x%02X with length %u. Data:", frame_type, frame_len);
    for (uint16_t i = 0; i < frame_len; i++)
    {
        LOG_INFO("  Byte %u: 0x%02X", i, frame_data[i]);
    }
    if (frame_type == 0x10 && frame_len >= 2)
    {
      test_num = rd_i16_be(frame_data);
      protocol_value_dirty = 1;
      LOG_INFO("Decoded int16 payload: %d", test_num);
    }
    
}



void Uart_protocol_timeout_handler(uint8_t current_frame_type)
{
  LOG_WARN("Timeout occurred while waiting for ACK of frame type 0x%02X. Handling timeout.", current_frame_type);
}

static void RunAckWaitTest(UART_protocol_t* protocol_instance)
{
  static uint32_t last_test_tick = 0;
  static uint8_t test_counter = 0;
  uint8_t payload;

  if ((HAL_GetTick() - last_test_tick) < 2000U)
  {
    return;
  }

  last_test_tick = HAL_GetTick();
  payload = test_counter++;

  if (Uart_Protocol_Transmit_Frame(protocol_instance, &payload, 0x21, 1))
  {
    LOG_INFO("ACK test frame sent. type=0x21 payload=0x%02X", payload);
  }
  else
  {
    LOG_WARN("Failed to send ACK test frame.");
  }
}

static void RenderProtocolValue(void)
{
  char buffer[24];

  if (protocol_value_dirty == 0U)
  {
    return;
  }

  snprintf(buffer, sizeof(buffer), "RX16:%6d    ", test_num);
  OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
  OLED_Refresh();
  protocol_value_dirty = 0;
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
Protothread_t IMU_Task;
Protothread_t SerialTask_handle;
MulitKey_t key1;
MulitKey_t key2;
MotorAT4950 motor1;
UART_protocol_t uart_protocol_instance;
uint8_t receive_buffer[UART_PROTOCOL_FRAME_BUFFER_LEN];

Uart_Protocol_FunctionsParameters RequiredParam = {
  .Head_Tial_Frame_struct = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x55,
    .Tailframe2 = 0xAA
  },
  .transmit_function = Uart_Protocol_Tranmsit_ForHal,
  .frame_received_handler = frame_received_handler_Fucntion,
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void SetMotor1Level(uint8_t level) 
{
    HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, (GPIO_PinState)level);
}


static void SetMotor1ComparePWM(uint16_t ccr_value) 
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value);
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
  MX_USART1_UART_Init();
  MX_SPI3_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear(); 
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  OLED_DisPlay_On();
  PT_INIT(&IMU_Task);
  HSM_OLED_Init();
  MulitKey_Init(&key1,ReadKey1Pin,Key1PressedCallback,Key1PressedCallback,RISE_BORDER_TRIGGER);
  MulitKey_Init(&key2,ReadKey2Pin,Key2PressedCallback,Key2PressedCallback,RISE_BORDER_TRIGGER);

  Uart_Protocol_Init(&uart_protocol_instance, RequiredParam);
  {
    Uart_Protocol_ACKFunctionsParameters ack_param = {
      .GetTick = HAL_GetTick,
      .timeout_handler = Uart_protocol_timeout_handler
    };
    Uart_Protocol_Register_ACK(&uart_protocol_instance, ack_param);
  }
  Uart_Protocol_Register_Parse_WatchDog(&uart_protocol_instance, HAL_GetTick, 400); // 200ms 看门狗超时
  MotorInit_AT46950(&motor1, SetMotor1ComparePWM, SetMotor1Level, 1000);
  SetDefaultDirection(&motor1, High_Level);
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_UARTEx_ReceiveToIdle_IT(&huart3,receive_buffer, sizeof(receive_buffer));
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  Motor_setSpeed(&motor1, 500); // 设置电机以50%的占空比正向旋转
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    IMU_task(&IMU_Task);
    MulitKey_Scan(&key1);
    MulitKey_Scan(&key2);
    HSM_OLED_Process();
    SerialTask(&SerialTask_handle);
    Uart_Protocol_Loop(&uart_protocol_instance);
    RenderProtocolValue();
    // RunAckWaitTest(&uart_protocol_instance);
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
  static uint32_t counter = 0;  
  if (htim->Instance == TIM1)
  {
    if (++counter % 500 == 0)
    {
      // printf("Counter: %lu\n", counter);
    }
    
  }
  
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3)
  {
    Uart_Protocol_ProcessReceivedDataBuffer(&uart_protocol_instance, receive_buffer, Size);
    // LOG_DEBUG("Received %u bytes on USART3. Data:%*s", Size, Size,receive_buffer);
    HAL_UARTEx_ReceiveToIdle_IT(&huart3, receive_buffer, sizeof(receive_buffer));
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
