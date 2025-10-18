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
#include "HX711.h"
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
system_status system_status_var = Ready;
char user_buf[32] = "\0";
float target_weight = 0;
int32_t target_weight_hardware = 0;
Medicine current_medicine;
extern uint8_t temp[64];                                                          // 鏉ヨ嚜main.c鐨勭紦鍐插尯
u8g2_t u8g2;                                                                      // u8g2瀵硅薄
int index = 0;                                                                    // String_Option鐨勫瓧绗︿覆鏁扮粍绱㈠紩
const char* String_Option[] = {"Ephedra","Cinnamon Twig"};            // 娴嬭瘯鐨勫瓧绗︿覆
bool toggle = false;                                                              // 娴嬭瘯bool鑺傜偣鐨勫彉閲?
int32_t passenger_num = 0;                                                        // 瀛樺偍涔樺?㈢殑鍙橀??
float weight = 0.0;
// 鐢ㄤ簬璺熻釜鑿滃崟淇℃伅鐨勬暟鎹?缁撴瀯浣撴寚鎸囬??
menu_data_t* menu_data_ptr;
// 鑺傜偣鎸囬拡
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

// RTC鍐呴儴缁撴瀯浣?
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};
typedef struct
{
  int32_t seconds;                  // 鏆傚瓨绉?
  int32_t minutes;                  // 鏆傚瓨鍒?
  int32_t hours;                    // 鏆傚瓨鏃?
  int32_t day;                      // 鏆傚瓨澶?
  int32_t month;                    // 鏆傚瓨鏈?
  int32_t year;                     // 鏆傚瓨骞?
}Clock_t;
Clock_t Clock = {0};                // 鏆傚瓨鏃堕棿缁撴瀯浣?
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


static int test_var = 0;                                            // INT绫诲瀷鐨勫彉閲?

/**
 * @brief    浣滀负sub1_sub1鐨勫洖璋冨嚱鏁?
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
 * @brief    浣滀负sub1_sub3鐨勫洖璋冨嚱鏁?
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
 * @brief    浣滀负sub1_sub4鐨勫洖璋冨嚱鏁?
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
 * @brief    浣滀负sub4鐨勮繘鍏ュ洖璋冨嚱鏁?,鐩?鐨勬殏瀛樿繘鍏ュ綋鍓嶄换鍔＄殑鏃堕??
 * @param    item      褰撳墠鑺傜偣淇℃伅锛岀敤鎴锋棤闇?浜嗚В
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
 * @brief    浣滀负sub4_sub7鐨勫洖璋冨嚱鏁帮紝鐩?鐨勬槸鍚慠TC澶囦唤瀵勫瓨鍣ㄥ啓鍏ュ勾鏈堟棩鍜屼慨鏀规椂鍒嗙??
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
 * @brief    浣滀负sub5_sub2鐨勫洖璋冨嚱鏁帮紝鐩?鐨勬槸鍚慹sp32鍙戦?佸綋鍓嶇殑涔樺?㈡暟閲?
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
 * @brief    浣滀负sub5_sub3鐨勫洖璋冨嚱鏁帮紝鐩?鐨勬槸鍚慹sp32鍙戦?佹竻绌哄綋鍓嶄箻瀹㈡暟閲忔寚浠?
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
 * @brief    浣滀负main_display鑺傜偣鐨勫洖璋冨嚱鏁帮紝鐩?鐨勬槸浣滀负娓叉煋棣栭〉闈?
 * @param    u8g2      u8g2鍙ユ焺,鍥炶皟鍑芥暟鍥哄畾鍙傛暟
 * @param    menu_data 鑺傜偣鏁版嵁鍙ユ焺,鍥炶皟鍑芥暟鍥哄畾鍙傛暟
 */
