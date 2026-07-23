#include "control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "mpu6050_user.h"
#include "Log.h"

// ============ 全局变量 ============
MPU6050_Data_t* mpu_control = NULL;   // MPU6050 数据指针

// 级联调试状态
CascadeDebugStage current_cascade_stage = CASCADE_IDLE;

// 调试参数（可通过串口修改）
float debug_base_speed = 0.15f;       // 基础速度 m/s
float debug_target_angle = 0.0f;      // 目标角度 degrees

// 路程累积
volatile float total_distance_traveled = 0.0f;

float Actual_Speed_A = 0.0f;
float Actual_Speed_B = 0.0f;
uint8_t gray_byte = 0;
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

// ============ 角度误差回调（归一化到 [-180, 180]）============
static float AngleErrorCallback(float setpoint, float measured)
{
    float err = setpoint - measured;
    if (err > 180.0f) err -= 360.0f;
    if (err < -180.0f) err += 360.0f;
    return err;
}

// ============ Control_Init ============
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

    // 角度环自定义误差回调
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

    // 速度环限制
    PID_Node_SetLimit(&pidMotor1Speed, (PID_Limit){
        .setpoint_max = 5.0f,
        .setpoint_min = -5.0f,
        .input_max = 5.0f,
        .input_min = -5.0f,
        .output_max = 1000.0f,
        .output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
    });
    PID_Node_SetLimit(&pidMotor2Speed, (PID_Limit){
        .setpoint_max = 5.0f,
        .setpoint_min = -5.0f,
        .input_max = 5.0f,
        .input_min = -5.0f,
        .output_max = 1000.0f,
        .output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
    });

    // 角度环限制
    PID_Limit angle_limit = {
        .setpoint_min = -180.0f,
        .setpoint_max = 180.0f,
        .input_min = -180.0f,
        .input_max = 180.0f,
        .output_max = 0.2f,
        .output_min = -0.2f,
        .integral_max = 20.0f,
        .derivative_max = 20.0f,
        .deadband = 1.0f
    };
    PID_Node_SetSetpoint(&pidAngle, 0.0f);
    PID_Node_SetLimit(&pidAngle, angle_limit);
    PID_Node_SetIntegralAttenuationKp(&pidAngle, 0.98f);
}

// ============ Control_UpdateSpeedFeedback ============
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt > 0.0f) {
        float dt_ms = dt / 1000.0f;
        Actual_Speed_A = -(diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
        Actual_Speed_B = (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
    }

    // 路程累积
    float avg_diff = (fabsf((float)diff_A) + fabsf((float)diff_B)) / 2.0f;
    float distance_this_cycle = (avg_diff * WHEEL_CIRCUMFERENCE) / EFFECTIVE_PPR;
    total_distance_traveled += distance_this_cycle;

    // 低通滤波
    static float filtered_A = 0, filtered_B = 0;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;
}

// ============ Control_Update（核心：4种级联调试模式）============
void Control_Update(float dt)
{
    // 检测模式切换（用于自动捕获当前yaw、复位积分等）
    static CascadeDebugStage last_stage = CASCADE_IDLE;
    bool just_entered = (current_cascade_stage != last_stage);

    if (just_entered) {
        // 进入角度相关模式时，自动捕获当前yaw作为目标
        if (current_cascade_stage == CASCADE_ANGLE_ONLY ||
            current_cascade_stage == CASCADE_SPEED_ANGLE) {
            if (mpu_control != NULL) {
                debug_target_angle = mpu_control->yaw;
            }
        }
        // 复位所有PID积分
        PID_Node_ResetIntegral(&pidMotor1Speed);
        PID_Node_ResetIntegral(&pidMotor2Speed);
        PID_Node_ResetIntegral(&pidGrayscale);
        PID_Node_ResetIntegral(&pidAngle);
        total_distance_traveled = 0.0f;

        LOG_INFO("Cascade stage changed to: %d, target_angle=%.1f",
                 current_cascade_stage, debug_target_angle);
        last_stage = current_cascade_stage;
    }

    float target_A = 0.0f, target_B = 0.0f;

    switch (current_cascade_stage) {

    // ==================== IDLE：电机停转 ====================
    case CASCADE_IDLE:
        Motor_setSpeed(&motor1, 0);
        Motor_setSpeed(&motor2, 0);
        return;  // 不执行PID

    // ==================== 纯速度环 ====================
    case CASCADE_SPEED_ONLY:
        target_A = debug_base_speed;
        target_B = debug_base_speed;
        break;

    // ==================== 纯角度环（原地转向）====================
    case CASCADE_ANGLE_ONLY:
    {
        PID_Node_SetSetpoint(&pidAngle, debug_target_angle);
        PID_Node_UpdateMeasurement(&pidAngle, mpu_control->yaw);
        PID_ExecuteNode(&pidAngle, dt);
        float angle_out = pidAngle.output;
        // 角度输出直接作为差速，无前进分量
        target_A = -angle_out;
        target_B = angle_out;
        break;
    }

    // ==================== 速度环 + 灰度环 串级 ====================
    case CASCADE_SPEED_GRAY:
    {
        // 灰度环计算
        PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
        PID_ExecuteNode(&pidGrayscale, dt);
        float speed_diff = pidGrayscale.output;
        // 灰度输出转为左右轮速度差
        target_A = debug_base_speed - speed_diff;
        target_B = debug_base_speed + speed_diff;
        break;
    }

    // ==================== 速度环 + 角度环 串级 ====================
    case CASCADE_SPEED_ANGLE:
    {
        // 角度环计算（保持航向）
        PID_Node_SetSetpoint(&pidAngle, debug_target_angle);
        PID_Node_UpdateMeasurement(&pidAngle, mpu_control->yaw);
        PID_ExecuteNode(&pidAngle, dt);
        float steering = pidAngle.output;
        // 角度输出转为左右轮转向补偿
        target_A = debug_base_speed - steering;
        target_B = debug_base_speed + steering;
        break;
    }

    default:
        break;
    }

    // 速度闭环（所有非IDLE模式共用）
    PID_Node_SetSetpoint(&pidMotor1Speed, target_A);
    PID_Node_SetSetpoint(&pidMotor2Speed, target_B);
    PID_Node_UpdateMeasurement(&pidMotor1Speed, Actual_Speed_A);
    PID_Node_UpdateMeasurement(&pidMotor2Speed, Actual_Speed_B);
    PID_ExecuteNode(&pidMotor1Speed, dt);
    PID_ExecuteNode(&pidMotor2Speed, dt);

    // 输出 PWM
    Motor_setSpeed(&motor1, (int16_t)pidMotor1Speed.output);
    Motor_setSpeed(&motor2, (int16_t)pidMotor2Speed.output);
}
