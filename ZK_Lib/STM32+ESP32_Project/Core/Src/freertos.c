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
#define MENU_USE_CMSIS_OS2
#include "../../shared/multikey/multikey.h"
#include "HX711.h"
#include "Motor.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  Runing,
  Stop,
  Ready
}system_status;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
Motor_Init_Struct motor1;
Motor_Init_Struct motor2;
bool Motor1_Ephedra = false;
bool Motor2_Cinnamon = false;
system_status system_status_var = Ready;
char user_buf[32] = "\0";
float target_weight = 0;
int32_t target_weight_hardware = 0;
Medicine current_medicine;
extern uint8_t temp[64];                                                          // 来自main.c的缓冲区
u8g2_t u8g2;                                                                      // u8g2对象
int index = 0;                                                                    // String_Option的字符串数组索引
const char* String_Option[] = {"Ephedra","Cinnamon"};            // 测试的字符串
bool toggle = false;                                                              // 测试bool节点的变�?
int32_t passenger_num = 0;                                                        // 存储乘�?�的变�??
float weight = 0.0;
// 用于跟踪菜单信息的数�?结构体指指�??
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
menu_item_t* sub1_sub4 = NULL;
menu_item_t* sub5_sub2 = NULL;
menu_item_t* sub5_sub3 = NULL;
menu_item_t* sub5_sub4 = NULL;

// RTC内部结构�?
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};
typedef struct
{
  int32_t seconds;                  // 暂存�?
  int32_t minutes;                  // 暂存�?
  int32_t hours;                    // 暂存�?
  int32_t day;                      // 暂存�?
  int32_t month;                    // 暂存�?
  int32_t year;                     // 暂存�?
}Clock_t;
Clock_t Clock = {0};                // 暂存时间结构�?
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
/* Definitions for HX711_TASK */
osThreadId_t HX711_TASKHandle;
const osThreadAttr_t HX711_TASK_attributes = {
  .name = "HX711_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
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
void HX711_Task(void *argument);

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

  /* creation of KEY_TASK */
  KEY_TASKHandle = osThreadNew(KEY_Task, NULL, &KEY_TASK_attributes);

  /* creation of UART_TASK */
  UART_TASKHandle = osThreadNew(uart_task, NULL, &UART_TASK_attributes);

  /* creation of HX711_TASK */
  HX711_TASKHandle = osThreadNew(HX711_Task, NULL, &HX711_TASK_attributes);

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


static int test_var = 0;                                            // INT类型的变�?

/**
 * @brief    作为sub1_sub1的回调函�?
 */
void test()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  UART_Protocol_INT(UART_protocol_structure,test_var);
}


/**
 * @brief    作为sub1_sub3的回调函�?
 */
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

/**
 * @brief    作为sub1_sub4的回调函�?
 */
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


/**
 * @brief    作为sub4的进入回调函�?,�?的暂存进入当前任务的时�??
 * @param    item      当前节点信息，用户无�?了解
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
 * @brief    作为sub4_sub7的回调函数，�?的是向RTC备份寄存器写入年月日和修改时分�??
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
 * @brief    作为sub5_sub2的回调函数，�?的是向esp32发�?�当前的乘�?�数�?
 */
void Send_Passenger()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  UART_Protocol_Passenger(UART_protocol_structure,++passenger_num);
}


/**
 * @brief    作为sub5_sub3的回调函数，�?的是向esp32发�?�清空当前乘客数量指�?
 */
void Send_Clear()
{
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  };
  passenger_num = 0;
  UART_Protocol_Clear(UART_protocol_structure);
}

/**
 * @brief    作为main_display节点的回调函数，�?的是作为渲染首页�?
 * @param    u8g2      u8g2句柄,回调函数固定参数
 * @param    menu_data 节点数据句柄,回调函数固定参数
 */
