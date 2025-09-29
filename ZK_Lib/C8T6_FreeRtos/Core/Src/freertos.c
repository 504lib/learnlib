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
#include "protocol.h"
#include "Log.h"
#include "rtc.h"
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
extern uint8_t temp[16];
u8g2_t u8g2;
int index = 0;
const char* String_Option[] = {"begin","test1","test2","test3","end"};
bool toggle = false;

menu_item_t* root = NULL;
menu_item_t* sub1 = NULL;
menu_item_t* sub2 = NULL;
menu_item_t* sub3 = NULL;
menu_item_t* sub4 = NULL;
menu_item_t* sub5 = NULL;
menu_item_t* sub1_sub1 = NULL;
menu_item_t* sub1_sub2 = NULL;
menu_item_t* sub1_sub3 = NULL;
menu_item_t* sub1_sub4 = NULL;
menu_item_t* sub2_sub1 = NULL;
menu_item_t* sub2_sub2 = NULL;
menu_item_t* main_display = NULL;

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
/* Definitions for UART_TASK */
osThreadId_t UART_TASKHandle;
const osThreadAttr_t UART_TASK_attributes = {
  .name = "UART_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UART_TXMute */
osMutexId_t UART_TXMuteHandle;
const osMutexAttr_t UART_TXMute_attributes = {
  .name = "UART_TXMute"
};
/* Definitions for KEY_EVENT */
osEventFlagsId_t KEY_EVENTHandle;
const osEventFlagsAttr_t KEY_EVENT_attributes = {
  .name = "KEY_EVENT"
};
/* Definitions for UART_EVENT */
osEventFlagsId_t UART_EVENTHandle;
const osEventFlagsAttr_t UART_EVENT_attributes = {
  .name = "UART_EVENT"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void U8g2_Task(void *argument);
void LED_Task(void *argument);
void uart_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of UART_TXMute */
  UART_TXMuteHandle = osMutexNew(&UART_TXMute_attributes);

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

  /* creation of UART_TASK */
  UART_TASKHandle = osThreadNew(uart_task, NULL, &UART_TASK_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of KEY_EVENT */
  KEY_EVENTHandle = osEventFlagsNew(&KEY_EVENT_attributes);

  /* creation of UART_EVENT */
  UART_EVENTHandle = osEventFlagsNew(&UART_EVENT_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_U8g2_Task */


static int test_var = 0;
// 诊断函数
// 完全避免使用任何格式化函�???
void test()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  // UART_Protocol_FLOAT(UART_protocol_structure,3.3);
  UART_Protocol_INT(UART_protocol_structure,test_var);
}

void test2()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  UART_Protocol_FLOAT(UART_protocol_structure,3.3);
}

void test3()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  UART_Protocol_ACK(UART_protocol_structure);
}
void main_display_cb(u8g2_t* u8g2, menu_data_t* menu_data)
{
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  uint8_t seconds = sTime.Seconds;
  uint8_t minutes = sTime.Minutes;
  uint8_t hours = sTime.Hours;
  u8g2_SetFont(u8g2,u8g2_font_12x6LED_mn);
  char buf[20];
  snprintf(buf, sizeof(buf), "%0.2d:%0.2d:%0.2d", hours, minutes, seconds);
  u8g2_DrawStr(u8g2, 64, 20, buf);
  u8g2_DrawLine(u8g2,60,0,60,64);
  u8g2_SetFont(u8g2,u8g2_font_6x10_tf);
  u8g2_DrawStr(u8g2, 70, 40, "WIFI");
  snprintf(buf,sizeof(buf),"int:%d",test_var);
  u8g2_DrawStr(u8g2,0,10,buf);

  snprintf(buf,sizeof(buf),"mode:%s",String_Option[index]);
  u8g2_DrawStr(u8g2,0,20,buf);

  snprintf(buf,sizeof(buf),"bool:%s",toggle ? "true" : "false");
  u8g2_DrawStr(u8g2,0,30,buf);
} 
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
  LOG_INFO("u8g2 init has been finished...");
  u8g2_FirstPage(&u8g2);
  root = create_submenu_item("main_menu");
  sub1 = create_submenu_item("param_int");
  sub2 = create_submenu_item("param_enum");
  sub3 = create_toggle_item("sub_menu_3",&toggle);
  sub4 = create_submenu_item("sub_menu_4");
  sub5 = create_submenu_item("sub_menu_5");
  sub1_sub1 = create_function_item("SendUART_INT", test);
  sub1_sub2 = create_param_int_item("Change_int", &test_var, 0, 100, 1);
  sub1_sub3 = create_function_item("SendUART_FLOAT", test2);
  sub1_sub4 = create_function_item("SendUART_ACK", test3);
  sub2_sub1 = create_param_enum_item("Change_param",&index,String_Option,5);
  main_display = create_main_item("main",root, main_display_cb);
  Link_Parent_Child(root, sub1);
  Link_next_sibling(sub1, sub2);
  Link_next_sibling(sub2, sub3);
  Link_next_sibling(sub3, sub4);
  Link_next_sibling(sub4, sub5);
  Link_Parent_Child(sub1, sub1_sub1);
  Link_next_sibling(sub1_sub1, sub1_sub2);
  Link_next_sibling(sub1_sub2, sub1_sub3);
  Link_next_sibling(sub1_sub3, sub1_sub4);
  Link_Parent_Child(sub2, sub2_sub1);
  menu_data_ptr = menu_data_init(main_display);
  LOG_INFO("menu Nodes is has been inited ...");
  LOG_INFO("u8g2 task has been init...");
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
  uint8_t Blink_Flag = 0;
  uint32_t tick = 0;
  LOG_INFO("LED task has been init ...");
  // LOG_INFO("LED task started.");
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(KEY_EVENTHandle,KEY_DOWN_EVENT|KEY_UP_EVENT|KEY_ENTER_EVENT|KEY_CANCEL_EVENT|KEY_FUNCTION_EVENT,osFlagsWaitAny,osWaitForever);
    if(flags & KEY_UP_EVENT)
    {
      if(osKernelGetTickCount() - tick < 200) continue;
      LOG_DEBUG("KEY_UP arise ...");
      navigate_up(menu_data_ptr);
      tick = osKernelGetTickCount();
      // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_DOWN_EVENT)
    {

      if(osKernelGetTickCount() - tick < 200) continue;
      LOG_DEBUG("KEY_DOWN arise ...");
      navigate_down(menu_data_ptr);
      tick = osKernelGetTickCount();
      // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_ENTER_EVENT)
    {
      if(osKernelGetTickCount() - tick < 200) continue;
      LOG_DEBUG("KEY_ENTER arise ...");
      navigate_enter(menu_data_ptr);
      tick = osKernelGetTickCount();
      // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_CANCEL_EVENT)
    {
      if(osKernelGetTickCount() - tick < 200) continue;
      LOG_DEBUG("KEY_CANCEL arise ...");
      navigate_back(menu_data_ptr);
      tick = osKernelGetTickCount();
      // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    if(flags & KEY_FUNCTION_EVENT)
    {
      if(osKernelGetTickCount() - tick < 200) continue;
      // LOG_INFO("KEY_UP arise ...");
      // Handle function event
      Blink_Flag = !Blink_Flag;
      if(Blink_Flag)
      {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      }
      else
      {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
      }
      tick = osKernelGetTickCount();
    }
  }
  /* USER CODE END LED_Task */
}

/* USER CODE BEGIN Header_uart_task */
/**
* @brief Function implementing the UART_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_uart_task */
void uart_task(void *argument)
{
  /* USER CODE BEGIN uart_task */
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  }; 
  uint8_t data[32] = {0};
  UartFrame* frame_buffer = Get_Uart_Frame_Buffer();
  uint32_t flags;
  LOG_INFO("UART_RX task has been init ...");
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(UART_EVENTHandle,UART_RECEIVE_EVENT,osFlagsWaitAny,100);
    if(flags & UART_RECEIVE_EVENT)
    {
      LOG_INFO("data frame is has came ... , Size = %d",frame_buffer->Size);
      memset(data,0,sizeof(data));
      uint16_t size = frame_buffer->Size;
      Uart_Buffer_Get_frame(frame_buffer,data);
//      HAL_UART_Transmit_DMA(&huart1,data,size);
      // HAL_UART_Transmit(&huart1,data,size,HAL_MAX_DELAY);
      Receive_Uart_Frame(UART_protocol_structure,data,size);
    }
    else
    {
      // 超时，检查ORE标志
      if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE)) 
      {
          LOG_WARN("ORE detected in task, recovering...");
          __HAL_UART_CLEAR_OREFLAG(&huart1);
          // 重新启动DMA接收
          HAL_UARTEx_ReceiveToIdle_DMA(&huart1, temp, sizeof(temp));
          // __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
      }
    }
  }
  /* USER CODE END uart_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

