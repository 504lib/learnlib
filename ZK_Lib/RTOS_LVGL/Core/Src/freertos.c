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
#include "ili9341.h"
#include "usart.h"
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
/* Definitions for LVGL_TASK */
osThreadId_t LVGL_TASKHandle;
const osThreadAttr_t LVGL_TASK_attributes = {
  .name = "LVGL_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void LVGL_Task(void *argument);

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
  /* creation of LVGL_TASK */
  LVGL_TASKHandle = osThreadNew(LVGL_Task, NULL, &LVGL_TASK_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_LVGL_Task */
void test_max_fps(void)
{
    uint32_t start_time, end_time;
    uint32_t frame_count = 0;
    uint16_t colors[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN};
    uint8_t color_index = 0;
    
    start_time = HAL_GetTick();
    
    // 测试10秒内的帧数
    while((HAL_GetTick() - start_time) < 10000) 
    {
        LCD_Clear(colors[color_index]);
        color_index = (color_index + 1) % 6;
        frame_count++;
        if(frame_count 	% 100 == 0) 
        {
            // 每100帧打印一次
            uint32_t current_time = HAL_GetTick() - start_time;
            printf("FPS: %.1f\n", (float)frame_count * 1000 / current_time);
        }
    }
    
    printf("最终平均FPS: %.1f\n", (float)frame_count * 1000 / 10000);
}
/**
  * @brief  Function implementing the LVGL_TASK thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_LVGL_Task */

void LVGL_Task(void *argument)
{
  /* USER CODE BEGIN LVGL_Task */
  LCD_Init();
  // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,WHITE);
  LCD_Clear(BLACK);
  // LCD_DrawFilledCircle(LCD_WIDTH / 2, LCD_HEIGHT / 2, 50, RED);
    printf("LVGL Task Running...\n");
  /* Infinite loop */
  for(;;)
  {
    // printf("LVGL Task Running...\n");
    // HAL_UART_Transmit(&huart1,(uint8_t*)"LVGL Task Running...\n",20,1000);
    // for (uint16_t i = 0; i < LCD_W; i++)
    // {
    //   LCD_ShowString_DMA(i,10,(uint8_t*)"test",RED,WHITE,16,0);
    //   osDelay(5);
    // }
    test_max_fps();
    // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,WHITE);
    // osDelay(500);
    // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,BLUE);
    // osDelay(500);
    // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,GRED);
    // osDelay(500);
    // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,RED);
    // osDelay(500);
    // LCD_Fill_DMA_DoubleBuffer(0,0,LCD_W,LCD_H,GREEN);
    osDelay(10000);
  }
  /* USER CODE END LVGL_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

