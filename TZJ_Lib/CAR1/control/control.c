#include "control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "mpu6050_user.h"

// ============ 全局变量 ============
float Actual_Speed_A = 0.0f;
float Actual_Speed_B = 0.0f;
 uint8_t gray_byte = 0;
float gray_error = 0.0f;

// PID 实例

extern PID_Node pidGrayscale;   // 来自 control.c


// 控制参数（可通过串口修改）
static float base_speed = 0.15f;          // 基础速度 m/s
static float gray_to_speed_gain = 0.5f;   // 灰度输出到速度差的系数

// 电机硬件句柄（从 main.c 引用）
extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

// 车轮物理参数
#define WHEEL_CIRCUMFERENCE 0.1508f   // 车轮周长 m
#define EFFECTIVE_PPR 1040            // 每转脉冲数（根据编码器配置）

// ============ 函数实现 ============
void Control_Init(void)
{
    // 初始化灰度 PID
    PID_Node_Init(&pidGrayscale, "Grayscale", 0.045f, 0.0001f, 1.5f);

    PID_Limit limit;

    // 灰度环限制
    limit.output_max = 0.8f;
    limit.output_min = -0.8f;
    limit.integral_max = 2.0f;
    limit.deadband = 0.3f;
    PID_Node_SetLimit(&pidGrayscale, limit);
    PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);


}

// 根据编码器差值更新实际速度（在中断中调用）
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt > 0.0f) {
        Actual_Speed_A = (diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt);
        Actual_Speed_B = -(diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt);
    }

    // 简单低通滤波
    static float filtered_A = 0, filtered_B = 0;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;
}

// 主控制函数（在定时器中断中调用，传入 dt）
void Control_Update(float dt)
{
    // 1. 更新传感器数据（已经由中断中的 MPU6050_Update 和灰度读取完成）
    //    gray_error 已经通过 CalculateGrayError_Advanced 更新
    //    yaw、pitch、roll 已通过 MPU6050_Update 更新（若需要）

    // 2. 灰度 PID
    PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
    PID_ExecuteNode(&pidGrayscale, dt);
    float gray_output = pidGrayscale.output;   // 范围 -1000~1000

    // 3. 灰度输出转换为速度差
    float speed_diff = gray_output * gray_to_speed_gain;
    if (speed_diff > 0.3f) speed_diff = 0.3f;
    if (speed_diff < -0.3f) speed_diff = -0.3f;

    // 4. 计算左右电机目标速度
    float target_A = base_speed - speed_diff;
    float target_B = base_speed + speed_diff;





}