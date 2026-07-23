#include "tasks.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "oled.h"
#include "mpu6050_user.h"
#include "grayscale.h"
#include "key_control.h"
#include "PID_Node.h"
#include "control.h"
#include "tim.h"

// ========== Protothread 实例定义 ==========
Protothread_t task1_pt;
Protothread_t task2_pt;
Protothread_t SerialTask_pt;
Protothread_t oled_pt;

// ========== Tasks_Init ==========
void Tasks_Init(void)
{
    PT_INIT(&task1_pt);
    PT_INIT(&task2_pt);
    PT_INIT(&SerialTask_pt);
    PT_INIT(&oled_pt);
}

// ========== task1：LED闪烁 500ms ==========
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

// ========== task2：LED闪烁 1000ms ==========
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

// ========== SerialTask：按当前级联模式输出调试数据 ==========
void SerialTask(Protothread_t* pt)
{
    PT_BEGIN(pt);

    // 首次进入时打印 SerialPlot 协议头
    printf("#Cascade_Debug_Mode\n");
    LOG_INFO("SerialTask started, cascade stage = %d", current_cascade_stage);

    while(1)
    {
        // 仅在非IDLE模式下输出
        if (current_cascade_stage != CASCADE_IDLE)
        {
            uint16_t pwm1 = (uint16_t)__HAL_TIM_GetCompare(&htim1, TIM_CHANNEL_1);
            uint16_t pwm2 = (uint16_t)__HAL_TIM_GetCompare(&htim1, TIM_CHANNEL_2);

            switch (current_cascade_stage)
            {
                // ---- 纯速度环：speedA, speedB, pwm1, pwm2 ----
                case CASCADE_SPEED_ONLY:
                    printf("%.2f,%.2f,%d,%d\n",
                           Actual_Speed_A,
                           Actual_Speed_B,
                           (int)pwm1,
                           (int)pwm2);
                    break;

                // ---- 纯角度环：yaw, target, err, output, pwm1, pwm2 ----
                case CASCADE_ANGLE_ONLY:
                    printf("yaw=%.1f,target=%.1f,err=%.1f,out=%.2f,pwm1=%d,pwm2=%d\n",
                           mpu->yaw,
                           debug_target_angle,
                           pidAngle.data.error,
                           pidAngle.output,
                           (int)pwm1,
                           (int)pwm2);
                    break;

                // ---- 速度+灰度串级：speedA, speedB, pwm1, pwm2, gray_err, gray_out ----
                case CASCADE_SPEED_GRAY:
                    printf("%.2f,%.2f,%d,%d,%.2f,%.2f\n",
                           Actual_Speed_A,
                           Actual_Speed_B,
                           (int)pwm1,
                           (int)pwm2,
                           gray_error,
                           pidGrayscale.output);
                    break;

                // ---- 速度+角度串级：speedA, speedB, pwm1, pwm2, yaw, target, angle_out ----
                case CASCADE_SPEED_ANGLE:
                    printf("%.2f,%.2f,%d,%d,%.1f,%.1f,%.2f\n",
                           Actual_Speed_A,
                           Actual_Speed_B,
                           (int)pwm1,
                           (int)pwm2,
                           mpu->yaw,
                           debug_target_angle,
                           pidAngle.output);
                    break;

                default:
                    break;
            }
        }

        PT_WAIT_TICK(pt, 100);  // 每100ms输出一次
    }
    PT_END(pt);
}

