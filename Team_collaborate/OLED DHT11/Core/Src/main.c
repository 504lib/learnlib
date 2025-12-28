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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht11.h"
#include "oled.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PIN GPIO_PIN_13
#define LED_GPIO_PORT GPIOC

SystemState g_systemState = SYS_RUNNING;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
DHT11_Data_TypeDef DHT11_Data;

// ΢����ʱ������ʹ��TIM2��
void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_Base_Start(&htim2);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
    HAL_TIM_Base_Stop(&htim2);
}

// ������ʱ����
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        delay_us(1000);
    }
}

void LED_Blink(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    delay_ms(500);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
//	HAL_Delay(50);
}

uint8_t uart_rx_data;           
uint8_t uart_rx_buffer[64];     
uint8_t uart_rx_index = 0;     
uint8_t uart_rx_flag = 0;       
uint8_t system_state = 1; 
// �ϴη���ʱ��
uint32_t last_send_time = 0; 
uint32_t last_read_time = 0;

const uint32_t SEND_INTERVAL = 1000; 

float temperature = 25.0f;      
float humidity = 50.0f;         
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
  DHT11_Init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &uart_rx_buffer[0], 1); // �������ڽ����ж�  
  OLED_Init();
  OLED_Clear();   // ����	
  OLED_ShowString(0, 0, (uint8_t *)"DHT11 Sensor", 8);
  OLED_ShowString(0, 1, (uint8_t *)"Initializing...", 8);
//  OLED_Refresh();
  HAL_Delay(1000);
  
//  printf("System Started - UART Control Example\r\n");
//  printf("Send 'ON' to start, 'OFF' to stop\r\n");
  HAL_UART_Receive_IT(&huart1, &uart_rx_data, 1); 
  last_send_time = HAL_GetTick();
  OLED_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//		OLED_ShowCHinese(48,0,17);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  HAL_GPIO_TogglePin(LED_GPIO_PORT,LED_PIN);
//	  HAL_Delay(200);
        uint32_t current_time = HAL_GetTick();
        
        if (g_systemState == SYS_RUNNING)
        {
            // ÿ2���ȡһ��DHT11
            if (current_time - last_read_time >= 100)
            {
                if (DHT11_Read(&DHT11_Data) == SUCCESS)
                {
                    // ���㸡����ֵ
                    float temp = DHT11_Data.temp_int + DHT11_Data.temp_deci * 0.1f;
                    float humidity = DHT11_Data.humi_int + DHT11_Data.humi_deci * 0.1f;
                    
                    // ʹ��oled.c�е�OLED_ShowTempHumidity������ʾ
                    OLED_ShowTempHumidity(temp, humidity);
//                    LED_Blink();
                    // ���������1000ms�����
                    if (current_time - last_send_time >= 1000)
                    {
//                        printf("temp:%.2f hum:%.2f\r\n", temp, humidity);
                        last_send_time = current_time;
                        
                        // LED��˸
                        LED_Blink();
                    }
                }
                else
                {
                    // ��ȡʧ�ܣ���ʾ������Ϣ
//                    OLED_Clear();
                    OLED_ShowString(0, 0, (uint8_t *)"DHT11 Error", 16);
//                    OLED_Refresh();
                }
                last_read_time = current_time;
            }
        }
        else if (g_systemState == SYS_STOPPED)
        {
            // ϵͳֹͣ״̬��������ʾֹͣ��Ϣ
            static uint8_t stopped_displayed = 0;
            if (!stopped_displayed)
            {
//                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"System Stopped", 16);
                OLED_ShowString(0, 2, (uint8_t *)"Send 'ON' to start", 16);
//                OLED_Refresh();
                stopped_displayed = 1;
            }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1)
  {
    if (uart_rx_data == '\r' || uart_rx_data == '\n') 
    {
        uart_rx_buffer[uart_rx_index] = '\0';
        if (strcmp((char *)uart_rx_buffer, "ON") == 0) 
        {
            system_state = 1;
            printf("System state: ON - Start reading and sending\r\n");
        }
        else if (strcmp((char *)uart_rx_buffer, "OFF") == 0) 
        {
            system_state = 0;
            printf("System state: OFF - Stop reading and sending\r\n");
        }
        else
        {
            printf("Unknown command: %s\r\n", uart_rx_buffer);
        }
        
        uart_rx_index = 0; 
    }
    else 
    {
        if (uart_rx_index < sizeof(uart_rx_buffer) - 1) 
        {
            uart_rx_buffer[uart_rx_index++] = uart_rx_data;
        }
        else
        {
            uart_rx_index = 0;
            printf("Command too long, resetting buffer\r\n");
        }
    }
    HAL_UART_Receive_IT(&huart1, &uart_rx_data, 1);
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