void main_display_cb(u8g2_t* u8g2, menu_data_t* menu_data)
{
  
  char buf[32];
  // 1. 顶部大时间显�???
  if (system_status_var == Runing)
  {
 u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        
        // ����״̬��
        u8g2_DrawBox(u8g2, 0, 0, 128, 12);
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawStr(u8g2, 40, 13, "WEIGHING");
        u8g2_SetDrawColor(u8g2, 1);
        
        // �û���Ϣ
        u8g2_SetFont(u8g2, u8g2_font_9x15_tf);
        snprintf(buf, sizeof(buf), "%s", user_buf);
        u8g2_DrawStr(u8g2, 5, 28, buf);
        
        // ��ǰ���� - ������
        u8g2_SetFont(u8g2, u8g2_font_logisoso20_tn);
        snprintf(buf, sizeof(buf), "%.1f g", weight);
        uint8_t weight_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - weight_width) / 2, 50, buf);
        
        // ������
        float progress = (weight / target_weight) * 100.0f;
        if (progress > 100.0f) progress = 100.0f;
        
        // ����������
        u8g2_DrawFrame(u8g2, 10, 55, 108, 8);
        // ���������
        uint8_t bar_width = (uint8_t)((progress / 100.0f) * 106);
        u8g2_DrawBox(u8g2, 11, 56, bar_width, 6);
        
        // ���Ȱٷֱ�
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        snprintf(buf, sizeof(buf), "%.0f%%", progress);
        u8g2_DrawStr(u8g2, 120 - u8g2_GetStrWidth(u8g2, buf), 62, buf);
  }
  else if(system_status_var == Ready)
  {

        u8g2_SetFont(u8g2, u8g2_font_logisoso26_tn);
        
        // ʱ�� - ���������
        snprintf(buf, sizeof(buf), "%02d:%02d", sTime.Hours, sTime.Minutes);
        uint8_t time_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - time_width) / 2, 30, buf);
        
        // ���� - С������ʱ���Ҳ�
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        snprintf(buf, sizeof(buf), "%02d", sTime.Seconds);
        u8g2_DrawStr(u8g2, (128 - time_width) / 2 + time_width + 5, 30, buf);
        
        // ���� - �е�����
        u8g2_SetFont(u8g2, u8g2_font_9x6LED_mn);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", 
                sDate.Year + 2000, sDate.Month, sDate.Date);
        uint8_t date_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - date_width) / 2, 45, buf);
        
        // �ײ���ʾ��Ϣ
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 20, 62, "WAITING FOR USER");


  }
  else if (system_status_var == Stop)
  {
        u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        
        // ����״̬��
        u8g2_DrawBox(u8g2, 0, 0, 128, 20);
        u8g2_SetDrawColor(u8g2, 0); // ��ɫ����
        snprintf(buf,sizeof(buf),"%s",user_buf);
        u8g2_DrawStr(u8g2, 5, 15, buf);
        u8g2_SetDrawColor(u8g2, 1); // �ָ���ɫ
        
        snprintf(buf,sizeof(buf),"%s",String_Option[(uint8_t)current_medicine]);
        u8g2_DrawStr(u8g2, 5, 30, buf);

        // ������Ϣ - ͻ����ʾ
        u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        snprintf(buf, sizeof(buf), "Please take it");
        uint8_t weight_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, 0, 45, buf);
        
        snprintf(buf,sizeof(buf),"away...");
        weight_width = u8g2_GetStrWidth(u8g2,buf);
        u8g2_DrawStr(u8g2, (120 - weight_width) / 2, 60, buf);
        // �ײ�״ָ̬ʾ
  }
  
  
  // 3. 状�?�卡�???
}

void join_queue()
{
  static uint16_t count = 0;
  UART_protocol UART_protocol_structure = {
  .Headerframe1 = 0xAA,
  .Headerframe2 = 0x55,
  .Tailframe1 = 0x0D,
  .Tailframe2 = 0x0A
  };
  char buf[16] = {0};
  snprintf(buf,sizeof(buf),"HARDWARE_%d",count++);
  float Send_target_weight = target_weight_hardware;
  UART_Protocol_CURRENT_USER(UART_protocol_structure,buf,(Medicine)index,Send_target_weight,strlen(buf));
  LOG_INFO("%d",strlen(String_Option[index]));

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
  sub1 = create_submenu_item("choose_medicine",NULL,NULL);
  sub2 = create_toggle_item("toggle",&toggle);
  sub4 = create_submenu_item("Clock_Set",set_RTC_TEMP,NULL);
  sub1_sub1 = create_param_enum_item("medicine",&index,String_Option,2);
  sub1_sub2 = create_param_int_item("target weight", &target_weight_hardware, 0, 1000, 1);
  sub1_sub3 = create_function_item("join_queue", join_queue);
  sub4_sub1 = create_param_int_item("seconds", &Clock.seconds, 0, 59, 1);
  sub4_sub1 = create_param_int_item("minutes", &Clock.minutes, 0,59, 1);
  sub4_sub2 = create_param_int_item("hours", &Clock.hours, 0, 23, 1);
  sub4_sub3 = create_param_int_item("years", &Clock.year, 0, 99, 1);
  sub4_sub5 = create_param_int_item("monthes", &Clock.month, 1, 12, 1);
  sub4_sub6 = create_param_int_item("days", &Clock.day, 1, 31, 1);
  sub4_sub7 = create_function_item("Set_time",RTC_Set_Time);
  main_display = create_main_item("main",root, main_display_cb);
  Link_Parent_Child(root, sub1);
  Link_next_sibling(sub1, sub2);
  Link_next_sibling(sub2, sub4);
  Link_Parent_Child(sub1, sub1_sub1);
  Link_next_sibling(sub1_sub1, sub1_sub2);
  Link_next_sibling(sub1_sub2,sub1_sub3);
  Link_Parent_Child(sub4, sub4_sub1);
  Link_next_sibling(sub4_sub1, sub4_sub2);
  Link_next_sibling(sub4_sub2, sub4_sub3);
  Link_next_sibling(sub4_sub3, sub4_sub4);
  Link_next_sibling(sub4_sub4, sub4_sub5);
  Link_next_sibling(sub4_sub5, sub4_sub6);
  Link_next_sibling(sub4_sub6, sub4_sub7);
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
 * @brief    UP�����İ�����⺯��,���ڰ�������ص�
 * @param    key       ����������,�ص������̶�����
 * @return   uint8_t   ����״̬,ֻ��0��1��������
 */
uint8_t Key_UP_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port,KEY_UP_Pin);
}	

