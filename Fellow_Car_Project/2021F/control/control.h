#ifndef __CONTROL_H
#define __CONTROL_H

#include "PID_Node.h"
#include "mpu6050_user.h" 

#define CTRL_MODE_IDLE     0
#define CTRL_MODE_STRAIGHT 1
#define CTRL_MODE_TRACKING 2
#define CTRL_MODE_STOP     3
#define CTRL_MODE_MANUAL   4

bool Control_IsRedMarkerCentered(void);
bool Control_IsAtStart(void);   // 判断是否回到起点（全白）
bool Control_IsCrossDetected(uint8_t threshold);  //判断分叉口

extern uint8_t ctrl_mode;
extern float   target_yaw;
float AngleDiff(float current, float target);    // 角度差函数

// 速度反馈全局变量
extern float Actual_Speed_A;
extern float Actual_Speed_B;

// 灰度传感器数据
extern volatile uint8_t gray_byte;
extern float gray_error;

// PID 实例声明
extern PID_Node pidMotor1Speed;
extern PID_Node pidMotor2Speed;
extern PID_Node pidAngularVelocity;
//extern PID_Node pidYaw;
extern PID_Node pidGrayscale;
extern PID_Node pidAngle;

// MPU6050 数据句柄
extern MPU6050_Data_t* mpu;

// 可选：级联链表
extern PID_Link cascadeLink;   // 如果使用级联，可以在这里声明

//// 控制模式枚举（与 f_sm.h 保持一致）
//typedef enum {
//    CTRL_MODE_IDLE = 0,
//    CTRL_MODE_STRAIGHT,
//    CTRL_MODE_TRACKING,
//    CTRL_MODE_STOP
//} CtrlMode_t;

// 函数声明
void Control_Init(void);
void Control_Update(float dt);
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);
void Control_SetManualSpeeds(float left, float right);	
bool Control_IsAtEnd(void);
#endif
