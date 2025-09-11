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
#include "u8g2.h"
#include "u8g2_user.h"
#include "menu.h"
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
u8g2_t u8g2;
uint8_t progress = 0;
uint8_t index = 0;


menu_item_t* root = NULL;
menu_item_t* sub1 = NULL;
menu_item_t* sub2 = NULL;
menu_item_t* sub3 = NULL;
menu_item_t* sub4 = NULL;
menu_item_t* sub5 = NULL;
menu_item_t* sub1_sub1 = NULL;
menu_item_t* sub1_sub2 = NULL;

menu_data_t* menu_data_ptr;
/* USER CODE END Variables */
/* Definitions for U8G2_TASK */
osThreadId_t U8G2_TASKHandle;
const osThreadAttr_t U8G2_TASK_attributes = {
  .name = "U8G2_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LED_TASK */
osThreadId_t LED_TASKHandle;
const osThreadAttr_t LED_TASK_attributes = {
  .name = "LED_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KEY_EVENT */
osEventFlagsId_t KEY_EVENTHandle;
const osEventFlagsAttr_t KEY_EVENT_attributes = {
  .name = "KEY_EVENT"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void U8g2_Task(void *argument);
void LED_Task(void *argument);

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
  /* creation of U8G2_TASK */
  U8G2_TASKHandle = osThreadNew(U8g2_Task, NULL, &U8G2_TASK_attributes);

  /* creation of LED_TASK */
  LED_TASKHandle = osThreadNew(LED_Task, NULL, &LED_TASK_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of KEY_EVENT */
  KEY_EVENTHandle = osEventFlagsNew(&KEY_EVENT_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_U8g2_Task */
/**
  * @brief  Function implementing the U8G2_TASK thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_U8g2_Task */
void U8g2_Task(void *argument)
{
  /* USER CODE BEGIN U8g2_Task */
  u8g2Init(&u8g2);
  u8g2_FirstPage(&u8g2);
  root = create_submenu_item("main_menu");
  sub1 = create_submenu_item("sub_menu_1");
  sub2 = create_submenu_item("sub_menu_2");
  sub3 = create_submenu_item("sub_menu_3");
  sub4 = create_submenu_item("sub_menu_4");
  sub5 = create_submenu_item("sub_menu_5");
  sub1_sub1 = create_submenu_item("sub_menu_1_1");
  sub1_sub2 = create_submenu_item("sub_menu_1_2");
  Link_Parent_Child(root, sub1);
  Link_next_sibling(sub1, sub2);
  Link_next_sibling(sub2, sub3);
  Link_next_sibling(sub3, sub4);
  Link_next_sibling(sub4, sub5);
  Link_Parent_Child(sub1, sub1_sub1);
  Link_next_sibling(sub1_sub1, sub1_sub2);
  menu_data_ptr = menu_data_init(root);
  // menu_data.selected_item = root->first_child;
  /* Infinite loop */
  for(;;)
  {
    u8g2_FirstPage(&u8g2);
    do {
      show_menu(&u8g2,menu_data_ptr,3);
    } while (u8g2_NextPage(&u8g2));
    osDelay(1);
  }
  /* USER CODE END U8g2_Task */
}

/* USER CODE BEGIN Header_LED_Task */
/**
* @brief Function implementing the LED_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED_Task */
void LED_Task(void *argument)
{
  /* USER CODE BEGIN LED_Task */
  uint32_t flags;
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(KEY_EVENTHandle,KEY_DOWN_EVENT|KEY_UP_EVENT|KEY_ENTER_EVENT|KEY_CANCEL_EVENT,osFlagsWaitAny,osWaitForever);
    if(flags & KEY_UP_EVENT)
    {
      navigate_up(menu_data_ptr);
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_DOWN_EVENT)
    {
      navigate_down(menu_data_ptr);
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_ENTER_EVENT)
    {
      navigate_enter(menu_data_ptr);
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_CANCEL_EVENT)
    {
      navigate_back(menu_data_ptr);
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
  }
  /* USER CODE END LED_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

