#ifndef __SPEED_CONTROL_H
#define __SPEED_CONTROL_H

#include "main.h"
#include "tim.h"
#include "PID_Node.h"
#include <stdint.h>
#include <stdbool.h>
#include <Log.h>
#include "Motor_AT4950.h"
#include "mpu6050_user.h" 
// ==================== 车轮参数 (请根据实际测量修改) ====================
#define WHEEL_DIAMETER_MM       45.0f       // 车轮直径 (mm)
#define ENCODER_PPR             1040         // 编码器线数 (每转脉冲数)

#define WHEEL_CIRCUMFERENCE_MM  0.1508f
#define SPEED_SAMPLE_TIME_MS    20.0f        // 速度采样周期 (ms)
#define MAX_SPEED_MM_S          800.0f      // 最大速度限制 (mm/s)

// 速度转换系数: speed (mm/s) = delta_count * (π*D * 1000) / (PPR * T)
#define SPEED_COEFF             (WHEEL_CIRCUMFERENCE_MM) / (ENCODER_PPR * SPEED_SAMPLE_TIME_MS)
// 小车几何参数（用于差速分解）
#define WHEEL_TRACK_M           0.15f       // 轮距（米）
// ==================== 角度环控制接口 ====================

// 外部可访问的角度环PID节点（用于设置参数、显示等）
extern PID_Node yaw_pid;

/**
 * @brief 初始化角度环PID控制器
 */
void AngleControl_Init(void);

/**
 * @brief 执行一次角度环串级控制（应周期性调用）
 * @param base_speed     基础线速度 (m/s)
 * @param target_yaw_deg 目标偏航角 (度)
 * @param dt             控制周期 (秒)，例如0.02s
 */
void AngleControl_Update(float base_speed, float target_yaw_deg, float dt);

/**
 * @brief 设置目标偏航角（度）
 */
void AngleControl_SetTarget(float yaw_deg);
#endif 
