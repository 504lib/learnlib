#include "tasks.h"
#include "../SPI_OLED/oled.h"
#include "../MPU_6050/mpu6050_user.h"
#include "../Control_Speed/Control_Speed.h"
#include "../key_control/key_control.h"
#include "../Log/Log.h"
#include <stdio.h>

// 外部变量（定义在 empty.c 中）
extern volatile uint8_t  gray_byte;
extern volatile float    gray_error;
extern volatile uint16_t distance_mm;

Protothread_t oled_pt;

void Tasks_Init(void)
{
    PT_INIT(&oled_pt);
}

void OLED_Task(Protothread_t* pt)
{
    static uint32_t times = 0;
    char buffer[30] = {0};
    uint8_t mode = 0;
    PT_BEGIN(pt);
    while (1)
    {
        mode = KeyControl_GetDisplayMode();

        if (mode == 0)   // 默认模式（原版显示不动）
        {
            MPU6050_Data_t* mpu = MPU6050_GetHandle();
            snprintf(buffer, sizeof(buffer), "yaw:%.1f", mpu->yaw);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "times:%u", times++);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "%d%d%d%d%d%d%d%d",
                (gray_byte & 0x01) ? 1 : 0,
                (gray_byte & 0x02) ? 1 : 0,
                (gray_byte & 0x04) ? 1 : 0,
                (gray_byte & 0x08) ? 1 : 0,
                (gray_byte & 0x10) ? 1 : 0,
                (gray_byte & 0x20) ? 1 : 0,
                (gray_byte & 0x40) ? 1 : 0,
                (gray_byte & 0x80) ? 1 : 0);
            LOG_INFO("Grey byte: %s", buffer);
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
            snprintf(buffer, sizeof(buffer), "err:%.2f", gray_error);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
        }
        else if (mode == 2)   // 速度+距离模式
        {
            float speed_a = Control_Speed_GetActualSpeed(CONTROL_SPEED1_OBJECT);
            float speed_b = Control_Speed_GetActualSpeed(CONTROL_SPEED2_OBJECT);
            snprintf(buffer, sizeof(buffer), "L:%.2f R:%.2f", speed_a, speed_b);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "Dist:%u mm", distance_mm);
            OLED_ShowString(0, 24, (uint8_t*)buffer, 16, 1);
        }
        else if (mode == 3)   // 题1：巡线
        {
            OLED_ShowString(0, 0, (uint8_t*)"TASK1", 16, 1);
            OLED_ShowString(0, 20, (uint8_t*)"XunXian", 16, 1);
            OLED_ShowString(0, 48, (uint8_t*)"KEY4:Start", 8, 1);
        }
        else if (mode == 4)   // 题2：固定距离跟随
        {
            OLED_ShowString(0, 0, (uint8_t*)"TASK2", 16, 1);
            OLED_ShowString(0, 20, (uint8_t*)"FixedDist", 16, 1);
            OLED_ShowString(0, 48, (uint8_t*)"KEY4:Start", 8, 1);
        }
        else if (mode == 5)   // 题3：变速跟随
        {
            OLED_ShowString(0, 0, (uint8_t*)"TASK3", 16, 1);
            OLED_ShowString(0, 20, (uint8_t*)"VarDist", 16, 1);
            OLED_ShowString(0, 48, (uint8_t*)"KEY4:Start", 8, 1);
        }
        else if (mode == 6)   // 题4：超车
        {
            OLED_ShowString(0, 0, (uint8_t*)"TASK4", 16, 1);
            OLED_ShowString(0, 20, (uint8_t*)"Overtake", 16, 1);
            OLED_ShowString(0, 48, (uint8_t*)"KEY4:Start", 8, 1);
        }

        OLED_Refresh();
        PT_WAIT_TICK(pt, 100);
    }
    PT_END(pt);
}
