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
#include "multikey.h"
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
typedef struct{
  CmdType type;
  union 
  {
    int32_t int_value;
    float float_value;
  }value;
  
}Ack_Queue_t;
extern uint8_t temp[16];                                                          // 来自main.c的缓冲区
u8g2_t u8g2;                                                                      // u8g2对象
int index = 0;                                                                    // String_Option的字符串数组索引
const char* String_Option[] = {"begin","test1","test2","test3","end"};            // 测试的字符串
bool toggle = false;                                                              // 测试bool节点的变�??
int32_t passenger_num = 0;                                                        // 存储乘客的变�??

// 用于跟踪菜单信息的数据结构体指指�??
menu_data_t* menu_data_ptr;
// 节点指针
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
menu_item_t* sub4_sub1 = NULL;
menu_item_t* sub4_sub2 = NULL;
menu_item_t* sub4_sub3 = NULL;
menu_item_t* sub4_sub4 = NULL;
menu_item_t* sub4_sub5 = NULL;
menu_item_t* sub4_sub6 = NULL;
menu_item_t* sub4_sub7 = NULL;
menu_item_t* sub5_sub1 = NULL;
menu_item_t* sub5_sub2 = NULL;
menu_item_t* sub5_sub3 = NULL;
menu_item_t* sub5_sub4 = NULL;