/**
 * @brief    DOWN�����İ�����⺯��,���ڰ�������ص�
 * @param    key       ����������,�ص������̶�����
 * @return   uint8_t   ����״̬,ֻ��0��1��������
 */
uint8_t Key_DOWN_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin);
}

/**
 * @brief    ENTER�����İ�����⺯��,���ڰ�������ص�
 * @param    key       ����������,�ص������̶�����
 * @return   uint8_t   ����״̬,ֻ��0��1��������
 */
uint8_t Key_ENTER_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port,KEY_ENTER_Pin);
}
 
/**
 * @brief    CANCEL�����İ�����⺯��,���ڰ�������ص�
 * @param    key       ����������,�ص������̶�����
 * @return   uint8_t   ����״̬,ֻ��0��1��������
 */
uint8_t Key_CANCEL_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_CANCEL_GPIO_Port,KEY_CANCEL_Pin);
}

/**
 * @brief    UP�������ص�
 * @attention ע�⣬�ú����Ѿ��������ͳ���ͬʱ����
 * @param    key       ����������,�ص������̶�����
 */
void KEY_UP_Pressed(MulitKey_t* key)
{
  navigate_up(menu_data_ptr);
}

/**
 * @brief    DOWN�������ص�
 * @attention ע�⣬�ú����Ѿ��������ͳ���ͬʱ����
 * @param    key       ����������,�ص������̶�����
 */
void KEY_DOWN_Pressed(MulitKey_t* key)
{
  navigate_down(menu_data_ptr);
}

/**
 * @brief    ENTER�������ص�
 * @attention ע�⣬�ú����Ѿ��������ͳ���ͬʱ����
 * @param    key       ����������,�ص������̶�����
 */
void KEY_ENTER_Pressed(MulitKey_t* key)
{
  navigate_enter(menu_data_ptr);
}

/**
 * @brief    CANCEL�������ص�
 * @attention ע�⣬�ú����Ѿ��������ͳ���ͬʱ����
 * @param    key       ����������,�ص������̶�����
 */
void KEY_CANCEL_Pressed(MulitKey_t* key)
{
  navigate_back(menu_data_ptr);
}

/**
 * @brief    �ú������յ�PASSENGER��Ϣ�Ļص�����
 * @param    value     function of param
 */
