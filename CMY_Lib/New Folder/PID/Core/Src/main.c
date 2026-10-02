/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "./Log.h"
#include "Motor_AT4950.h"
#include "./PID_Node.h"
#include <stdbool.h>
#include <stdlib.h>
// #include "mpu6050_user.h"     

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define WHEEL_CIRCUMFERENCE 0.1508f   // 车轮周长 m
#define EFFECTIVE_PPR 1040            // 每转脉冲数
// #define ENCODER_DIFF_LIMIT   100    // 单周期最大合理增量，按你的实际调整
// #define ENCODER_DIFF_DEAD    0.5f   // 死区，噪声


// #define ENCODER_DIFF_LIMIT   300     // 单周期最大合理增量，根据实际调整
// #define ENCODER_DEAD_ZONE    2       // 增量死区
// #define MED_N                5       // 中值滤波窗口（奇数）
// #define AVG_N                6       // 滑动平均窗口
// #define SPEED_LPF_ALPHA      0.15f   // 速度一阶低通系数
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

//串口发送函数
void ysy_tx(const char* buffer, size_t buffer_size)
{
  HAL_UART_Transmit(&huart6,(uint8_t*)buffer,buffer_size,100);
}

// ========== 左轮底层函数 ==========
// 假设左轮PWM用定时器1的通道1（ARR=10000），IN1接PA1
void Left_PWM_Set(uint16_t ccr_value) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value);
}
void Left_IN1_Set(uint8_t level) {
    HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, level);
}

// ========== 右轮底层函数 ==========
// 假设右轮PWM用定时器1的通道2（ARR=10000），IN1接PA2
void Right_PWM_Set(uint16_t ccr_value) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr_value);
}
void Right_IN1_Set(uint8_t level) {
    HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, level);
}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

MotorAT4950 motor1;
MotorAT4950 motor2;
PID_Node motor1_Pid;
PID_Node motor2_Pid;


/* 走直线参数 */
float   line_speed     = 0.5f;      // 基础速度 m/s
int32_t total_pulse_L  = 0;         // 左轮累计脉冲
int32_t total_pulse_R  = 0;         // 右轮累计脉冲
float   sync_integral  = 0.0f;      // 同步 PI 积分

/* 同步 PI 参数，先给初值，后面调 */
#define SYNC_KP  0.0005f
#define SYNC_KI  0.00002f
#define SYNC_LIMIT 0.3f


// /* ★ 新增：航向环 */
// PID_Node  yaw_Pid;
// float     target_yaw      = 0.0f;
// float     yaw_correction  = 0.0f;

// #define YAW_KP            3.0f
// #define YAW_KI            0.05f
// #define YAW_KD            0.8f
// #define YAW_OUTPUT_LIMIT  0.4f
// #define YAW_DEADBAND      0.5f

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

