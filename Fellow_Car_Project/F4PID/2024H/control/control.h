#ifndef __CONTROL_H
#define __CONTROL_H

#include "PID_Node.h"
#include "mpu6050_user.h"

// ========== 级联调试阶段枚举 ==========
typedef enum {
    CASCADE_IDLE = 0,            // 空闲，不计算PID，电机不转
    CASCADE_SPEED_ONLY = 1,      // 纯速度环
    CASCADE_ANGLE_ONLY = 2,      // 纯角度环（原地转向）
    CASCADE_SPEED_GRAY = 3,      // 速度环+灰度环 串级
    CASCADE_SPEED_ANGLE = 4,     // 速度环+角度环 串级
} CascadeDebugStage;

extern CascadeDebugStage current_cascade_stage;

// 调试用可调参数（可通过串口修改）
extern float debug_base_speed;     // 基础速度 (m/s)，默认 0.15
extern float debug_target_angle;   // 目标角度 (degrees)，默认 0

// 速度反馈全局变量
extern float Actual_Speed_A;
extern float Actual_Speed_B;

// 灰度传感器数据
extern uint8_t gray_byte;
extern float gray_error;

// PID 实例声明
extern PID_Node pidMotor1Speed;
extern PID_Node pidMotor2Speed;
extern PID_Node pidGrayscale;
extern PID_Node pidAngle;

// MPU6050 数据句柄
extern MPU6050_Data_t* mpu;

// 函数声明
void Control_Init(void);
void Control_Update(float dt);
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);

#endif