// RTC内部结构�??
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};
typedef struct
{
  int32_t seconds;                  // 暂存�??
  int32_t minutes;                  // 暂存�??
  int32_t hours;                    // 暂存�??
  int32_t day;                      // 暂存�??
  int32_t month;                    // 暂存�??
  int32_t year;                     // 暂存�??
}Clock_t;
Clock_t Clock = {0};                // 暂存时间结构�??
/* USER CODE END Variables */
/* Definitions for U8G2_TASK */
osThreadId_t U8G2_TASKHandle;
const osThreadAttr_t U8G2_TASK_attributes = {
  .name = "U8G2_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for KEY_TASK */
osThreadId_t KEY_TASKHandle;
const osThreadAttr_t KEY_TASK_attributes = {
  .name = "KEY_TASK",
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
/* Definitions for ACK_Task */
osThreadId_t ACK_TaskHandle;
const osThreadAttr_t ACK_Task_attributes = {
  .name = "ACK_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for ACK_Queue */
osMessageQueueId_t ACK_QueueHandle;
const osMessageQueueAttr_t ACK_Queue_attributes = {
  .name = "ACK_Queue"
};
/* Definitions for UART_TXMute */
osMutexId_t UART_TXMuteHandle;
const osMutexAttr_t UART_TXMute_attributes = {
  .name = "UART_TXMute"
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
void KEY_Task(void *argument);
void uart_task(void *argument);
void ACK_TASK(void *argument);

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

  /* Create the queue(s) */
  /* creation of ACK_Queue */
  ACK_QueueHandle = osMessageQueueNew (10, sizeof(Ack_Queue_t), &ACK_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of U8G2_TASK */
  U8G2_TASKHandle = osThreadNew(U8g2_Task, NULL, &U8G2_TASK_attributes);

  /* creation of KEY_TASK */
  KEY_TASKHandle = osThreadNew(KEY_Task, NULL, &KEY_TASK_attributes);

  /* creation of UART_TASK */
  UART_TASKHandle = osThreadNew(uart_task, NULL, &UART_TASK_attributes);

  /* creation of ACK_Task */
  ACK_TaskHandle = osThreadNew(ACK_TASK, NULL, &ACK_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of UART_EVENT */
  UART_EVENTHandle = osEventFlagsNew(&UART_EVENT_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_U8g2_Task */


static int test_var = 0;                                            // INT类型的变�??

/**
 * @brief    作为sub1_sub1的回调函�??
 */
void test()
{
  Ack_Queue_t ack_queue_t = {
    .type = INT,
    .value.int_value = test_var
  };
  osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
  // UART_protocol UART_protocol_structure = {
  //   .Headerframe1 = 0xAA,
  //   .Headerframe2 = 0x55,
  //   .Tailframe1 = 0x0D,
  //   .Tailframe2 = 0x0A
  // };
  // UART_Protocol_INT(UART_protocol_structure,test_var);
  
}


/**
 * @brief    作为sub1_sub3的回调函�??
 */
void test2()
{
  Ack_Queue_t ack_queue_t = {
    .type = FLOAT,
    .value.float_value = 3.3
  };
  osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
  // UART_protocol UART_protocol_structure = {
  //   .Headerframe1 = 0xAA,
  //   .Headerframe2 = 0x55,
  //   .Tailframe1 = 0x0D,
  //   .Tailframe2 = 0x0A
  // };
  // UART_Protocol_FLOAT(UART_protocol_structure,3.3);
  
}

/**
 * @brief    作为sub1_sub4的回调函�??
 */
void test3()
{
  Ack_Queue_t ack_queue_t = {
    .type = ACK,
    .value.int_value = 0
  };
  osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
  // UART_protocol UART_protocol_structure = {
  //   .Headerframe1 = 0xAA,
  //   .Headerframe2 = 0x55,
  //   .Tailframe1 = 0x0D,
  //   .Tailframe2 = 0x0A
  // };
  // UART_Protocol_ACK(UART_protocol_structure);
}


/**
 * @brief    作为sub4的进入回调函�??,目的暂存进入当前任务的时�??
 * @param    item      当前节点信息，用户无�??了解
 */
void set_RTC_TEMP(menu_item_t* item)
{
  Clock.seconds = sTime.Seconds;
  Clock.minutes = sTime.Minutes;
  Clock.hours = sTime.Hours;
  Clock.day = sDate.Date;
  Clock.month = sDate.Month;
  Clock.year = sDate.Year;
}

/**
 * @brief    作为sub4_sub7的回调函数，目的是向RTC备份寄存器写入年月日和修改时分秒
 */
void RTC_Set_Time()
{
  sTime.Hours = Clock.hours;
  sTime.Minutes = Clock.minutes;
  sTime.Seconds = Clock.seconds;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.Month = Clock.month;
  sDate.Date = Clock.day;
  sDate.Year = Clock.year;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  RTC_SaveDate(&sDate);
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  LOG_DEBUG("Seted date: %04d-%02d-%02d", sDate.Year + 2000, sDate.Month, sDate.Date);
  RTC_RestoreDate(&sDate);
  LOG_DEBUG("Restored date: %04d-%02d-%02d", sDate.Year + 2000, sDate.Month, sDate.Date);
}

/**
 * @brief    作为sub5_sub2的回调函数，目的是向esp32发�?�当前的乘客数量
 */
void Send_Passenger()
{
  Ack_Queue_t ack_queue_t = {
    .type = PASSENGER_NUM,
    .value.int_value = passenger_num
  };
  osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
  // UART_protocol UART_protocol_structure = {
  //   .Headerframe1 = 0xAA,
  //   .Headerframe2 = 0x55,
  //   .Tailframe1 = 0x0D,
  //   .Tailframe2 = 0x0A
  // };
  // if (osMutexAcquire(UART_TXMuteHandle, osWaitForever) == osOK)
  // {
  //   UART_Protocol_Passenger(UART_protocol_structure,++passenger_num);
  //   osMutexRelease(UART_TXMuteHandle);
  // }
}


/**
 * @brief    作为sub5_sub3的回调函数，目的是向esp32发�?�清空当前乘客数量指�??
 */
void Send_Clear()
{
  Ack_Queue_t ack_queue_t = {
    .type = CLEAR,
    .value.int_value = 0
  };
  osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
  // UART_protocol UART_protocol_structure = {
  //   .Headerframe1 = 0xAA,
  //   .Headerframe2 = 0x55,
  //   .Tailframe1 = 0x0D,
  //   .Tailframe2 = 0x0A
  // };
  // Ack_Queue_t ack_queue_t = {
  //   .type = CLEAR,
  //   .value.int_value = 0
  // };
  // passenger_num = 0;
  // UART_Protocol_Clear(UART_protocol_structure);
  // osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,100);
}

/**
 * @brief    作为main_display节点的回调函数，目的是作为渲染首页面
 * @param    u8g2      u8g2句柄,回调函数固定参数
 * @param    menu_data 节点数据句柄,回调函数固定参数
 */
void main_display_cb(u8g2_t* u8g2, menu_data_t* menu_data)
{
  
  char buf[32];
  
  // 1. 顶部大时间显�????
  u8g2_SetFont(u8g2, u8g2_font_logisoso26_tn);
  snprintf(buf, sizeof(buf), "%02d:%02d", sTime.Hours, sTime.Minutes);
  uint8_t time_width = u8g2_GetStrWidth(u8g2, buf);
  u8g2_DrawStr(u8g2, 0, 30, buf);

  u8g2_SetFont(u8g2, u8g2_font_9x6LED_mn);
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", 
           sDate.Year + 2000, sDate.Month, sDate.Date);
  uint8_t date_width = u8g2_GetStrWidth(u8g2, buf);
  u8g2_DrawStr(u8g2, 0, 40, buf);
  // 2. 秒数小字显示
  u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
  snprintf(buf, sizeof(buf), "%02d", sTime.Seconds);
  u8g2_DrawStr(u8g2, time_width + 5, 30, buf);
  
  // 3. 状�?�卡�????
  u8g2_DrawFrame(u8g2, 5, 43, 118, 20);  // 卡片外框
  
  // 卡片内部分隔�????
  u8g2_DrawVLine(u8g2, 42, 45, 17);
  u8g2_DrawVLine(u8g2, 79, 45, 17);
  
  // 卡片内容
  u8g2_SetFont(u8g2, u8g2_font_5x7_tf);
  u8g2_DrawStr(u8g2, 10, 50, "INT");
  u8g2_DrawStr(u8g2, 47, 50, "MODE");
  u8g2_DrawStr(u8g2, 84, 50, "STAT");
  
  u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
  snprintf(buf, sizeof(buf), "%d", test_var);
  u8g2_DrawStr(u8g2, 15, 60, buf);
  
  // 模式显示缩写
  char mode_abbr[4] = {0};
  strncpy(mode_abbr, String_Option[index], 3);
  u8g2_DrawStr(u8g2, 50, 60, mode_abbr);
  
  u8g2_DrawStr(u8g2, 88, 60, toggle ? "ON" : "OFF");
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
  root = create_submenu_item("main_menu",NULL,NULL);
  sub1 = create_submenu_item("param_int",NULL,NULL);
  sub2 = create_submenu_item("param_enum",NULL,NULL);
  sub3 = create_toggle_item("toggle",&toggle);
  sub4 = create_submenu_item("Clock_Set",set_RTC_TEMP,NULL);
  sub5 = create_submenu_item("Set_passenger",NULL,NULL);
  sub1_sub1 = create_function_item("SendUART_INT", test);
  sub1_sub2 = create_param_int_item("Change_int", &test_var, 0, 100, 1);
  sub1_sub3 = create_function_item("SendUART_FLOAT", test2);
  sub1_sub4 = create_function_item("SendUART_ACK", test3);
  sub2_sub1 = create_param_enum_item("Change_param",&index,String_Option,5);
  sub4_sub1 = create_param_int_item("seconds", &Clock.seconds, 0, 59, 1);
  sub4_sub2 = create_param_int_item("minutes", &Clock.minutes, 0,59, 1);
  sub4_sub3 = create_param_int_item("hours", &Clock.hours, 0, 23, 1);
  sub4_sub4 = create_param_int_item("years", &Clock.year, 0, 99, 1);
  sub4_sub5 = create_param_int_item("monthes", &Clock.month, 1, 12, 1);
  sub4_sub6 = create_param_int_item("days", &Clock.day, 1, 31, 1);
  sub4_sub7 = create_function_item("Set_time",RTC_Set_Time);
  sub5_sub1 = create_param_int_item("Set_Passenger",&passenger_num,0,255,1);
  sub5_sub2 = create_function_item("SendUART_Passenger",Send_Passenger);
  sub5_sub3 = create_function_item("clear",Send_Clear);
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
  Link_Parent_Child(sub4, sub4_sub1);
  Link_next_sibling(sub4_sub1, sub4_sub2);
  Link_next_sibling(sub4_sub2, sub4_sub3);
  Link_next_sibling(sub4_sub3, sub4_sub4);
  Link_next_sibling(sub4_sub4, sub4_sub5);
  Link_next_sibling(sub4_sub5, sub4_sub6);
  Link_next_sibling(sub4_sub6, sub4_sub7);
  Link_Parent_Child(sub5,sub5_sub1);
  Link_next_sibling(sub5_sub1,sub5_sub2);
  Link_next_sibling(sub5_sub2,sub5_sub3);
  menu_data_ptr = menu_data_init(main_display);
  LOG_INFO("menu Nodes is has been inited ...");
  LOG_INFO("u8g2 task has been init...");
  /* Infinite loop */
  for(;;)
  {
    static uint32_t last_tick = 0;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    if (osKernelGetTickCount() - last_tick > 60000)
    {
      RTC_SaveDate(&sDate);
      LOG_INFO("Date has been saved to Backup reg...");
      last_tick = osKernelGetTickCount();
    }
    u8g2_FirstPage(&u8g2);
    update_animation(menu_data_ptr);

  //  static uint8_t progess = 0;
    do {
      show_menu(&u8g2,menu_data_ptr,3);
      // u8g2_DrawBox(&u8g2,progess,0,128,15);
    } while (u8g2_NextPage(&u8g2));
    // progess = (progess + 1) % 128;
    osDelay(1);
  }
  /* USER CODE END U8g2_Task */
}

/* USER CODE BEGIN Header_KEY_Task */
/**
 * @brief    UP按键的按键检测函�??,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状�??,只有0�??1两个参数
 */
uint8_t Key_UP_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port,KEY_UP_Pin);
}	

/**
 * @brief    DOWN按键的按键检测函�??,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状�??,只有0�??1两个参数
 */
uint8_t Key_DOWN_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin);
}

/**
 * @brief    ENTER按键的按键检测函�??,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状�??,只有0�??1两个参数
 */
uint8_t Key_ENTER_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port,KEY_ENTER_Pin);
}
 
