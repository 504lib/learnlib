#ifndef __CONTROL_H
#define __CONTROL_H

#include "PID_Node.h"
#include "mpu6050_user.h" 

// 速度反馈全局变量
extern float Actual_Speed_A;
extern float Actual_Speed_B;

// 灰度传感器数据
extern uint8_t gray_byte;
extern float gray_error;

// PID 实例声明
extern PID_Node pidMotor1Speed;
extern PID_Node pidMotor2Speed;
extern PID_Node pidAngularVelocity;
//extern PID_Node pidYaw;
//extern PID_Node pidGrayscale;

// MPU6050 数据句柄
extern MPU6050_Data_t* mpu;

// 可选：级联链表
extern PID_Link cascadeLink;   // 如果使用级联，可以在这里声明

// 控制状态（如需要）
// extern CascadeDebugStage current_cascade_stage; // 根据需要定义

// 函数声明
void Control_Init(void);
void Control_Update(float dt);
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);
void Control_Update(float dt);
	
#endif
