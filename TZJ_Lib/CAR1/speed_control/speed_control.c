#include <speed_control.h>
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include <math.h>
// 角度环PID节点（全局）
PID_Node yaw_pid;
MPU6050_Data_t* mpu_hander = NULL;
// 角度环输出限幅：角速度 ±180 deg/s → ±3.14 rad/s
#define MAX_YAW_RATE_RAD_S  3.14f

// 角度环PID限幅参数
static PID_Limit yaw_pid_limit = {
    .setpoint_min = -180.0f, .setpoint_max = 180.0f,
    .input_min    = -180.0f, .input_max    = 180.0f,
    .output_min   = -0.2f, .output_max = 0.2f,
    .integral_max = 0.0f, .derivative_max = 0.10f,
    .deadband     = 1.0f      // 角度误差小于1度时视为0
};

// 外部电机对象（来自main.c）
extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

// 外部速度环PID节点（来自main.c）
extern PID_Node left_pid;
extern PID_Node right_pid;

// ==================== 角度环初始化 ====================
void AngleControl_Init(void) {
    PID_Node_Init(&yaw_pid, "Yaw_PID", 0.01f, 0.0f, 0.005f);  // 参数需实际调试
    PID_Node_SetLimit(&yaw_pid, yaw_pid_limit);
    PID_Node_SetSetpoint(&yaw_pid, 0.0f);   // 默认目标0度
	mpu_hander = MPU6050_GetHandle();
}

void AngleControl_SetTarget(float yaw_deg) {
    PID_Node_SetSetpoint(&yaw_pid, yaw_deg);
}

// ==================== 角度环更新 ====================
void AngleControl_Update(float base_speed, float target_yaw_deg, float dt) {
    
    
    // 1. 更新测量值（当前偏航角）
    PID_Node_UpdateMeasurement(&yaw_pid, mpu_hander->yaw);
    
    // 2. 设置目标角度
    PID_Node_SetSetpoint(&yaw_pid, target_yaw_deg);
    
    // 3. 执行角度环PID
    PID_ExecuteNode(&yaw_pid, dt);
    
    // 4. 获取目标偏航角速度（rad/s）
    float target_yaw_rate = yaw_pid.output;
    
    // 5. 差速分解：计算左右轮目标速度（m/s）
//    float half_track = WHEEL_TRACK_M / 2.0f;
    float left_speed  = base_speed + target_yaw_rate;
    float right_speed = base_speed - target_yaw_rate;
    
    // 6. 更新速度环设定值
    PID_Node_SetSetpoint(&left_pid,  left_speed);
    PID_Node_SetSetpoint(&right_pid, right_speed);
}