/**
 * @brief    CANCEL按键的按键检测函�??,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状�??,只有0�??1两个参数
 */
uint8_t Key_CANCEL_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_CANCEL_GPIO_Port,KEY_CANCEL_Pin);
}

/**
 * @brief    UP键触发回�??
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_UP_Pressed(MulitKey_t* key)
{
  navigate_up(menu_data_ptr);
}

/**
 * @brief    DOWN键触发回�??
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_DOWN_Pressed(MulitKey_t* key)
{
  navigate_down(menu_data_ptr);
}

/**
 * @brief    ENTER键触发回�??
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_ENTER_Pressed(MulitKey_t* key)
{
  navigate_enter(menu_data_ptr);
}

/**
 * @brief    CANCEL键触发回�??
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_CANCEL_Pressed(MulitKey_t* key)
{
  navigate_back(menu_data_ptr);
}

/**
 * @brief    该函数是收到PASSENGER信息的回调函�??
 * @param    value     function of param
 */
void synchronized_passengers(uint8_t value)
{ 
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  }; 
  passenger_num = value;
  // if (osMutexAcquire(UART_TXMuteHandle,osWaitForever))
  // {
  //   UART_Protocol_ACK(UART_protocol_structure);
  // }
  
}

void ACK_Event_mutex()
{
  if (UART_TXMuteHandle != NULL)
  {
    osEventFlagsSet(UART_EVENTHandle, UART_RECEIVE_ACK_EVENT);
  }
  
}
/**
* @brief Function implementing the KEY_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_KEY_Task */
void KEY_Task(void *argument)
{
  /* USER CODE BEGIN KEY_Task */

  MulitKey_t Key_UP_S;                  // UP按键对象
  MulitKey_t Key_DOWN_S;                // DOWN按键对象
  MulitKey_t Key_ENTER_S;               // ENTER按键对象
  MulitKey_t Key_CANCEL_S;              // CANCEL按键对象
  MulitKey_Init(&Key_UP_S,Key_UP_ReadPin,KEY_UP_Pressed,KEY_UP_Pressed,FALL_BORDER_TRIGGER);
  MulitKey_Init(&Key_DOWN_S,Key_DOWN_ReadPin,KEY_DOWN_Pressed,KEY_DOWN_Pressed,FALL_BORDER_TRIGGER);
  MulitKey_Init(&Key_ENTER_S,Key_ENTER_ReadPin,KEY_ENTER_Pressed,KEY_ENTER_Pressed,FALL_BORDER_TRIGGER);
  MulitKey_Init(&Key_CANCEL_S,Key_CANCEL_ReadPin,KEY_CANCEL_Pressed,KEY_CANCEL_Pressed,FALL_BORDER_TRIGGER);
  /* Infinite loop */
  for(;;)
  {
    MulitKey_Scan(&Key_UP_S);
    MulitKey_Scan(&Key_DOWN_S);
    MulitKey_Scan(&Key_ENTER_S);
    MulitKey_Scan(&Key_CANCEL_S);
    osDelay(1);
  }
  /* USER CODE END KEY_Task */
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
  Ack_Queue_t ack_queue_t = {0};
  uint8_t data[32] = {0};                                             // 暂存数据帧的缓冲�??
  UartFrame* frame_buffer = Get_Uart_Frame_Buffer();                  // 获得环形环形缓冲区的指针
  uint32_t flags;                                                     // 事件�??
  uint8_t passenger_temp = 0;                                         // 暂存passenger的数量的变量，一旦与实际passenger不同，立刻向esp32发�?�信�??
  LOG_INFO("UART_RX task has been init ...");
  set_PASSENGER_Callback(synchronized_passengers);
  set_ACK_Callback(ACK_Event_mutex);
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(UART_EVENTHandle,UART_RECEIVE_EVENT,osFlagsWaitAny,100);
    if(flags & UART_RECEIVE_EVENT)                                    // 事件组收到�?�知，立刻处理数据帧
    {
      LOG_INFO("data frame is has came ... , Size = %d",frame_buffer->Size);
      memset(data,0,sizeof(data));                                    // 防止有残留数据帧，清空缓冲区
      uint16_t size = frame_buffer->Size;                             // 获得该帧长度
      Uart_Buffer_Get_frame(frame_buffer,data);                       // 从环形缓冲区提取出帧数据，方便处�??
      Receive_Uart_Frame(UART_protocol_structure,data,size);          // 处理数据
    }
    else
    {
      if(passenger_temp != passenger_num)                             // �??旦与实际passenger不同，立刻向esp32发�?�信�??
      {
        ack_queue_t.type = PASSENGER_NUM;
        ack_queue_t.value.int_value = passenger_num;
        osMessageQueuePut(ACK_QueueHandle,&ack_queue_t,0,osWaitForever);
        passenger_temp = passenger_num;                               // 保持同步
      }
      // 超时，检查ORE标志
      if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE))                // ORE异常，需要马上恢复缓冲区,否则DMA会瘫�??
      {
          LOG_WARN("ORE detected in task, recovering...");
          __HAL_UART_CLEAR_OREFLAG(&huart1);
          // 重新启动DMA接收
          HAL_UARTEx_ReceiveToIdle_DMA(&huart1, temp, sizeof(temp));
      }
    }
  }
  /* USER CODE END uart_task */
}