float Actual_Speed_A = 0;
float Actual_Speed_B = 0;

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

 // 编码器速度反馈（在中断中调用）
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
	//     // ---- 1. 增量限幅，防止噪声尖峰 ----
  //  if (diff_A >  ENCODER_DIFF_LIMIT) diff_A =  ENCODER_DIFF_LIMIT;
  //  if (diff_A < -ENCODER_DIFF_LIMIT) diff_A = -ENCODER_DIFF_LIMIT;
  //  if (diff_B >  ENCODER_DIFF_LIMIT) diff_B =  ENCODER_DIFF_LIMIT;
  //  if (diff_B < -ENCODER_DIFF_LIMIT) diff_B = -ENCODER_DIFF_LIMIT;
	
	// 	    // 2. 死区：如果增量很小，直接当0，防止静止抖动
  //  if (abs(diff_A) < 2) diff_A = 0;
  //  if (abs(diff_B) < 2) diff_B = 0;
	
    if (dt > 0.0f) {
        float dt_ms = dt / 1000.0f;
        Actual_Speed_A = (diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
        Actual_Speed_B =  (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
    }

    // 简单低通滤波
    static float filtered_A = 0, filtered_B = 0;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;

// 			    // ---- 1. 增量限幅，防止噪声尖峰 ----
//    if (diff_A >  ENCODER_DIFF_LIMIT) diff_A =  ENCODER_DIFF_LIMIT;
//    if (diff_A < -ENCODER_DIFF_LIMIT) diff_A = -ENCODER_DIFF_LIMIT;
//    if (diff_B >  ENCODER_DIFF_LIMIT) diff_B =  ENCODER_DIFF_LIMIT;
//    if (diff_B < -ENCODER_DIFF_LIMIT) diff_B = -ENCODER_DIFF_LIMIT;

//    // ---- 2. 死区，防止静止抖动 ----
//    if (abs(diff_A) < ENCODER_DEAD_ZONE) diff_A = 0;
//    if (abs(diff_B) < ENCODER_DEAD_ZONE) diff_B = 0;

//    // ---- 3. 计算原始速度 ----
//    float spd_A = 0, spd_B = 0;
//    if (dt > 0.0f) {
//        float dt_s = dt / 1000.0f;
//        spd_A = (diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_s);
//        spd_B = (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_s);
//    }

//    // ---- 4. 中值滤波（去单点尖峰） ----
//    static float med_buf_A[MED_N] = {0}, med_buf_B[MED_N] = {0};
//    static int   med_idx_A = 0, med_idx_B = 0;

//    med_buf_A[med_idx_A] = spd_A;
//    med_idx_A = (med_idx_A + 1) % MED_N;
//    float tmp_A[MED_N];
//    for (int i = 0; i < MED_N; i++) tmp_A[i] = med_buf_A[i];
//    for (int i = 0; i < MED_N - 1; i++) {
//        for (int j = 0; j < MED_N - 1 - i; j++) {
//            if (tmp_A[j] > tmp_A[j + 1]) {
//                float t = tmp_A[j]; tmp_A[j] = tmp_A[j + 1]; tmp_A[j + 1] = t;
//            }
//        }
//    }
//    spd_A = tmp_A[MED_N / 2];

//    med_buf_B[med_idx_B] = spd_B;
//    med_idx_B = (med_idx_B + 1) % MED_N;
//    float tmp_B[MED_N];
//    for (int i = 0; i < MED_N; i++) tmp_B[i] = med_buf_B[i];
//    for (int i = 0; i < MED_N - 1; i++) {
//        for (int j = 0; j < MED_N - 1 - i; j++) {
//            if (tmp_B[j] > tmp_B[j + 1]) {
//                float t = tmp_B[j]; tmp_B[j] = tmp_B[j + 1]; tmp_B[j + 1] = t;
//            }
//        }
//    }
//    spd_B = tmp_B[MED_N / 2];

//    // ---- 5. 滑动平均 ----
//    static float avg_buf_A[AVG_N] = {0}, avg_buf_B[AVG_N] = {0};
//    static int   avg_idx_A = 0, avg_idx_B = 0;

//    avg_buf_A[avg_idx_A] = spd_A;
//    avg_idx_A = (avg_idx_A + 1) % AVG_N;
//    float sum_A = 0;
//    for (int i = 0; i < AVG_N; i++) sum_A += avg_buf_A[i];
//    spd_A = sum_A / AVG_N;

//    avg_buf_B[avg_idx_B] = spd_B;
//    avg_idx_B = (avg_idx_B + 1) % AVG_N;
//    float sum_B = 0;
//    for (int i = 0; i < AVG_N; i++) sum_B += avg_buf_B[i];
//    spd_B = sum_B / AVG_N;

//    // ---- 6. 一阶低通滤波 ----
//    static float filtered_A = 0, filtered_B = 0;
//    filtered_A = (1.0f - SPEED_LPF_ALPHA) * filtered_A + SPEED_LPF_ALPHA * spd_A;
//    filtered_B = (1.0f - SPEED_LPF_ALPHA) * filtered_B + SPEED_LPF_ALPHA * spd_B;

//    Actual_Speed_A = filtered_A;
//    Actual_Speed_B = filtered_B;
}

// /* ★ 新增：角度误差归一化 */
// static float NormalizeAngleError(float error)
// {
//     while (error >  180.0f) error -= 360.0f;
//     while (error < -180.0f) error += 360.0f;
//     return error;
// }

// /* ★ 新增：航向环的误差计算（带死区） */
// static float YawErrorCalculation(float setpoint, float measured)
// {
//     float err = NormalizeAngleError(setpoint - measured);
//     if (fabsf(err) < YAW_DEADBAND) err = 0.0f;
//     return err;
// }

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
  MX_TIM1_Init();
  MX_USART6_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */



  //开启PWM输出
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);

  //开启编码器模式
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

  //初始化电机驱动
  MotorInit_AT46950(&motor1 ,Left_PWM_Set,Left_IN1_Set,1000);
  MotorInit_AT46950(&motor2 ,Right_PWM_Set,Right_IN1_Set,1000);

  //设置电机初始方向
  SetDefaultDirection(&motor1,High_Level);  
  SetDefaultDirection(&motor2,High_Level);

  //设置电机初始速度
  Motor_setSpeed(&motor1,0);
  Motor_setSpeed(&motor2,0);

  //初始化Log日志模块
  LOG_Init(ysy_tx);
  //设置日志输出等级为DEBUG
  LOG_Set_Level(LOG_LEVEL_DEBUG);

  //初始化PID控制器
  PID_Node_Init(&motor1_Pid, "Motor1_PID", 700.0f, 3.0f, 3.0f);
  PID_Node_Init(&motor2_Pid, "Motor2_PID", 700.0f, 3.0f, 3.0f);

  PID_Node_SetIntegralAttenuationKp(&motor1_Pid, 0.9999f);
	PID_Node_SetIntegralAttenuationKp(&motor2_Pid, 0.9999f);

  // PID_Node_SetSetpoint(&motor1_Pid, 0.5f);
  // PID_Node_SetSetpoint(&motor2_Pid, 0.5f);

  // 设置motor1的PID限制参数
  PID_Node_SetLimit(&motor1_Pid, (PID_Limit){
    .setpoint_min = 0.0f, 
    .setpoint_max = 2.0f, 
    .input_min = 0.0f, 
    .input_max = 2.0f, 
    .output_min = 0.0f, 
    .output_max = 1000.0f, 
    .integral_max = 1000.0f, 
    .derivative_max = 1000.0f, 
    .deadband = 0.00f
  });

  // 设置motor2的PID限制参数
  PID_Node_SetLimit(&motor2_Pid, (PID_Limit){
    .setpoint_min = 0.0f, 
    .setpoint_max = 2.0f, 
    .input_min = 0.0f, 
    .input_max = 2.0f, 
    .output_min = 0.0f, 
    .output_max = 1000.0f, 
    .integral_max = 1000.0f, 
    .derivative_max = 1000.0f, 
    .deadband = 0.00f
  });
	
	uint32_t last_tick = 0;
	bool change = false;


  // /* ★ 新增：航向环 PID 初始化 */
  // PID_Node_Init(&yaw_Pid, "Yaw_PID", YAW_KP, YAW_KI, YAW_KD);
  // PID_Node_SetIntegralAttenuationKp(&yaw_Pid, 0.99f);
  // PID_Node_SetLimit(&yaw_Pid, (PID_Limit){
  //     .setpoint_min   = -180.0f, .setpoint_max   = 180.0f,
  //     .input_min      = -180.0f, .input_max      = 180.0f,
  //     .output_min     = -YAW_OUTPUT_LIMIT,
  //     .output_max     =  YAW_OUTPUT_LIMIT,
  //     .integral_max   = 50.0f,
  //     .derivative_max = 200.0f,
  //     .deadband       = 0.0f,
  // });
  // PID_Node_SetCustomCallback(&yaw_Pid, (PID_Custom_Functions){
  //     .custom_error_calculation = YawErrorCalculation,
  // });

  // /* ★ 新增：IMU 初始化 + 校准（校准期间车必须静止） */
  // MPU_Init();
  // MPU6050_Calibrate(200);
  // MPU6050_Update();      // 空跑一次，让 yaw 有值


    // 初始 setpoint，等待 2 秒让车摆好
  PID_Node_SetSetpoint(&motor1_Pid, 0.0f);
  PID_Node_SetSetpoint(&motor2_Pid, 0.0f);

  // 清零编码器，从零开始计数
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  __HAL_TIM_SET_COUNTER(&htim3, 0);
  total_pulse_L = 0;
  total_pulse_R = 0;
  sync_integral = 0.0f;

  HAL_Delay(2000);

  // /* ★ 新增：车静止摆好后，捕获初始航向作为目标 */
  // MPU6050_Update();
  // target_yaw = MPU6050_GetHandle()->yaw;
  // PID_Node_SetSetpoint(&yaw_Pid, target_yaw);

  //开启定时器4
  HAL_TIM_Base_Start_IT(&htim4);

  // 开始走直线
  PID_Node_SetSetpoint(&motor1_Pid, line_speed);
  PID_Node_SetSetpoint(&motor2_Pid, line_speed);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {  
    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);
    // HAL_Delay(1000);
    //  Motor_setSpeed(&motor1,1000);
    //  Motor_setSpeed(&motor2,1000);
    // LOG_DEBUG("Testing debug log");
    // if (HAL_GetTick() - last_tick > 5000)
    // {
    //   HAL_GPIO_TogglePin(LED0_PCB_GPIO_Port, LED0_PCB_Pin);
    //   last_tick = HAL_GetTick();
    //   change = !change;
    //   if (change)
    //   {
    //       PID_Node_SetSetpoint(&motor1_Pid, 0.6f);
    //       PID_Node_SetSetpoint(&motor2_Pid, 0.6f);
    //   }
    //   else
    //   {
    //       PID_Node_SetSetpoint(&motor1_Pid, 1.0f);
    //       PID_Node_SetSetpoint(&motor2_Pid, 1.0f);
    //   }
    // }

    // Motor_setSpeed(&motor1, 200);
    // Motor_setSpeed(&motor2, 200);

    LOG_RAW("L=%ld R=%ld A=%.2f B=%.2f\n",
        (long)total_pulse_L, (long)total_pulse_R,
        Actual_Speed_A, Actual_Speed_B);
    LOG_RAW(" %.2f,%.2f\n", Actual_Speed_A, Actual_Speed_B);
    HAL_Delay(20);

    // static uint32_t last_imu_tick = 0;

    // if (HAL_GetTick() - last_imu_tick >= 20)
    // {
    //     last_imu_tick += 20;           // 累加而不是赋值，避免漂移

    //     MPU6050_Update();              // 严格 50Hz 调用

    //     /* 日志降到 10Hz，减少阻塞 */
    //     static uint8_t log_div = 0;
    //     if (++log_div >= 5)
    //     {
    //         log_div = 0;
    //         LOG_RAW("yaw=%.2f tgt=%.2f corr=%.3f A=%.2f B=%.2f\n",
    //                 MPU6050_GetHandle()->yaw,
    //                 target_yaw,
    //                 yaw_correction,
    //                 Actual_Speed_A, Actual_Speed_B);
    //     }
    //   }

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) // 检查是否是定时器4的中断
    {
        // // 读取电机1和电机2的速度（假设有函数可以获取速度）
        // float motor1_speed = Motor_getSpeed(&motor1);
        // float motor2_speed = Motor_getSpeed(&motor2);

        // // 更新PID控制器的测量值
        // motor1_Pid.measured_value = motor1_speed;
        // motor2_Pid.measured_value = motor2_speed;

        // // 计算PID输出
        // PID_Node_Compute(&motor1_Pid);
        // PID_Node_Compute(&motor2_Pid);

        // // 设置电机速度为PID输出值
        // Motor_setSpeed(&motor1, motor1_Pid.output);
        // Motor_setSpeed(&motor2, motor2_Pid.output);

        static uint32_t last_motor1_cnt = 0;
        static uint32_t last_motor2_cnt = 0;

        static uint32_t counter = 0;

        if(counter % 2 == 0)
        {
          PID_ExecuteNode(&motor1_Pid,2);
          PID_ExecuteNode(&motor2_Pid,2);
          Motor_setSpeed(&motor1, motor1_Pid.output);
          Motor_setSpeed(&motor2, motor2_Pid.output);
        }


        if(counter % 20 == 0)
        {
          int32_t motor1_cnt = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
          int32_t motor2_cnt = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
          int32_t diff_A = (motor1_cnt - last_motor1_cnt);
          int32_t diff_B = (motor2_cnt - last_motor2_cnt);

          diff_A = -diff_A; // 根据编码器方向调整符号

          Control_UpdateSpeedFeedback(diff_A, diff_B, 20); // 假设 dt 为 20 ms
          last_motor1_cnt = motor1_cnt;
          last_motor2_cnt = motor2_cnt;

          PID_Node_UpdateMeasurement(&motor1_Pid, Actual_Speed_A);
          PID_Node_UpdateMeasurement(&motor2_Pid, Actual_Speed_B);

         // ===== 走直线同步修正 =====

          total_pulse_L += diff_A;
          total_pulse_R += diff_B;

            // 到终点就停车
          float sync_error = (float)(total_pulse_L - total_pulse_R);
          sync_integral += sync_error * 0.02f;
          float sync_correction = SYNC_KP * sync_error + SYNC_KI * sync_integral;
          if (sync_correction >  SYNC_LIMIT) sync_correction =  SYNC_LIMIT;
          if (sync_correction < -SYNC_LIMIT) sync_correction = -SYNC_LIMIT;
          PID_Node_SetSetpoint(&motor1_Pid, line_speed - sync_correction);
          PID_Node_SetSetpoint(&motor2_Pid, line_speed + sync_correction);
       

          __HAL_TIM_SET_COUNTER(&htim2, 0); // 重置计数器
          __HAL_TIM_SET_COUNTER(&htim3, 0); // 重置计数器  
          last_motor1_cnt = 0;
          last_motor2_cnt = 0;
      }
     

        // static uint8_t times = 0;
        
        // if (counter % 500 == 0) // 每500次中断（假设
        // {
        //     LOG_INFO("current times: %d", times++);
        // }
        counter ++;
    }
    
}

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