void synchronized_passengers(uint8_t value)
{
  passenger_num = value;
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
  uint8_t data[32] = {0};                                             // 暂存数据帧的缓冲�?
  UartFrame* frame_buffer = Get_Uart_Frame_Buffer();                  // 获得�?形环形缓冲区的指�?
  uint32_t flags;                                                     // 事件�?
  uint8_t passenger_temp = 0;                                         // 暂存passenger的数量的变量，一旦与实际passenger不同，立刻向esp32发�?�信�?
  LOG_INFO("UART_RX task has been init ...");
  set_PASSENGER_Callback(synchronized_passengers);
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(UART_EVENTHandle,UART_RECEIVE_EVENT,osFlagsWaitAny,100);
    if(flags & UART_RECEIVE_EVENT)                                    // 事件组收到�?�知，立刻�?�理数据�?
    {
      LOG_INFO("data frame is has came ... , Size = %d",frame_buffer->Size);
      memset(data,0,sizeof(data));                                    // 防�?�有残留数据帧，清空缓冲�?
      uint16_t size = frame_buffer->Size;                             // 获得该帧长度
      Uart_Buffer_Get_frame(frame_buffer,data);                       // 从环形缓冲区提取出帧数据，方便�?��??
      Receive_Uart_Frame(UART_protocol_structure,data,size);          // 处理数据
    }
    else
    {
      if(passenger_temp != passenger_num)                             // �?旦与实际passenger不同，立刻向esp32发�?�信�?
      {
        UART_Protocol_Passenger(UART_protocol_structure,passenger_num);
        passenger_temp = passenger_num;                               // 保持同�??
      }
      // 超时，�??�?ORE标志
      if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE))                // ORE异常，需要马上恢复缓冲区,否则DMA会瘫�?
      {
          LOG_WARN("ORE detected in task, recovering...");
          __HAL_UART_CLEAR_OREFLAG(&huart1);
          // 重新�?动DMA接收
          HAL_UARTEx_ReceiveToIdle_DMA(&huart1, temp, sizeof(temp));
      }
    }
  }
  /* USER CODE END uart_task */
}

/* USER CODE BEGIN Header_HX711_Task */
void current_user_cb(char* name,Medicine medicine,float weight,uint8_t size)
{
  strcpy(user_buf,name);
  current_medicine = medicine;
  target_weight = weight;
  printf("current user:%s,target_weight = %.2f,medicine type:%d\n",user_buf,target_weight,medicine);
}


void Motor1_IN1_Write(bool state)
{
  HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,(GPIO_PinState)state);
}

void Motor1_IN2_Write(bool state)
{
  HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,(GPIO_PinState)state);
}

void Motor1_PWM_Start(bool state)
{
  if (state)
  {
    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
  }
  else
  {
    HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_1);
  }
}

void Motor_SetDuty(uint32_t duty)
{
  __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,duty);
}

/**
* @brief Function implementing the HX711_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_HX711_Task */
void HX711_Task(void *argument)
{
  /* USER CODE BEGIN HX711_Task */
  HX711_Tare();
  UART_protocol UART_protocol_structure = {
    .Headerframe1 = 0xAA,
    .Headerframe2 = 0x55,
    .Tailframe1 = 0x0D,
    .Tailframe2 = 0x0A
  }; 
  // Motor_Init(TT_Motor,
  //   &motor1,
  //   Motor1_IN1_Write,
  //   NULL,
  //   Motor1_PWM_Start,
  //   Motor_SetDuty
  // ,100); 
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
 set_Current_User(current_user_cb); 
 uint32_t last_tick = 0;
  /* Infinite loop */
  for(;;)
  {
    float weight_temp = HX711_GetFilteredWeight();
    if ((weight - weight_temp >= 0.05) || (weight_temp - weight) >= 0.05)
    {
      UART_Protocol_WEIGHT(UART_protocol_structure,weight_temp);
    }
    weight = weight_temp;
    if (system_status_var == Stop && weight < 0.2)
    {
      system_status_var = Ready;
      UART_Protocol_MOTOR_READY(UART_protocol_structure);
    }
    else if (system_status_var == Ready && target_weight != 0.0f)
    {
      system_status_var = Runing;
    }
    else if(target_weight <= weight && target_weight != 0.0f && system_status_var == Runing)
    {
      system_status_var = Stop;
      target_weight = 0.0f;
      if (osKernelGetTickCount() - last_tick >= 5000)
      {
        UART_Protocol_MOTOR_STOP(UART_protocol_structure);
        last_tick = osKernelGetTickCount();
      }
      
    }
    if (system_status_var == Runing)
    {
      if (current_medicine == Medicine1)
      {
        Motor1_Ephedra = true;
      }
      else if (current_medicine == Medicine2)
      {
        Motor2_Cinnamon = true;
      }
    }
    else if (system_status_var == Ready || system_status_var == Stop)
    {
      Motor1_Ephedra = false;
      Motor2_Cinnamon = false;
    }
    if (Motor1_Ephedra)
    {
      // Motor_Excute(&motor1,Motor_Forward,30);
      HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,GPIO_PIN_SET);
      __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,80);
    }
    else
    {

     HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,GPIO_PIN_RESET);
      __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,0);

    }
	 if (Motor2_Cinnamon)
    {
      // Motor_Excute(&motor1,Motor_Forward,30);
      HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,GPIO_PIN_SET);
      __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,80);
    }
    else
    {

     HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,GPIO_PIN_RESET);
      __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,0);

    }

    

    
    osDelay(100);
  }
  /* USER CODE END HX711_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