void main_display_cb(u8g2_t* u8g2, menu_data_t* menu_data)
{
  
  char buf[32];
  // 1. 椤堕儴澶ф椂闂存樉锟???
  if (system_status_var == Runing)
  {
 u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        
        // 顶部状态栏
        u8g2_DrawBox(u8g2, 0, 0, 128, 12);
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawStr(u8g2, 40, 13, "WEIGHING");
        u8g2_SetDrawColor(u8g2, 1);
        
        // 用户信息
        u8g2_SetFont(u8g2, u8g2_font_9x15_tf);
        snprintf(buf, sizeof(buf), "%s", user_buf);
        u8g2_DrawStr(u8g2, 5, 28, buf);
        
        // 当前重量 - 大字体
        u8g2_SetFont(u8g2, u8g2_font_logisoso20_tn);
        snprintf(buf, sizeof(buf), "%.1f g", weight);
        uint8_t weight_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - weight_width) / 2, 50, buf);
        
        // 进度条
        float progress = (weight / target_weight) * 100.0f;
        if (progress > 100.0f) progress = 100.0f;
        
        // 进度条背景
        u8g2_DrawFrame(u8g2, 10, 55, 108, 8);
        // 进度条填充
        uint8_t bar_width = (uint8_t)((progress / 100.0f) * 106);
        u8g2_DrawBox(u8g2, 11, 56, bar_width, 6);
        
        // 进度百分比
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        snprintf(buf, sizeof(buf), "%.0f%%", progress);
        u8g2_DrawStr(u8g2, 120 - u8g2_GetStrWidth(u8g2, buf), 62, buf);
  }
  else if(system_status_var == Ready)
  {

        u8g2_SetFont(u8g2, u8g2_font_logisoso26_tn);
        
        // 时间 - 大字体居中
        snprintf(buf, sizeof(buf), "%02d:%02d", sTime.Hours, sTime.Minutes);
        uint8_t time_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - time_width) / 2, 30, buf);
        
        // 秒数 - 小字体在时间右侧
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        snprintf(buf, sizeof(buf), "%02d", sTime.Seconds);
        u8g2_DrawStr(u8g2, (128 - time_width) / 2 + time_width + 5, 30, buf);
        
        // 日期 - 中等字体
        u8g2_SetFont(u8g2, u8g2_font_9x6LED_mn);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", 
                sDate.Year + 2000, sDate.Month, sDate.Date);
        uint8_t date_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, (128 - date_width) / 2, 45, buf);
        
        // 底部提示信息
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 20, 62, "WAITING FOR USER");


  }
  else if (system_status_var == Stop)
  {
        u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        
        // 顶部状态栏
        u8g2_DrawBox(u8g2, 0, 0, 128, 20);
        u8g2_SetDrawColor(u8g2, 0); // 白色文字
        snprintf(buf,sizeof(buf),"%s",user_buf);
        u8g2_DrawStr(u8g2, 5, 15, buf);
        u8g2_SetDrawColor(u8g2, 1); // 恢复黑色
        
        snprintf(buf,sizeof(buf),"medicine:%d",current_medicine);
        u8g2_DrawStr(u8g2, 5, 30, buf);

        // 重量信息 - 突出显示
        u8g2_SetFont(u8g2, u8g2_font_9x15B_tf);
        snprintf(buf, sizeof(buf), "Please take it");
        uint8_t weight_width = u8g2_GetStrWidth(u8g2, buf);
        u8g2_DrawStr(u8g2, 0, 45, buf);
        
        snprintf(buf,sizeof(buf),"away...");
        weight_width = u8g2_GetStrWidth(u8g2,buf);
        u8g2_DrawStr(u8g2, (120 - weight_width) / 2, 60, buf);
        // 底部状态指示
  }
  
  
  // 3. 鐘讹拷?锟藉崱锟???
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
  sub1_sub1 = create_param_enum_item("Change_param",&index,String_Option,2);
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
 * @brief    UP按键的按键检测函数,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状态,只有0和1两个参数
 */
uint8_t Key_UP_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port,KEY_UP_Pin);
}	

/**
 * @brief    DOWN按键的按键检测函数,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状态,只有0和1两个参数
 */
uint8_t Key_DOWN_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin);
}

/**
 * @brief    ENTER按键的按键检测函数,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状态,只有0和1两个参数
 */
uint8_t Key_ENTER_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port,KEY_ENTER_Pin);
}
 
/**
 * @brief    CANCEL按键的按键检测函数,用于按键对象回调
 * @param    key       按键对象句柄,回调函数固定参数
 * @return   uint8_t   按键状态,只有0和1两个参数
 */
uint8_t Key_CANCEL_ReadPin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(KEY_CANCEL_GPIO_Port,KEY_CANCEL_Pin);
}

