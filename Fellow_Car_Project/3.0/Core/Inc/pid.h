#ifndef __PID_H
#define __PID_H

#include <math.h>

//声明一个结构体类型
// pid.h
typedef struct {
    float target_val;    // 目标值
    float actual_val;    // 实际值
    float err;           // 当前误差
    float err_last;      // 上次误差
    float err_sum;       // 误差积分项
    float Kp, Ki, Kd;    // PID系数
    float max_integral;  // 新增：积分限幅
    float max_derivative; // 新增：微分限幅
    float max_output;    // 新增：输出限幅
    float min_output;
    float last_output;     // <--- 新增：用于限制输出变化速度
    float max_delta;       // <--- 新增：每次最大变化量
    float deadband;
} tPid;

// PID结构体实例声明
extern tPid pidMotor1Speed;
extern tPid pidMotor2Speed;
extern tPid pidMpu6050;        // 可以保持原样
extern tPid pid_yaw;           // 角度环
extern tPid pid_grayscale;     // 灰度环
extern tPid pidAngularVelocity; // 新增：角速度环

//声明函数
float P_realize(tPid * pid,float actual_val);
void PID_init(void);
float PI_realize(tPid * pid,float actual_val);
float PID_realize(tPid * pid,float actual_val);
float PID_Angle(tPid *pid, float actual_yaw, float target_yaw);
#endif
