/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 后驱循迹小车 - STM32F407VET6
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "key_control.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "PID_Node.h"
#include "speed_control.h"      // 新增
#include "Log.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <mpu6050.h>
#include <mpu6050_user.h>
#include <MadgwickAHRS.h>
#include "mpu6050_user.h"
#include "control.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
MotorAT4950 motor1;
MotorAT4950 motor2;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define LOW_FILITER_KP 0.2
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
MPU6050_Data_t* mpu = NULL;
/* USER CODE BEGIN PV */

// ==================== 电机对象 ====================

PID_Node left_pid, right_pid;


uint32_t last_time = 0;
float i1 = 0;
float i2 = 0;
float last_i1 = 0;
float last_i2 = 0; 
float filiter_speed1 = 0.0f;
float filiter_speed2 = 0.0f;

int16_t encoder1_value;
int16_t encoder2_value;
int16_t	delta2;
int16_t	delta1;

extern uint8_t gray_byte;

extern float gray_error;

extern PID_Node yaw_pid;
PID_Node pidGrayscale;

		

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SetMotor1PWM(uint16_t ccr) {
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, ccr);
}
void SetMotor2PWM(uint16_t ccr) {
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, ccr);
}
void SetMotor1IN1(uint8_t level) {
    HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, (GPIO_PinState)level);
}
void SetMotor2IN1(uint8_t level) {
    HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, (GPIO_PinState)level);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
   PID_Limit limit1 = {
        .setpoint_min = 0.0f, .setpoint_max = 0.8f,
        .input_min    = 0.0f, .input_max    = 0.8f,
        .output_min   = -1000.0f,  .output_max   = 1000.0f,
        .integral_max = 1000.0f,    .derivative_max = 1000.0f,
        .deadband     = 0.0f      // 速度误差小于5 RPM时视为0
    };
    PID_Limit limit2 = {
        .setpoint_min = 0.0f, .setpoint_max = 0.8f,
        .input_min    = 0.0f, .input_max    = 0.8f,
        .output_min   = -1000.0f,  .output_max   = 1000.0f,
        .integral_max = 1000.0f,    .derivative_max = 1000.0f,
        .deadband     = 0.0f      // 速度误差小于5 RPM时视为0
    };

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
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  
  MPU_Init();
      // 2. 初始化电机驱动
    MotorInit_AT46950(&motor1,          // 注意函数名有拼写错误
                      SetMotor1PWM,
                      SetMotor1IN1,
                      100);           // Auto_Reload = 100
    MotorInit_AT46950(&motor2,
                      SetMotor2PWM,     
                      SetMotor2IN1,
                      100);
    PID_Node_Init(&left_pid,"Left_PID",2000.0f, 1.0f, 0.0f);   // PID参数需要根据实际情况调整
    PID_Node_Init(&right_pid, "Right_PID", 2000.0f, 1.0f,0.0f);
    PID_Node_SetSetpoint(&left_pid, 0.0f);
    PID_Node_SetSetpoint(&right_pid, 0.0f);

    PID_Node_SetLimit(&left_pid, limit1);
    PID_Node_SetLimit(&right_pid, limit2);
    // OLED初始化
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"F407 Car System", 16, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Initializing...", 16, 1);
    OLED_Refresh();

	AngleControl_Init();
	
    // 按键初始化
    KeyControl_Init();
	Control_Init();
//    // 电机初始化
//    Motor_Init();
//    
//    // PID初始化
//    PID_Init();
//    
//    // 速度环初始化
//    SpeedLoop_Init();

    // 启动PWM
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // 左电机PWM (根据实际引脚修改)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // 右电机PWM (根据实际引脚修改)
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    
// 初始化MPU6050（假设已调用 MPU_Init()）
//if (MPU_Init() != 0) {
//    OLED_ShowString(0, 32, (uint8_t*)"MPU6050 Error!", 16, 1);
//    OLED_Refresh();
//    while(1);
//}

