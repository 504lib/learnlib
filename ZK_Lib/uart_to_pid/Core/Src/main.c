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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include "protocol.h"
#include "Log.h"
#include "static_queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    uint8_t buffer[MAX_RINGBUFFER_SIZE]; // 缓冲区大小，比如64或128
    size_t size;
}Queue_Frame;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
DECLARE_STATIC_QUEUE(RxBuffer_QUEUE,Queue_Frame, 10);
RxBuffer_QUEUE_t rx_buffer_queue;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t aRxBuffer[MAX_RINGBUFFER_SIZE];
typedef struct
{
    float Kp;
    float Ki;
    float Kd;
}sim_PID;
sim_PID speed_pid = {0};
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
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)aRxBuffer, MAX_RINGBUFFER_SIZE);
  RxBuffer_QUEUE_INIT(&rx_buffer_queue);
  uint32_t HAL_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Queue_Frame frame_buffer;
    __disable_irq();
    bool has_frame = RxBuffer_QUEUE_POP(&rx_buffer_queue, &frame_buffer);
    __enable_irq();
    if (has_frame) // 检查队列是否有数据
    {
      LOG_INFO("data frame is has came ... , Size = %d",frame_buffer.size);
      if (sscanf((char*)frame_buffer.buffer, "SPEED_PID:%f,%f,%f", &speed_pid.Kp, &speed_pid.Ki, &speed_pid.Kd) == 3)
      {
        LOG_INFO("Parsed SPEED_PID: Kp=%.2f, Ki=%.2f, Kd=%.2f", speed_pid.Kp, speed_pid.Ki, speed_pid.Kd);
      }
      else
      {
        LOG_WARN("Failed to parse SPEED_PID from frame: %s", frame_buffer.buffer);
      }
    }
    if (HAL_GetTick() - HAL_tick > 1000)
    {
      LOG_INFO("Current PID values: Kp=%.2f, Ki=%.2f, Kd=%.2f", speed_pid.Kp, speed_pid.Ki, speed_pid.Kd);
      HAL_tick = HAL_GetTick();
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
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    StaticBufferError error = check_pool((char*)&aRxBuffer, MAX_RINGBUFFER_SIZE, Size);
    if (error != OK)
    {
        LOG_FATAL("Buffer error: %d", error);
        HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)aRxBuffer, MAX_RINGBUFFER_SIZE);
        return;
    }
    if (!RxBuffer_QUEUE_IS_INIT(&rx_buffer_queue))
    {
        LOG_FATAL("Queue not initialized");
        HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)aRxBuffer, MAX_RINGBUFFER_SIZE);
        return;
    }
    Queue_Frame new_frame;
    uint16_t copy_size = Size;
    if (copy_size >= MAX_RINGBUFFER_SIZE)
    {
      copy_size = MAX_RINGBUFFER_SIZE - 1;
    }
    memcpy(new_frame.buffer, aRxBuffer, copy_size * sizeof(uint8_t));
    new_frame.buffer[copy_size] = '\0'; // 确保字符串结束
    new_frame.size = copy_size;
    if (!RxBuffer_QUEUE_IS_FULL(&rx_buffer_queue))
    {
      RxBuffer_QUEUE_PUSH(&rx_buffer_queue, new_frame);
      LOG_DEBUG("Frame added to queue, size=%d", copy_size);
    }

    HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)aRxBuffer, MAX_RINGBUFFER_SIZE);
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
