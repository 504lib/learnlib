#include "control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "mpu6050_user.h"
#include "Log.h"

// ============ 全局变量 ============
MPU6050_Data_t* mpu_control = NULL;

uint8_t ctrl_mode = 0;
float   target_yaw = 0.0f;

float manual_speed_A = 0.0f;
float manual_speed_B = 0.0f;

float Actual_Speed_A = 0.0f;
float Actual_Speed_B = 0.0f;
uint8_t volatile gray_byte = 0;
float gray_error = 0.0f;

// PID 实例
PID_Node pidMotor1Speed;
PID_Node pidMotor2Speed;
PID_Node pidGrayscale;
PID_Node pidAngle;

// 电机硬件句柄（从 main.c 引用）
extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

// 车轮物理参数
#define WHEEL_CIRCUMFERENCE 0.1508f   // 车轮周长 m
#define EFFECTIVE_PPR 1040            // 每转脉冲数

// ---------- 可配置速度参数 ----------
static float straight_speed = 0.15f;      // 直行速度 (m/s)
static float tracking_speed = 0.15f;      // 循迹速度 (m/s)

// ---------- 角度误差回调（保持 360° 环绕）----------
static float AngleErrorCallback(float setpoint, float measured)
{
    float err = setpoint - measured;
    if (err > 180.0f) err -= 360.0f;
    if (err < -180.0f) err += 360.0f;
    return err;
}

// ---------- 函数实现 ----------
void Control_Init(void)
{
    mpu_control = MPU6050_GetHandle();

    // 初始化灰度 PID
    PID_Node_Init(&pidGrayscale, "Grayscale", 0.045f, 0.0005f, 0.5f);
    // 初始化速度 PID
    PID_Node_Init(&pidMotor1Speed, "M1_Speed", 2000.0f, 6.0f, 0.0f);
    PID_Node_Init(&pidMotor2Speed, "M2_Speed", 2000.0f, 6.0f, 0.0f);
    // 初始化角度 PID
    PID_Node_Init(&pidAngle, "Angle", 0.005f, 0.00f, 0.04f);

    // 设置角度误差回调（处理 ±180° 环绕）
    PID_Custom_Functions custom;
    custom.custom_error_calculation = AngleErrorCallback;
    PID_Node_SetCustomCallback(&pidAngle, custom);

    // 灰度环限制
    PID_Limit gray_limit = {
        .output_max = 0.6f,
        .output_min = -0.6f,
        .integral_max = 2.0f,
        .deadband = 0.3f,
        .setpoint_min = -1000.0f,
        .setpoint_max = 1000.0f,
        .input_min = -1000.0f,
        .input_max = 1000.0f,
        .derivative_max = 20.0f
    };
    PID_Node_SetLimit(&pidGrayscale, gray_limit);
    PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);

    // 速度环限制（左右轮各自）
    PID_Node_SetLimit(&pidMotor1Speed, (PID_Limit){
        .setpoint_max = 5.0f, .setpoint_min = -5.0f,
        .input_max = 5.0f,    .input_min = -5.0f,
        .output_max = 1000.0f,.output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
    });
    PID_Node_SetLimit(&pidMotor2Speed, (PID_Limit){
        .setpoint_max = 5.0f, .setpoint_min = -5.0f,
        .input_max = 5.0f,    .input_min = -5.0f,
        .output_max = 1000.0f,.output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
    });

    // 角度环限制
    PID_Limit angle_limit = {
        .setpoint_min = -180.0f, .setpoint_max = 180.0f,
        .input_min = -180.0f,   .input_max = 180.0f,
        .output_max = 0.2f,     .output_min = -0.2f,
        .integral_max = 20.0f,
        .derivative_max = 20.0f,
        .deadband = 1.0f
    };
    PID_Node_SetSetpoint(&pidAngle, 0.0f);
    PID_Node_SetLimit(&pidAngle, angle_limit);
    PID_Node_SetIntegralAttenuationKp(&pidAngle, 0.98f);
}