// 显示校准提示
	OLED_Clear();
	OLED_ShowString(0, 0, (uint8_t*)"MPU Calibrating", 16, 1);
	OLED_ShowString(0, 16, (uint8_t*)"Keep Still...", 16, 1);
	OLED_Refresh();
	// 调用校准函数（内部会循环采样并显示进度）
	mpu = MPU6050_GetHandle();

	MPU6050_Calibrate(200);   // 该函数需自行实现进度显示（见下文修改）

	// 校准完成提示
	OLED_Clear();
	OLED_ShowString(0, 0, (uint8_t*)"MPU Ready", 16, 1);
	OLED_Refresh();

    HAL_Delay(200);
    
    LOG_INFO("Car system started with speed loop enabled");
//	printf("左电机: %d rpm, 右电机: %d rpm\r\n", left_motor_speed, right_motor_speed);
//	printf("当前速度: %.2f m/s\r\n", actual_velocity);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  SetDefaultDirection(&motor1, Low_Level);  
  SetDefaultDirection(&motor2, Low_Level);
//		Motor_setSpeed(&motor1,1000 );
//		Motor_setSpeed(&motor2, 400);

      HAL_TIM_Base_Start_IT(&htim4); // 启动定时器4中断，用于周期性任务

  while (1)
  {		
//		static uint32_t last_mpu_update = 0;
		if(HAL_GetTick() - last_time > 20)
		  {	   
			  printf("%.2f,%.2f,%.2f,%.2f,%.2f\n",filiter_speed1,filiter_speed2,yaw_pid.output,left_pid.setpoint,right_pid.setpoint);
	//		LOG_DEBUG("Test print");
			  last_time = HAL_GetTick();
		  }
    	static uint32_t last_display = 0;
		if (HAL_GetTick() - last_display >= 100) {
			last_display = HAL_GetTick();
			char buf[32];
			OLED_Clear();
			snprintf(buf, sizeof(buf), "L:%.2f R:%.2f", filiter_speed2, filiter_speed1);
			OLED_ShowString(0, 0, (uint8_t*)buf, 12, 1);
			snprintf(buf, sizeof(buf), "Yaw:%.1f", mpu->yaw);
			OLED_ShowString(0, 16, (uint8_t*)buf, 12, 1);
			snprintf(buf,sizeof(buf),"gray_byte:0x%x",gray_byte);
			OLED_ShowString(0, 32, (uint8_t*)buf, 12, 1);
			OLED_Refresh();
		}


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
//  static uint32_t count = 0 ;
//	uint32_t now = HAL_GetTick();
//	float target_angle = 0.0f;   // 可替换为遥控/按键输入的目标角度
//	float base_speed = 0.1f;     // 基础前进速度

//    if (htim->Instance == TIM4) {
//        // 这里可以放置定时器中断处理代码，例如周期性任务
//		count++;
//		if(count % 5 ==0)
//		{
//			MPU6050_Update();   // ★ 关键：更新姿态数据
//			static int16_t last1 = 0;
//			static int16_t last2 = 0;
//				
//			 encoder1_value = (int16_t) __HAL_TIM_GetCounter(&htim2);
//			 encoder2_value = -(int16_t) __HAL_TIM_GetCounter(&htim3);
//			delta1 =	encoder1_value - last1 ;
//			last1 = encoder1_value;
//			delta2 =	encoder2_value - last2;
//			last2 = encoder2_value;
//			 i1 =	delta1 * SPEED_COEFF * 10.0f;
//			 i2 =	delta2 * SPEED_COEFF * 10.0f;
//			filiter_speed1 = i1 * LOW_FILITER_KP + last_i1 * (1 - LOW_FILITER_KP);
//			filiter_speed2 = i2 * LOW_FILITER_KP + last_i2 * (1 - LOW_FILITER_KP);
//			last_i1 = filiter_speed1;
//			last_i2 = filiter_speed2;

//		}
//		PID_Node_UpdateMeasurement(&left_pid, filiter_speed1);
//		PID_Node_UpdateMeasurement(&right_pid, filiter_speed2);

//		AngleControl_Update(base_speed, target_angle, 4.0f);
//		PID_ExecuteNode(&left_pid, 4.0f);   // 这里的dt需要根据实际控制周期调整
//		PID_ExecuteNode(&right_pid, 4.0f);  // 这里的dt需要根据实际控制周期调整
//		Motor_setSpeed(&motor1,left_pid.output );
//		Motor_setSpeed(&motor2, right_pid.output);

// 
//		static uint32_t last_angle_time = 0;
//		
//        if(count >= 200) { // 每200次中断（约800ms）执行一次
//            count = 0;
//            //在这里执行需要周期性处理的任务，例如更新OLED显示、读取传感器数据等
////            LOG_INFO("TIM4 interrupt triggered");
//        }
//					// 示例：每20ms执行一次角度环 
    static uint32_t count = 0;
    float base_speed = 0.30f;           // 基础速度 m/s（可调）
    float gray_to_speed_gain = 0.5f;    // 灰度输出->速度差系数
    float max_speed_diff = 0.3f;        // 最大速度差限幅

    if (htim->Instance == TIM4)
    {
        count++;
        // 每5次中断更新一次 MPU6050 和速度（原逻辑保留）
        if (count % 5 == 0)
        {
            MPU6050_Update();
            static int16_t last1 = 0, last2 = 0;
            encoder1_value = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
            encoder2_value = -(int16_t)__HAL_TIM_GET_COUNTER(&htim3);
            delta1 = encoder1_value - last1;
            last1 = encoder1_value;
            delta2 = encoder2_value - last2;
            last2 = encoder2_value;

            i1 = delta1 * SPEED_COEFF * 10.0f;
            i2 = delta2 * SPEED_COEFF * 10.0f;
            filiter_speed1 = i1 * LOW_FILITER_KP + last_i1 * (1 - LOW_FILITER_KP);
            filiter_speed2 = i2 * LOW_FILITER_KP + last_i2 * (1 - LOW_FILITER_KP);
            last_i1 = filiter_speed1;
            last_i2 = filiter_speed2;
        }

        // ========== 串级控制：灰度环（外环）→ 速度环（内环） ==========
        // 每10次中断执行一次灰度 PID（约40ms，假设中断周期4ms）
        
            // 1. 读取灰度传感器
		gray_byte = GPIOE->IDR & 0xFF;
		gray_error = CalculateGrayError_Advanced(gray_byte);
		
		// 2. 灰度 PID 计算
		PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
		PID_ExecuteNode(&pidGrayscale, 4.0f);   // dt 按您要求保持4.0
		float gray_output = pidGrayscale.output;   // -1000 ~ 1000
		
		// 3. 转换为速度差
		float speed_diff = gray_output;
		if (speed_diff > max_speed_diff) speed_diff = max_speed_diff;
		if (speed_diff < -max_speed_diff) speed_diff = -max_speed_diff;
		
		// 4. 计算左右轮目标速度
		float target_left = base_speed + speed_diff;
		float target_right = base_speed - speed_diff;
		if (target_left < 0) target_left = 0;
		if (target_right < 0) target_right = 0;
		
		// 5. 设置速度内环的设定点
		PID_Node_SetSetpoint(&left_pid, target_left);
		PID_Node_SetSetpoint(&right_pid, target_right);
        
        // ===========================================================

        // 更新速度测量值
        PID_Node_UpdateMeasurement(&left_pid, filiter_speed1);
        PID_Node_UpdateMeasurement(&right_pid, filiter_speed2);
        
        // 执行速度 PID（内环）
        PID_ExecuteNode(&left_pid, 4.0f);
        PID_ExecuteNode(&right_pid, 4.0f);
        
        // 电机输出
        Motor_setSpeed(&motor1, (int16_t)left_pid.output);
        Motor_setSpeed(&motor2, (int16_t)right_pid.output);
        
        // 计数器清零（保留原逻辑）
        if (count >= 200) {
            count = 0;
        }
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