/**
 * @brief    UP键触发回调
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_UP_Pressed(MulitKey_t* key)
{
  navigate_up(menu_data_ptr);
}

/**
 * @brief    DOWN键触发回调
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_DOWN_Pressed(MulitKey_t* key)
{
  navigate_down(menu_data_ptr);
}

/**
 * @brief    ENTER键触发回调
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_ENTER_Pressed(MulitKey_t* key)
{
  navigate_enter(menu_data_ptr);
}

/**
 * @brief    CANCEL键触发回调
 * @attention 注意，该函数已经被单按和长按同时调用
 * @param    key       按键对象句柄,回调函数固定参数
 */
void KEY_CANCEL_Pressed(MulitKey_t* key)
{
  navigate_back(menu_data_ptr);
}

/**
 * @brief    该函数是收到PASSENGER信息的回调函数
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

  MulitKey_t Key_UP_S;                  // UP鎸夐敭瀵硅薄
  MulitKey_t Key_DOWN_S;                // DOWN鎸夐敭瀵硅薄
  MulitKey_t Key_ENTER_S;               // ENTER鎸夐敭瀵硅薄
  MulitKey_t Key_CANCEL_S;              // CANCEL鎸夐敭瀵硅薄
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
  uint8_t data[32] = {0};                                             // 鏆傚瓨鏁版嵁甯х殑缂撳啿鍖?
  UartFrame* frame_buffer = Get_Uart_Frame_Buffer();                  // 鑾峰緱鐜?褰㈢幆褰㈢紦鍐插尯鐨勬寚閽?
  uint32_t flags;                                                     // 浜嬩欢缁?
  uint8_t passenger_temp = 0;                                         // 鏆傚瓨passenger鐨勬暟閲忕殑鍙橀噺锛屼竴鏃︿笌瀹為檯passenger涓嶅悓锛岀珛鍒诲悜esp32鍙戦?佷俊鎭?
  LOG_INFO("UART_RX task has been init ...");
  set_PASSENGER_Callback(synchronized_passengers);
  /* Infinite loop */
  for(;;)
  {
    flags = osEventFlagsWait(UART_EVENTHandle,UART_RECEIVE_EVENT,osFlagsWaitAny,100);
    if(flags & UART_RECEIVE_EVENT)                                    // 浜嬩欢缁勬敹鍒伴?氱煡锛岀珛鍒诲?勭悊鏁版嵁甯?
    {
      LOG_INFO("data frame is has came ... , Size = %d",frame_buffer->Size);
      memset(data,0,sizeof(data));                                    // 闃叉?㈡湁娈嬬暀鏁版嵁甯э紝娓呯┖缂撳啿鍖?
      uint16_t size = frame_buffer->Size;                             // 鑾峰緱璇ュ抚闀垮害
      Uart_Buffer_Get_frame(frame_buffer,data);                       // 浠庣幆褰㈢紦鍐插尯鎻愬彇鍑哄抚鏁版嵁锛屾柟渚垮?勭??
      Receive_Uart_Frame(UART_protocol_structure,data,size);          // 澶勭悊鏁版嵁
    }
    else
    {
      if(passenger_temp != passenger_num)                             // 涓?鏃︿笌瀹為檯passenger涓嶅悓锛岀珛鍒诲悜esp32鍙戦?佷俊鎭?
      {
        UART_Protocol_Passenger(UART_protocol_structure,passenger_num);
        passenger_temp = passenger_num;                               // 淇濇寔鍚屾??
      }
      // 瓒呮椂锛屾?�鏌?ORE鏍囧織
      if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE))                // ORE寮傚父锛岄渶瑕侀┈涓婃仮澶嶇紦鍐插尯,鍚﹀垯DMA浼氱槴鐥?
      {
          LOG_WARN("ORE detected in task, recovering...");
          __HAL_UART_CLEAR_OREFLAG(&huart1);
          // 閲嶆柊鍚?鍔―MA鎺ユ敹
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
    // if (osKernelGetTickCount() - last_tick >= 5000)
    // {
    //   UART_Protocol_CURRENT_USER(UART_protocol_structure,"test",Medicine1,3.3,sizeof("test"));
    //   last_tick = osKernelGetTickCount();
    // }
    

    
    osDelay(100);
  }
  /* USER CODE END HX711_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

