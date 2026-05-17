#ifndef __CONTROL_SPEED_H__ 
#define __CONTROL_SPEED_H__

#include "./PID_Node/PID_Node.h"
#include "./Motor/Motor_AT4950.h"

typedef enum
{
    CONTROL_SPEED1_OBJECT = 0u,
    CONTROL_SPEED2_OBJECT
}Control_Speed_Select_Object_Option;

#define WHEEL_CIRCUMFERENCE 0.1508f   // 车轮周长 m
#define EFFECTIVE_PPR 1040            // 每转脉冲数（根据编码器配置）


// void Control_Init(MotorAT4950* m1, MotorAT4950* m2);
// void Control_Speed_SetSetPoint(Control_Speed_Select_Object_Option option, float speed);
// void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);
// void Control_Speed_SetSetPoint(Control_Speed_Select_Object_Option option, float speed);
// void Control_Speed_SetPID(Control_Speed_Select_Object_Option option, float kp, float ki, float kd);
// PID_Node* Control_Speed_PID_Hander(Control_Speed_Select_Object_Option option);
/* 初始化速度环（传入电机句柄） */
void Control_Init(MotorAT4950* m1, MotorAT4950* m2);
/* 设置目标速度 (m/s) */
void Control_Speed_SetSetPoint(Control_Speed_Select_Object_Option option, float speed);
/* 设置 PID 参数 */
void Control_Speed_SetPID(Control_Speed_Select_Object_Option option, float kp, float ki, float kd);
/* 获取 PID 节点指针（可用于调试打印） */
PID_Node* Control_Speed_PID_Hander(Control_Speed_Select_Object_Option option);
/* 获取当前实际速度 (m/s) */
float Control_Speed_GetActualSpeed(Control_Speed_Select_Object_Option option);
/* 速度环更新任务（20ms 调用一次） */
// void Control_UpdateSpeedPID(float dt);
void Control_UpdateSpeedPID(int32_t diff_A, int32_t diff_B, float dt);
void UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);

void Control_SetGrayTarget(float gray_error);
float Control_Gray_Update(float gray_error, float dt);
void Control_SetBaseSpeed(float speed);

#endif