/* USER CODE BEGIN Header_ACK_TASK */
/**
* @brief Function implementing the ACK_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ACK_TASK */
void ACK_TASK(void *argument)
{
  /* USER CODE BEGIN ACK_TASK */
  Ack_Queue_t ack_queue_t = {0};
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  }; 
  LOG_INFO("ACK task has been init ...");
  /* Infinite loop */
  for(;;)
  {
    if (osMessageQueueGet(ACK_QueueHandle,&ack_queue_t,NULL,osWaitForever) == osOK)
    {
      const uint32_t timeout_ms = 200;
      bool ACK_required = true;
      for (size_t i = 0; i < 3 && ACK_required; i++)
      {
        if (osMutexAcquire(UART_TXMuteHandle,osWaitForever) != osOK)
        {
          LOG_WARN("Failed to acquire UART_TXMute mutex");
          continue;
        }
        switch (ack_queue_t.type)
        {
          case INT:
            UART_Protocol_INT(UART_protocol_structure,ack_queue_t.value.int_value);
            break;
          case FLOAT:
            UART_Protocol_FLOAT(UART_protocol_structure,ack_queue_t.value.float_value);
            break;
          case PASSENGER_NUM:
            UART_Protocol_Passenger(UART_protocol_structure,ack_queue_t.value.int_value);
            break;
          case CLEAR:
            UART_Protocol_Clear(UART_protocol_structure);
            break;
          default:
            LOG_WARN("Unknown command type in ACK task");
            break;
        }
        osMutexRelease(UART_TXMuteHandle); 
        uint32_t flags = osEventFlagsWait(UART_EVENTHandle,UART_RECEIVE_ACK_EVENT,osFlagsWaitAny,timeout_ms);
        if (flags & UART_RECEIVE_ACK_EVENT)
        {
          ACK_required = false; // 收到ACK，跳出重发循环
          LOG_INFO("ACK received for command type %d", ack_queue_t.type);
        }
        else
        {
          LOG_WARN("ACK timeout for command type %d, retrying... (%d/3)", ack_queue_t.type, i + 1);
          if (i >= 2)
          {
            LOG_WARN("Failed to receive ACK after 3 attempts for command type %d", ack_queue_t.type);
          }
        }
        
      }
    }
    osDelay(1);
  }
  /* USER CODE END ACK_TASK */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

