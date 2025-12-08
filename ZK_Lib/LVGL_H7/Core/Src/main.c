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
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <stdbool.h>
//#include "LED.h"
//#include "ili9341.h"
#include <stdio.h>
#include <string.h>

#include "lvgl.h"                // 它为整个LVGL提供了更完整的头文件引用
#include "lv_port_disp.h"        // LVGL的显示支持
#include  "lv_port_indev.h"      // LVGL的输入设备支持

#include "touch.h"
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

volatile uint8_t flag = 0;
 uint8_t chr[50] = {0};
 uint8_t chr_single = 0;
bool lv_flush_flag = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static lv_obj_t * label;

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);

    /*Refresh the text*/
    lv_label_set_text_fmt(label, "%d", lv_slider_get_value(slider));
}

void mybtn_cb(lv_event_t * e)
{
  lv_obj_t * btn = lv_event_get_target(e);
  if (e->code == LV_EVENT_CLICKED)
  {
    static uint16_t count = 0;
    count++;
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "Pressed: %d", count);
  }
  
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  TP_Init();
  // LCD_Fill_DMA_DoubleBuffer(50,50,200,200,BLUE); // 测试DMA双缓冲填充
//  LCD_Clear(RED);//清全屏白色
	lv_init();                             // LVGL 初始化
	lv_port_disp_init();  
  lv_port_indev_init();
//  lv_color_t red = lv_color_make(255, 0, 0);   // 红色
//  lv_color_t green = lv_color_make(0, 255, 0); // 绿色
//  lv_color_t blue = lv_color_make(0, 0, 255);  // 蓝色
//  lv_color_t yellow = lv_color_make(255, 255, 0); // 黄色
//    lv_obj_t *rect = lv_obj_create(lv_scr_act()); // 父对象为当前屏幕
//    lv_obj_set_size(rect, 200, 200);              // 设置矩形大小
//    lv_obj_set_pos(rect, 50, 0);                 // 设置矩形位置

//  // 定义一个颜色数组用于测试
//  lv_color_t colors[] = {red, green, blue, yellow};
//  uint8_t color_index = 0;
	
	    // 按钮
  lv_obj_t *myBtn = lv_btn_create(lv_scr_act());                               // 创建按钮; 父对象：当前活动屏幕
  lv_obj_set_pos(myBtn, 10, 10);                                               // 设置坐标
  lv_obj_set_size(myBtn, 120, 50);                                             // 设置大小
  lv_obj_add_event_cb(myBtn, mybtn_cb, LV_EVENT_CLICKED, NULL); // 添加点击事件回调
  
  // 按钮上的文本
  lv_obj_t *label_btn = lv_label_create(myBtn);                                // 创建文本标签，父对象：上面的btn按钮
  lv_obj_align(label_btn, LV_ALIGN_CENTER, 0, 0);                              // 对齐于：父对象
  lv_label_set_text(label_btn, "Test");                                        // 设置标签的文本

  // 独立的标签
  lv_obj_t *myLabel = lv_label_create(lv_scr_act());                           // 创建文本标签; 父对象：当前活动屏幕
  lv_label_set_text(myLabel, "Hello world!");                                  // 设置标签的文本
  lv_obj_align(myLabel, LV_ALIGN_CENTER, 0, 0);                                // 对齐于：父对象
  lv_obj_align_to(myBtn, myLabel, LV_ALIGN_OUT_TOP_MID, 0, -20);               // 对齐于：某对象

 
  lv_obj_t* slider = lv_slider_create(lv_scr_act());                           // 创建滑块; 父对象：当前活动屏幕
  lv_obj_set_pos(slider, (320 - 200) / 2, 200);                                               // 设置坐标
  lv_obj_set_size(slider, 200, 20);                                             // 设置大小
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);                                 // 设置初始值  
  lv_slider_set_range(slider, 0, 180);                                 // 设置范围0-100

  label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "0");
  lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -30);    /*Align top of the slider*/
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  HAL_TIM_Base_Start_IT(&htim1); // 启动定时器中断


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // if(FT6336_Scan())
    // {
    //   printf("Touch at (%d, %d)\n", tp_dev.x[0], tp_dev.y[0]);
    //   // lv_indev_data_t data;
    //   // data.point.x = tp_dev.x[0];
    //   // data.point.y = tp_dev.y[0];
    //   // data.state = LV_INDEV_STATE_PR; // 触摸按下状态
    //   // lv_indev_set_point(lv_get_indev(LV_INDEV_TYPE_POINTER), &data);
    
	  // }
    // else
    // {
    //   lv_indev_data_t data;
    //   data.state = LV_INDEV_STATE_REL; // 触摸释放状态
    //   lv_indev_set_point(lv_get_indev(LV_INDEV_TYPE_POINTER), &data);
    // }
    if (lv_flush_flag)
    {
      lv_flush_flag  = false;
      lv_task_handler(); // 处理LVGL任务
      // printf("lv_task_handler called\n");
    }
  // static uint32_t last_tick = 0;
  //    if (HAL_GetTick() - last_tick >= 1000)
  //    {
  //        last_tick = HAL_GetTick();
  //        lv_obj_set_style_bg_color(rect, colors[color_index], LV_PART_MAIN);
  //        color_index = (color_index + 1) % 4; // 循环切换颜色
  //    }   
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t lv_tick = 0;
  if(htim->Instance == TIM1)
  {

    lv_tick_inc(1); // 每1毫秒调用一次lv_tick_inc
    lv_tick++;
    if(lv_tick % 5 == 0) // 每5毫秒调用一次lv_task_handler
    {
      
      lv_flush_flag = true;

    }
	if(lv_tick >= 200)
	{
		// printf("test\n");
		lv_tick = 0;
	}
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) 
  {
    HAL_UART_Receive_IT(&huart1,&chr_single,1);
    printf("%c",chr_single);
    if(chr_single == '\n')
    // if(strlen(chr) >= 1) 
    {
      flag = 1;
    } 
  }
  
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
