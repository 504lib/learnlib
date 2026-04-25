#include "./angle_control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include <math.h>
//PID_Node angle_pid;  // 角度环 PID 节点

//float base_speed = 0.3f;   // 基础前进速度（mm/s 或 编码器单位，取决于你的速度环设定值单位）

//float target_yaw = 0.0f;  // 目标角度（度）

//float steer_correction = 0.0f;  // 角度环输出（转向量，单位与速度设定值一致）
//void AngleControl_Init(void) {
//    // 初始化角度 PID 节点
//    PID_Node_Init(&angle_pid, "Angle_PID", 2.5f, 0.01f, 0.5f);
//    
//    // 设置输出限幅（转向修正量最大值，对应速度设定值范围）
//    PID_Node_SetMaxOutput(&angle_pid,  0.3f);
//    PID_Node_SetMinOutput(&angle_pid, -0.3f);
//    
//    // 设置积分限幅
//    PID_Node_SetMaxIntegral(&angle_pid, 0.2f);
//    
//    // 设置死区（角度误差小于此值不调整）
//    PID_Node_SetDeadband(&angle_pid, 1.0f);
//}