// ========== OLED_Task：显示模式0-7 ==========
void OLED_Task(Protothread_t* pt)
{
    char buffer[20] = {0};
    uint8_t mode = 0;
    PT_BEGIN(pt);
    while (1)
    {
        mode = KeyControl_GetDisplayMode();

        // ===== Mode 0：IMU姿态 =====
        if (mode == 0)
        {
            snprintf(buffer, sizeof(buffer), "Y:%.1f", mpu->yaw);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "R:%.1f", mpu->roll);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "P:%.1f", mpu->pitch);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 1：灰度传感器 =====
        else if (mode == 1)
        {
            OLED_ShowString(0, 0, (uint8_t*)"gray:", 16, 1);
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

        // ===== Mode 2：速度显示 =====
        else if (mode == 2)
        {
            snprintf(buffer, sizeof(buffer), "val_A:%.2f", Actual_Speed_A);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_B:%.2f", Actual_Speed_B);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "A_T:%.2f", pidMotor1Speed.setpoint);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "B_T:%.2f", pidMotor2Speed.setpoint);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 3：纯速度环 SPEED ONLY =====
        else if (mode == 3)
        {
            OLED_ShowString(0, 0, (uint8_t*)"SPEED ONLY", 16, 1);

            if (current_cascade_stage == CASCADE_SPEED_ONLY) {
                OLED_ShowString(0, 16, (uint8_t*)"** RUNNING **", 16, 1);
            } else {
                OLED_ShowString(0, 16, (uint8_t*)"Press Key2", 16, 1);
            }

            snprintf(buffer, sizeof(buffer), "Kp:%.0f Ki:%.0f",
                     pidMotor1Speed.parameters.kp,
                     pidMotor1Speed.parameters.ki);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);

            snprintf(buffer, sizeof(buffer), "A:%.2f B:%.2f",
                     Actual_Speed_A, Actual_Speed_B);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 4：纯角度环 ANGLE ONLY =====
        else if (mode == 4)
        {
            OLED_ShowString(0, 0, (uint8_t*)"ANGLE ONLY", 16, 1);

            if (current_cascade_stage == CASCADE_ANGLE_ONLY) {
                OLED_ShowString(0, 16, (uint8_t*)"** RUNNING **", 16, 1);
            } else {
                OLED_ShowString(0, 16, (uint8_t*)"Press Key2", 16, 1);
            }

            snprintf(buffer, sizeof(buffer), "Kp:%.4f Kd:%.2f",
                     pidAngle.parameters.kp,
                     pidAngle.parameters.kd);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);

            snprintf(buffer, sizeof(buffer), "Y:%.1f T:%.1f",
                     mpu->yaw, debug_target_angle);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 5：速度+灰度串级 SPEED+GRAY =====
        else if (mode == 5)
        {
            OLED_ShowString(0, 0, (uint8_t*)"SPEED+GRAY", 16, 1);

            if (current_cascade_stage == CASCADE_SPEED_GRAY) {
                OLED_ShowString(0, 16, (uint8_t*)"** RUNNING **", 16, 1);
            } else {
                OLED_ShowString(0, 16, (uint8_t*)"Press Key2", 16, 1);
            }

            snprintf(buffer, sizeof(buffer), "Kp:%.3f Kd:%.2f",
                     pidGrayscale.parameters.kp,
                     pidGrayscale.parameters.kd);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);

            snprintf(buffer, sizeof(buffer), "err:%.2f out:%.2f",
                     gray_error, pidGrayscale.output);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 6：速度+角度串级 SPEED+ANGLE =====
        else if (mode == 6)
        {
            OLED_ShowString(0, 0, (uint8_t*)"SPEED+ANGLE", 16, 1);

            if (current_cascade_stage == CASCADE_SPEED_ANGLE) {
                OLED_ShowString(0, 16, (uint8_t*)"** RUNNING **", 16, 1);
            } else {
                OLED_ShowString(0, 16, (uint8_t*)"Press Key2", 16, 1);
            }

            snprintf(buffer, sizeof(buffer), "Kp:%.4f Kd:%.2f",
                     pidAngle.parameters.kp,
                     pidAngle.parameters.kd);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);

            snprintf(buffer, sizeof(buffer), "err:%.1f out:%.2f",
                     pidAngle.data.error, pidAngle.output);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }

        // ===== Mode 7：IDLE / STOP =====
        else if (mode == 7)
        {
            OLED_ShowString(0, 0, (uint8_t*)"IDLE / STOP", 16, 1);
            OLED_ShowString(0, 16, (uint8_t*)"Motor Stopped", 16, 1);
            OLED_ShowString(0, 32, (uint8_t*)"Press Key2", 16, 1);
            OLED_ShowString(0, 48, (uint8_t*)"to STOP all", 16, 1);
        }

        OLED_Refresh();
        PT_WAIT_TICK(pt, 200);  // OLED 刷新率 5Hz
    }
    PT_END(pt);
}
