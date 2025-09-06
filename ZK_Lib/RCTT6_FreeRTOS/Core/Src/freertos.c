/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

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
/* USER CODE BEGIN Variables */


/* USER CODE END Variables */
/* Definitions for led1task */
osThreadId_t led1taskHandle;
const osThreadAttr_t led1task_attributes = {
  .name = "led1task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for led2task */
osThreadId_t led2taskHandle;
const osThreadAttr_t led2task_attributes = {
  .name = "led2task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for keytask */
osThreadId_t keytaskHandle;
const osThreadAttr_t keytask_attributes = {
  .name = "keytask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for flowtask */
osThreadId_t flowtaskHandle;
const osThreadAttr_t flowtask_attributes = {
  .name = "flowtask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KEYevent */
osEventFlagsId_t KEYeventHandle;
const osEventFlagsAttr_t KEYevent_attributes = {
  .name = "KEYevent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void LED1_TASK(void *argument);
void LED2_TASK(void *argument);
void KEY_TASK(void *argument);
void FLOW_TASK(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of led1task */
  led1taskHandle = osThreadNew(LED1_TASK, NULL, &led1task_attributes);

  /* creation of led2task */
  led2taskHandle = osThreadNew(LED2_TASK, NULL, &led2task_attributes);

  /* creation of keytask */
  keytaskHandle = osThreadNew(KEY_TASK, NULL, &keytask_attributes);

  /* creation of flowtask */
  flowtaskHandle = osThreadNew(FLOW_TASK, NULL, &flowtask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of KEYevent */
  KEYeventHandle = osEventFlagsNew(&KEYevent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_LED1_TASK */
/**
  * @brief  Function implementing the led1task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_LED1_TASK */
void LED1_TASK(void *argument)
{
  /* USER CODE BEGIN LED1_TASK */
  uint32_t flags = 0;
  /* Infinite loop */
  for(;;)
  {	
    flags = osEventFlagsWait(KEYeventHandle,LED0_BLINK,osFlagsNoClear,osWaitForever);
    if(flags & LED0_BLINK)
    {
      HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
      osDelay(500);
    }
  }
  /* USER CODE END LED1_TASK */
}

/* USER CODE BEGIN Header_LED2_TASK */
/**
* @brief Function implementing the led2task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED2_TASK */
void LED2_TASK(void *argument)
{
  /* USER CODE BEGIN LED2_TASK */
  uint32_t flags = 0;
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(KEYeventHandle,LED1_BLINK,osFlagsNoClear,osWaitForever);
    if(flags & LED1_BLINK)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
      osDelay(500);
    }
  }
  /* USER CODE END LED2_TASK */
}

/* USER CODE BEGIN Header_KEY_TASK */
/**
* @brief Function implementing the keytask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_KEY_TASK */
void KEY_TASK(void *argument)
{
  /* USER CODE BEGIN KEY_TASK */
  BUTTON_STATE KEY1_state = {0};
  BUTTON_STATE KEY2_state = {0};
  /* Infinite loop */
  for(;;)
  {
    KEY1_state.key_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin);
    KEY2_state.key_state = HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin);
    if(KEY1_state.key_state == true && KEY2_state.key_state == true)
    {
      KEY1_state.key_last_state = KEY1_state.key_state;
      KEY2_state.key_last_state = KEY2_state.key_state;
      // if(KEY1_state.key_state == GPIO_PIN_SET && KEY2_state.key_last_state == GPIO_PIN_SET)
      // {
        osEventFlagsClear(KEYeventHandle,LED0_BLINK|LED1_BLINK|LED_FLOW);
        osEventFlagsSet(KEYeventHandle,LED_FLOW);
        HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_SET);
      // }
    }
    else if (KEY1_state.key_state != KEY1_state.key_last_state)
    {
      KEY1_state.key_last_state = KEY1_state.key_state;
      if(KEY1_state.key_state == GPIO_PIN_SET)
      {
        osEventFlagsClear(KEYeventHandle,LED0_BLINK|LED1_BLINK|LED_FLOW);
        osEventFlagsSet(KEYeventHandle,LED0_BLINK);
        HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_SET);
      }
    }
    else if (KEY2_state.key_state != KEY2_state.key_last_state)
    {
      KEY2_state.key_last_state = KEY2_state.key_state;
      if(KEY2_state.key_state == GPIO_PIN_SET)
      {
        osEventFlagsClear(KEYeventHandle,LED0_BLINK|LED1_BLINK|LED_FLOW);
        osEventFlagsSet(KEYeventHandle,LED1_BLINK);
        HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_SET);
      }
    }
    
    osDelay(1);
  }
  /* USER CODE END KEY_TASK */
}

/* USER CODE BEGIN Header_FLOW_TASK */
/**
* @brief Function implementing the flowtask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FLOW_TASK */
void FLOW_TASK(void *argument)
{
  /* USER CODE BEGIN FLOW_TASK */
  uint32_t flag = 0;
  /* Infinite loop */
  for(;;)
  {
    flag = osEventFlagsWait(KEYeventHandle,LED_FLOW,osFlagsNoClear,osWaitForever);
    if (flag & LED_FLOW)
    {
      HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_SET);
      osDelay(500);
      HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_SET);
      osDelay(500);
      HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_RESET);
    }
  }
  /* USER CODE END FLOW_TASK */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

