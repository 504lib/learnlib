/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "oled.h"
#include <stdio.h>
#include <string.h>
#include "tim.h"  
#include "multikey.h" 
//{
//    HAL_ADC_Start(&hadc1);                       // 启动 ADC
//    HAL_ADC_PollForConversion(&hadc1, 10);       // 等待转换完成
//	HAL_ADC_Stop(&hadc1);
//    return HAL_ADC_GetValue(&hadc1);             // 获取 0~4095 的原始值
//}
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint32_t ADC_Read_Channel(uint32_t channel);
uint32_t ADC_Read_Channel_Average(uint32_t channel, uint8_t times);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
typedef enum {
    ALARM_PRESSURE = 1 << 0,
    ALARM_VIBRATION = 1 << 1,
    ALARM_ACK      = 1 << 2,   // 按键确认
} Alarm_mock;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static uint32_t angle_to_ccr(float angle_deg)   // ← 添加函数定义
{
    if (angle_deg < 0)   angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;
    return (uint32_t)(25 + (angle_deg / 180.0f) * (125 - 25));
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMutexId_t oledMutexHandle;   // OLED 互斥锁

uint8_t alarm_muted = 0;   // 0: 正常报警, 1: 静音

// ---------- 按键库相关 ----------
static uint8_t Key_ReadPin(MulitKey_t* key);
static void Key_OnPressed(MulitKey_t* key);
static MulitKey_t key1;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Vibration_Task */
osThreadId_t Vibration_TaskHandle;
const osThreadAttr_t Vibration_Task_attributes = {
  .name = "Vibration_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Pressure_Task */
osThreadId_t Pressure_TaskHandle;
const osThreadAttr_t Pressure_Task_attributes = {
  .name = "Pressure_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for alarm_task */
osThreadId_t alarm_taskHandle;
const osThreadAttr_t alarm_task_attributes = {
  .name = "alarm_task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for oled */
osMutexId_t oledHandle;
const osMutexAttr_t oled_attributes = {
  .name = "oled"
};
/* Definitions for alarm_event */
osEventFlagsId_t alarm_eventHandle;
const osEventFlagsAttr_t alarm_event_attributes = {
  .name = "alarm_event"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Vibration_Task_Function(void *argument);
void Pressure_Task_Function(void *argument);
void alarm_task_function(void *argument);

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
  /* creation of oled */
  oledHandle = osMutexNew(&oled_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  oledMutexHandle = osMutexNew(NULL);   // 创建互斥锁
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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Vibration_Task */
  Vibration_TaskHandle = osThreadNew(Vibration_Task_Function, NULL, &Vibration_Task_attributes);

  /* creation of Pressure_Task */
  Pressure_TaskHandle = osThreadNew(Pressure_Task_Function, NULL, &Pressure_Task_attributes);

  /* creation of alarm_task */
  alarm_taskHandle = osThreadNew(alarm_task_function, NULL, &alarm_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of alarm_event */
  alarm_eventHandle = osEventFlagsNew(&alarm_event_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  MulitKey_Init(&key1, Key_ReadPin, Key_OnPressed, NULL, FALL_BORDER_TRIGGER);
  for(;;)
  {
      MulitKey_Scan(&key1);
      osDelay(10);   // 扫描间隔 10ms，消抖时间 KEY_DEBOUNCE_TIME 单位是扫描次数，默认 10 次即 100ms	  
//	vTaskSuspend(defaultTaskHandle);
//    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Vibration_Task_Function */
/**
* @brief Function implementing the Vibration_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE BEGIN Header_Vibration_Task_Function */
/**
* @brief Function implementing the Vibration_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Vibration_Task_Function */
void Vibration_Task_Function(void *argument)
{
  /* USER CODE BEGIN Vibration_Task_Function */
  uint32_t adc_val;
  uint8_t percent;

  for(;;)
  {
    // 读取 PA3 (ADC_CH3) 平均值
    adc_val = ADC_Read_Channel_Average(ADC_CHANNEL_3, 10);
    // 计算归一化百分比（假设最大值为 3500，可根据实际调整）
    percent = (uint8_t)((float)adc_val / 3500.0f * 100.0f);
    if (percent > 100) percent = 100;

    // 获取 OLED 互斥锁（压力任务也可能在写，必须保护）
    if (osMutexAcquire(oledMutexHandle, 100) == osOK) {
        char buf[20];
        sprintf(buf, "Vib:%4d %3d%%", (int)adc_val, percent);
        OLED_ShowString(0, 16, (uint8_t*)buf, 16, 1);
        OLED_Refresh();
        osMutexRelease(oledMutexHandle);
    }
//    if (osMutexAcquire(oledMutexHandle, 100) == osOK) {
//        char buf[20];
//        OLED_ShowString(0, 48, "Vib:", 16, 1);
//        sprintf(buf, "%4d %3d%%", (int)adc_val, percent);
//        OLED_ShowString(40, 48, (uint8_t*)buf, 16, 1);
//        // 绘制幅度条（x=0, y=56，宽128，高8）
//        OLED_DrawBar(0, 56, 128, 8, percent, 100);
//        OLED_Refresh();
//        osMutexRelease(oledMutexHandle);
//    }

    // 振动报警（阈值 30%）
    if (percent > 20) {
        osEventFlagsSet(alarm_eventHandle,ALARM_VIBRATION);
	}
//    } else {
//		__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3,0);
//		HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_SET);
//        osEventFlagsClear(alarm_eventHandle,ALARM_VIBRATION);
//    }

//    printf("%d\n",percent);
    osDelay(100);
  }
  /* USER CODE END Vibration_Task_Function */
}

/* USER CODE BEGIN Header_Pressure_Task_Function */
/**
* @brief Function implementing the Pressure_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Pressure_Task_Function */
void Pressure_Task_Function(void *argument)
{
  /* USER CODE BEGIN Pressure_Task_Function */
  // 压力传感器校准参数（需根据实际传感器调整）
  const float VCC = 3300.0f;          // ADC 参考电压 mV
  const float Voltage_0 = 160.0f;     // 零点电压 mV
  const float Voltage_40 = 3750.0f;   // 满量程电压 mV (对应 40kPa)
  const float Pressure_Full = 40000.0f; // 满量程压力值 Pa

  char buf[20];
  uint32_t adc_raw;
  float voltage_mV;
  float pressure_Pa;
	
  for(;;)
  {
    // 读取 PA2 (ADC_CH2) 平均值
    adc_raw = ADC_Read_Channel_Average(ADC_CHANNEL_2, 10);
    voltage_mV = (adc_raw * VCC) / 4095.0f;
    // 线性映射
    if (voltage_mV <= Voltage_0) {
        pressure_Pa = 0.0f;
    } else if (voltage_mV >= Voltage_40) {
        pressure_Pa = Pressure_Full;
    } else {
        pressure_Pa = (voltage_mV - Voltage_0) * Pressure_Full / (Voltage_40 - Voltage_0);
    }

    // 获取 OLED 互斥锁
    if (osMutexAcquire(oledMutexHandle, 100) == osOK) {
        // 第一行：压力值
        sprintf(buf, "Pre:%5.0f Pa", (double)pressure_Pa);
        OLED_ShowString(0, 0, (uint8_t*)buf, 16, 1);
        OLED_Refresh();
        osMutexRelease(oledMutexHandle);
    }
//    if (osMutexAcquire(oledMutexHandle, 100) == osOK) {
//        OLED_ShowString(0, 0, "Pressure:", 16, 1);
//        sprintf(buf, "%5.0f Pa ", (double)pressure_Pa);
//        OLED_ShowString(0, 16, (uint8_t*)buf, 16, 1);
//        sprintf(buf, "%4.0f mV ", (double)voltage_mV);
//        OLED_ShowString(0, 32, (uint8_t*)buf, 16, 1);
//        OLED_Refresh();
//        osMutexRelease(oledMutexHandle);
//    }
	
    // 压力报警（阈值可调）
    if (pressure_Pa > 20000.0f) 
    {
		osEventFlagsSet(alarm_eventHandle,ALARM_PRESSURE);
    } 
//    else 
//    {
//		__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3,0);
//		HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_SET);
//        osEventFlagsClear(alarm_eventHandle,ALARM_PRESSURE);
//    }

    printf("%.1f\n",pressure_Pa);
    osDelay(100);
  }
  /* USER CODE END Pressure_Task_Function */
}

/* USER CODE BEGIN Header_alarm_task_function */
/**
* @brief Function implementing the alarm_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_alarm_task_function */
void alarm_task_function(void *argument)
{
  /* USER CODE BEGIN alarm_task_function */
    const uint32_t ALL_ALARMS = ALARM_PRESSURE | ALARM_VIBRATION;
    const uint32_t AUDIO_PSC = 72 - 1;   // 计数频率 = 72MHz / 72 = 1MHz
    uint32_t freq_low  = 1000;           // ARR=1000 → 1kHz
    uint32_t freq_high = 500;            // ARR=500  → 2kHz
    int32_t  dir = 1;
    uint32_t arr_val = freq_low;

    for(;;)
    {
        // 1. 等待传感器报警
        osEventFlagsWait(alarm_eventHandle,
                         ALL_ALARMS,
                         osFlagsWaitAny | osFlagsNoClear,
                         osWaitForever);

        // 2. 激活舵机和LED
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, angle_to_ccr(90.0f));  // 舵机90°
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);          // LED亮

        /* ---------- 配置 TIM4 为音频输出 ---------- */
        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);               // 先停止原 PWM
        htim4.Init.Prescaler = AUDIO_PSC;                      // 1MHz
        htim4.Init.Period    = freq_low - 1;
        HAL_TIM_PWM_Init(&htim4);                              // 重新初始化
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (freq_low - 1) / 2); // 50%占空比
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);              // 启动音频输出

        /* ---------- 警报声扫频循环，直到按键按下 ---------- */
        while (1)
        {
            // 检查按键确认标志
            if (osEventFlagsGet(alarm_eventHandle) & ALARM_ACK)
                break;

            // 更新频率：在 freq_low ~ freq_high 之间往复
            if (arr_val >= freq_low)      dir = -1;
            else if (arr_val <= freq_high) dir = 1;
            arr_val += dir * 5;           // 每步改变5个ARR单位，控制扫频速度

            // 直接写寄存器修改频率和占空比
            TIM4->ARR  = arr_val - 1;
            TIM4->CCR4 = (arr_val - 1) / 2;

            osDelay(10);                  // 10ms 更新一次，听感流畅
        }

        /* ---------- 解除报警 ---------- */
        // 关闭蜂鸣器
        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
        // 舵机归零、LED灭
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, angle_to_ccr(0.0f));
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

        // 清除所有报警标志，准备下一次触发
        osEventFlagsClear(alarm_eventHandle, ALL_ALARMS | ALARM_ACK);

        // 重置扫频起始值，为下次报警做准备
        arr_val = freq_low;
    }  
  /* USER CODE END alarm_task_function */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
// 读引脚回调：按下返回0（低电平），释放返回1
static uint8_t Key_ReadPin(MulitKey_t* key)
{
    // 根据你的实际引脚修改，例如 KEY_GPIO_Port, KEY_Pin
    return HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
}

// 按下回调：发送确认事件
static void Key_OnPressed(MulitKey_t* key)
{
    osEventFlagsSet(alarm_eventHandle, ALARM_ACK);
}
/* USER CODE END Application */