// 编码器速度反馈（在中断中调用）
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt > 0.0f) {
        float dt_ms = dt / 1000.0f;
        Actual_Speed_A = -(diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
        Actual_Speed_B =  (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
    }

    // 简单低通滤波
    static float filtered_A = 0, filtered_B = 0;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;
}

// 主控制函数（周期性调用）
void Control_Update(float dt)
{
//    // 1. 驱动状态机（状态转移与条件判断）
//    FSM_App_Run();

    float target_A = 0.0f, target_B = 0.0f;

    // 2. 根据当前控制模式计算转向补偿
    switch (ctrl_mode) {
        case 1:
        {
            // 角度 PID：目标航向由状态机在进入直行时设定
            PID_Node_SetSetpoint(&pidAngle, target_yaw);
            PID_Node_UpdateMeasurement(&pidAngle, mpu_control->yaw);
            PID_ExecuteNode(&pidAngle, dt);
            float steering = pidAngle.output;

            target_A = straight_speed - steering;
            target_B = straight_speed + steering;
            break;
        }

        case 2:
        {
            // 灰度 PID：沿黑线行驶
            PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
            PID_ExecuteNode(&pidGrayscale, dt);
            float speed_diff = pidGrayscale.output;

            target_A = tracking_speed - speed_diff;
            target_B = tracking_speed + speed_diff;
            break;
        }
		case 4:   // CTRL_MODE_MANUAL
		{	
			target_A = manual_speed_A;
			target_B = manual_speed_B;
			break;
        }
		default:
            target_A = 0.0f;
            target_B = 0.0f;
            break;
    }

    // 3. 速度闭环
    PID_Node_SetSetpoint(&pidMotor1Speed, target_A);
    PID_Node_SetSetpoint(&pidMotor2Speed, target_B);
    PID_Node_UpdateMeasurement(&pidMotor1Speed, Actual_Speed_A);
    PID_Node_UpdateMeasurement(&pidMotor2Speed, Actual_Speed_B);
    PID_ExecuteNode(&pidMotor1Speed, dt);
    PID_ExecuteNode(&pidMotor2Speed, dt);

    // 4. PWM 输出
    Motor_setSpeed(&motor1, (int16_t)pidMotor1Speed.output);
    Motor_setSpeed(&motor2, (int16_t)pidMotor2Speed.output);
}

float AngleDiff(float current, float target) {
    float diff = target - current;
    if (diff > 180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    return diff;
}

void Control_SetManualSpeeds(float left, float right) {
    ctrl_mode = 4;   // 手动模式编号 4
    // 或者定义一个宏 CTRL_MODE_MANUAL 4
    // 也可以不用 ctrl_mode，直接在 Control_Update 的 manual 分支处理
    // 但为了统一，我们改用全局变量存储手动速度
    extern float manual_speed_A, manual_speed_B;
    manual_speed_A = left;
    manual_speed_B = right;
    PID_Node_SetSetpoint(&pidMotor1Speed, left);
    PID_Node_SetSetpoint(&pidMotor2Speed, right);
}

// 检查是否到达终点（红线消失）
bool Control_IsAtEnd(void) {
    return (gray_byte == 0xFF);   // 所有传感器全白表示终点
}

uint8_t Control_GrayByte_Windows_Filiter(size_t window_size)
{
    // 简单的窗口滤波器，减少噪声影响
    // 例如：要求连续3次读取到同样的值才更新 gray_byte
    static uint8_t last_gray = 0;
    static uint8_t count = 0;
    uint8_t current_gray = GPIOE->IDR & 0xFF;

    if (current_gray == last_gray) {
        count++;
        if (count >= window_size) {
            gray_byte = current_gray;
            count = 0; // 重置计数器
        }
    } else {
        last_gray = current_gray;
        count = 1; // 新值出现，重置计数器
    }
    return gray_byte;
}

/**
 * @brief 检测是否到达路口/分叉口
 *        约定：最长连续低电平（黑）传感器数量 >= 阈值
 * @param threshold 连续黑点数量阈值（建议4~6，根据赛道调整）
 */
bool Control_IsCrossDetected(uint8_t threshold) {
    uint8_t bits = gray_byte;
    int max_cont = 0, cur_cont = 0;
    for (int i = 0; i < 8; i++) {
        if ((bits & (1 << i)) == 0) {
            cur_cont++;
            if (cur_cont > max_cont) max_cont = cur_cont;
        } else {
            cur_cont = 0;
        }
    }
    return max_cont >= threshold;
}
