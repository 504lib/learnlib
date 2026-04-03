#include "tasks.h"
#include "main.h"      // 为了 HAL_GPIO_TogglePin、LED 定义
#include "usart.h"     // 为了 huart1
#include <stdio.h>     // 为了 snprintf
#include <string.h>    // 为了 strlen
#include "main.h" 
#include "oled.h"
#include "mpu6050_user.h"
#include "grayscale.h"
#include "key_control.h"
#include "PID_Node.h"
#include "control.h"   // 新增，包含全局 PID 实例和速度变量

Protothread_t task1_pt;
Protothread_t task2_pt;
Protothread_t SerialTask_pt;
Protothread_t oled_pt;


// 初始化所有任务
void Tasks_Init(void)
{
    PT_INIT(&task1_pt);
    PT_INIT(&task2_pt);
    PT_INIT(&SerialTask_pt);
	PT_INIT(&oled_pt); 
}

// 任务函数实现
void task1(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while(1)
    {
        HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
        PT_WAIT_TICK(pt, 500);
    }
    PT_END(pt);
}

void task2(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while(1)
    {
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
        PT_WAIT_TICK(pt, 1000);
    }
    PT_END(pt);
}

void SerialTask(Protothread_t* pt)
{
    char buffer[50] = {0};
    mpu = MPU6050_GetHandle();   // 获取全局实例指针
    PT_BEGIN(pt);
    static size_t times = 0;
    while(1)
    {
//        snprintf(buffer, sizeof(buffer), "Serial task has execute %d time(s)\n", ++times);
		snprintf(buffer, sizeof(buffer),"Y:%.1f R:%.1f\n", mpu->yaw, mpu->roll);
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
        PT_WAIT_TICK(pt, 1000);
    }
    PT_END(pt);
}

void OLED_Task(Protothread_t* pt)
{
    char buffer[20] = {0};
    uint8_t mode = 0;
    PT_BEGIN(pt);
    while (1)
    {
        mode = KeyControl_GetDisplayMode();
//		if (mode == 0) {
//			// 临时调试：显示 mpu 地址和 yaw/roll 原始值
//			snprintf(buffer, sizeof(buffer), "mpu:0x%p", mpu);
//			OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
//			if (mpu != NULL) {
//				snprintf(buffer, sizeof(buffer), "y=%.1f r=%.1f", mpu->yaw, mpu->roll);
//				OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
//			} else {
//				OLED_ShowString(0, 16, "mpu is NULL", 16, 1);
//			}
//		}
        if (mode == 0)   // 姿态模式（简化，只显示当前角度）
        {
            snprintf(buffer, sizeof(buffer), "Y:%.1f", mpu->yaw);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "R:%.1f", mpu->roll);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            // 不再显示 Y_TAR 和 A_target
        }
        else if (mode == 1)   // 灰度模式
        {
            snprintf(buffer, sizeof(buffer), "gray:");
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            for (size_t i = 0; i < 8; i++)
            {
                if (gray_byte & (1 << i))
                    OLED_ShowString(48 + i * 8, 0, (uint8_t*)"1", 16, 1);
                else
                    OLED_ShowString(48 + i * 8, 0, (uint8_t*)"0", 16, 1);
            }
            snprintf(buffer, sizeof(buffer), "gray_err:%.2f", gray_error);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
        }
        else if (mode == 2)   // 速度模式
        {
            snprintf(buffer, sizeof(buffer), "val_A:%.2f", Actual_Speed_A);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_B:%.2f", Actual_Speed_B);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_A_T:%.2f", pidMotor1Speed.setpoint);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_B_T:%.2f", pidMotor2Speed.setpoint);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        OLED_Refresh();
        PT_WAIT_TICK(pt, 20);
    }
    PT_END(pt);
}

//void task4(Protothread_t* pt)
//{
//	PT_BEGIN(pt);
//	while(1)
//	{
//		uint8_t mode = 0;
//		PT_WAIT_UNTIL(pt,mode == 1);
//		PT_WAIT_TICK(pt,20);
//	}
//	
//	PT_END(pt);
//}
