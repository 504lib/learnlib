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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t state = 0;//状态机状态变量，初始状态为0
uint32_t last_tick = 0;//用于记录上一次的系统时间
uint8_t Pin_enum = 0;//用于记录当前点亮的LED灯，0表示LED0，1表示LED1
GPIO_PinState key1_last_state = GPIO_PIN_RESET;//用于记录按键一的上一次状态
GPIO_PinState key2_last_state = GPIO_PIN_RESET;//用于记录按键二的上一次状态
GPIO_PinState key1_current_state = GPIO_PIN_RESET;//用于记录按键一的当前状态
GPIO_PinState key2_current_state = GPIO_PIN_RESET;//用于记录按键二的当前状态
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* *************************** 读取按键状态 ***************************************** */
    key1_current_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin);//记录当前按键一的状态
    key2_current_state = HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin);//记录当前按键二的状态
    /* ******************************************************************************** */

    /* *************************** 状态机设计 ***************************************** */
    if(key1_current_state == GPIO_PIN_SET && key2_current_state == GPIO_PIN_SET)//若两个按键如果都按下(不是同时！！！)
    {
      //记录当前按键状态为上一次按键状态
      key1_last_state = key1_current_state;
      key2_last_state = key2_current_state;
      //让双灯灭
      //注意：只有两个引脚在同一个端口才能进行位或操作
      HAL_GPIO_WritePin(GPIOC,LED0_Pin|LED1_Pin,GPIO_PIN_SET);
      //状态切换
      state = 1;
      //初始化引脚状态
      Pin_enum = 0;
    }
    else if (key1_current_state != key1_last_state)//若按键一的当前状态与上一次状态不同
    {
      //立刻记录当前按键状态作为上一次按键状态
      key1_last_state = key1_current_state;
      if(key1_current_state == GPIO_PIN_SET)//若按键状态为按下且上次状态与之不同(即上一次为低电平)
      {
        //让双灯灭
        //注意：只有两个引脚在同一个端口才能进行位或操作
        HAL_GPIO_WritePin(GPIOC,LED0_Pin|LED1_Pin,GPIO_PIN_SET);
        //状态切换
        state = 2;
      }
    }
    else if (key2_current_state != key2_last_state)//若按键二的当前状态与上一次状态不同
    { 
      //立刻记录当前按键状态作为上一次按键状态
      key2_last_state = key2_current_state;
      if(key2_current_state == GPIO_PIN_SET)//若按键状态为按下且上次状态与之不同(即上一次为低电平)
      {
        //让双灯灭
        //注意：只有两个引脚在同一个端口才能进行位或操作
        HAL_GPIO_WritePin(GPIOC,LED0_Pin|LED1_Pin,GPIO_PIN_SET);
        //状态切换
        state = 3;
      }
    }
    /* ******************************************************************************** */

    /* ************************************ 状态机任务执行区域 ******************************************** */
    //全程使用非阻塞延时，以免影响按键检测
    if(state == 0)//初始状态，双灯灭
    {
      HAL_GPIO_WritePin(GPIOC,LED0_Pin|LED1_Pin,GPIO_PIN_SET);
    }
    else if (state == 1)//交替闪烁，周期是500ms
    {
      if(HAL_GetTick() - last_tick >= 250)
      {
        if(Pin_enum == 0)
        {
          HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
          HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_RESET);
          Pin_enum = 1;
        }
        else
        {
          HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
          HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_RESET);
          Pin_enum = 0;
        }
        last_tick = HAL_GetTick();
      }
    }
    else if (state == 2)//LED0闪烁，周期是1s
    {
      if(HAL_GetTick() - last_tick >= 500)
      {
        HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
        last_tick = HAL_GetTick();
      }
    }
    else if (state == 3)//LED1闪烁，周期是0.5s
    {
      if(HAL_GetTick() - last_tick >= 250)
      {
        HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
        last_tick = HAL_GetTick();
      }
    }
  }
  /* ******************************************************************************** */
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

#ifdef  USE_FULL_ASSERT
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
