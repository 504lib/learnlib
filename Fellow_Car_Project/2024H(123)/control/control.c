#include "control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "mpu6050_user.h"
#include "state_machine.h"

// ============ 全局变量 ============
MPU6050_Data_t* mpu_control = NULL;   // MPU6050 数据指针

float Actual_Speed_A = 0.0f;
float Actual_Speed_B = 0.0f;
uint8_t gray_byte = 0;
float gray_error = 0.0f;

// PID 实例
PID_Node pidMotor1Speed;
PID_Node pidMotor2Speed;
PID_Node pidGrayscale;
PID_Node pidAngle;

// 控制参数（可通过串口修改）
static float base_speed = 0.15f;          // 基础速度 m/s
//static float gray_to_speed_gain = 0.5f;   // 灰度输出到速度差的系数

// 电机硬件句柄（从 main.c 引用）
extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

// 车轮物理参数
#define WHEEL_CIRCUMFERENCE 0.1508f   // 车轮周长 m
#define EFFECTIVE_PPR 1040            // 每转脉冲数（根据编码器配置）

static float AngleErrorCallback(float setpoint, float measured)
{
    float err = setpoint - measured;
    if (err > 180.0f) err -= 360.0f;
    if (err < -180.0f) err += 360.0f;
    return err;
}

// ============ 函数实现 ============
void Control_Init(void)
{
	mpu_control = MPU6050_GetHandle();   // 获取 MPU6050 数据句柄
    // 初始化灰度 PID
    PID_Node_Init(&pidGrayscale, "Grayscale", 0.045f, 0.0005f, 0.5f);
    // 初始化速度 PID
    PID_Node_Init(&pidMotor1Speed, "M1_Speed", 2000.0f, 6.0f, 0.0f);
    PID_Node_Init(&pidMotor2Speed, "M2_Speed", 2000.0f, 6.0f, 0.0f);
	// 初始化角度 PID
	PID_Node_Init(&pidAngle, "Angle", 0.025f, 0.00f, 0.02f);
	
	// ===== 新增：设置自定义误差计算回调 =====
    PID_Custom_Functions custom;
    custom.custom_error_calculation = AngleErrorCallback;
    PID_Node_SetCustomCallback(&pidAngle, custom);
	
    PID_Limit limit = {
		.input_max = 5.0f,
		.input_min = 0.0f,
		.integral_max = 5.0f,
        .output_max = 1000.0f,
        .output_min = -1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.05f
	};
	// 灰度环限制
	PID_Limit gray_limit = {
		.output_max = 0.6f,
		.output_min = -0.6f,
		.integral_max = 2.0f,       // 积分限幅
		.deadband = 0.3f,           // 灰度误差死区
		.setpoint_min = -1000.0f,   // 灰度误差范围
		.setpoint_max = 1000.0f,
		.input_min = -1000.0f,
		.input_max = 1000.0f,
		.derivative_max = 20.0f
	};
	PID_Node_SetLimit(&pidGrayscale, gray_limit);
	PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);
    // 灰度环限制
//    limit.output_max = 1000.0f;
//    limit.output_min = -1000.0f;
//    limit.integral_max = 2.0f;
//    limit.deadband = 0.3f;
//    PID_Node_SetLimit(&pidGrayscale, limit);
//    PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);
	
    // 速度环限制
	// limit.input_max = 5.0f;
	// limit.input_min = 0.0f;
	// limit.setpoint_max = 5.0f;
	// limit.setpoint_min = 0.0f;
    // limit.output_max = 1000.0f;
    // limit.output_min = -1000.0f;
    // limit.integral_max = 1000.0f;
    // limit.derivative_max = 20.0f;
    // limit.deadband = 0.0f;
    PID_Node_SetLimit(&pidMotor1Speed, (PID_Limit){
        .setpoint_max = 5.0f,
        .setpoint_min = 0.0f,
        .input_max = 5.0f,
        .input_min = 0.0f,
        .output_max = 1000.0f,
        .output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
    });
    PID_Node_SetLimit(&pidMotor2Speed, (PID_Limit){
	    .setpoint_max = 5.0f,
        .setpoint_min = 0.0f,
        .input_max = 5.0f,
        .input_min = 0.0f,
        .output_max = 1000.0f,
        .output_min = -1000.0f,
        .integral_max = 1000.0f,
        .derivative_max = 20.0f,
        .deadband = 0.0f
	});
	
	PID_Limit angle_limit = {
		.setpoint_min = -180.0f, 
		.setpoint_max = 180.0f,
		.input_min = -180.0f, 
		.input_max = 180.0f,
		.output_max = 0.2f,      // 最大转向补偿速度差（m/s），根据base_speed调整
		.output_min = -0.2f,
		.integral_max = 20.0f,
		.derivative_max = 20.0f,
		.deadband = 1.0f          
	};
    PID_Node_SetSetpoint(&pidAngle, 0.0f);   // 目标角度为0度（保持当前航向）
	PID_Node_SetLimit(&pidAngle, angle_limit);
	PID_Node_SetIntegralAttenuationKp(&pidAngle, 0.98f);
}

// 根据编码器差值更新实际速度（在中断中调用）
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt > 0.0f) {
		float dt_ms = dt / 1000.0f;
        Actual_Speed_A = -(diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
        Actual_Speed_B = (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
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
    // 1. 执行状态机
    SM_Update();

    // 2. 根据当前动作选择控制模式
    float target_A = 0, target_B = 0;

    switch (sm.current_action)
    {
        case ACT_STRAIGHT:
        {
            // 直行模式：使用角度 PID 保持航向
            // 目标角度在进入 ACT_STRAIGHT 时已锁定在 sm.target_angle
            float current_yaw = mpu_control->yaw;
//            float angle_error = sm.target_angle - current_yaw;
//            // 角度归一化到 [-180, 180]
//            if (angle_error > 180.0f) angle_error -= 360.0f;
//            if (angle_error < -180.0f) angle_error += 360.0f;
			PID_Node_SetSetpoint(&pidAngle,sm.target_angle);
//            PID_Node_UpdateMeasurement(&pidAngle, current_yaw);
//			float error = sm.target_angle - current_yaw;
//			while(error > 180.0f) error -= 360.0f;
//			while(error < -180.0f) error += 360.0f;
			PID_Node_SetSetpoint(&pidAngle, sm.target_angle);
			PID_Node_UpdateMeasurement(&pidAngle, current_yaw);   // 正常更新测量值
			PID_ExecuteNode(&pidAngle, dt);                       // PID内部会调用回调计算误差			
//			pidAngle.data.error = error;
            PID_ExecuteNode(&pidAngle, dt);
            float steering = pidAngle.output;   // 范围由 limit.output_max 限制，单位 m/s

            target_A = base_speed - steering;
            target_B = base_speed + steering;
            break;
        }

        case ACT_TRACKING:
        {
            // 循迹模式：使用灰度 PID
            PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
            PID_ExecuteNode(&pidGrayscale, dt);
            float gray_output = pidGrayscale.output;          // 范围 -1000~1000
            float speed_diff = gray_output;
            // 限制速度差最大值，防止过冲
//            if (speed_diff > 0.6f) speed_diff = 0.6f;
//            if (speed_diff < -0.6f) speed_diff = -0.6f;

            target_A = base_speed - speed_diff;
            target_B = base_speed + speed_diff;
            break;
        }

        case ACT_STOP:
        case ACT_IDLE:
        default:
            target_A = 0;
            target_B = 0;
            break;
    }

    // 可选：限制目标速度非负（根据你的电机是否允许反转）
    // if (target_A < 0) target_A = 0;
    // if (target_B < 0) target_B = 0;

    // 3. 速度闭环
    PID_Node_SetSetpoint(&pidMotor1Speed, target_A);
    PID_Node_SetSetpoint(&pidMotor2Speed, target_B);
    PID_Node_UpdateMeasurement(&pidMotor1Speed, Actual_Speed_A);
    PID_Node_UpdateMeasurement(&pidMotor2Speed, Actual_Speed_B);
    PID_ExecuteNode(&pidMotor1Speed, dt);
    PID_ExecuteNode(&pidMotor2Speed, dt);

    // 4. 输出 PWM
    Motor_setSpeed(&motor1, (int16_t)pidMotor1Speed.output);
    Motor_setSpeed(&motor2, (int16_t)pidMotor2Speed.output);

    // 调试打印（可选）
    // static uint32_t last_time = 0;
    // if (HAL_GetTick() - last_time > 200) {
    //     printf("action=%d, yaw=%.1f, steer=%.2f, tA=%.2f, tB=%.2f\n",
    //            sm.current_action, mpu_control->yaw, 
    //            (sm.current_action == ACT_STRAIGHT) ? pidAngle.output : 0,
    //            target_A, target_B);
    //     last_time = HAL_GetTick();
    // }
}
//void Control_Update(float dt)
//{
//    // 1. 读取yaw
//    float current_yaw = mpu_control->yaw;
//    
//    // 2. 角度环：计算转向补偿 steering (单位: m/s)
//    //    注意：目标角度已在初始化时设为0，也可以动态修改
//    PID_Node_UpdateMeasurement(&pidAngle, current_yaw);
//    PID_ExecuteNode(&pidAngle, dt);
//    float steering = pidAngle.output;   // 范围由 limit.output_max 限制，如 ±0.3
//    
//    // 3. 计算左右轮目标速度（基础速度 ± 转向补偿）
//    float target_A = base_speed - steering;
//    float target_B = base_speed + steering;
//    
//    // 可选：限制目标速度范围（防止过大）
//    // if (target_A > 1.0f) target_A = 1.0f;
//    // if (target_A < -0.5f) target_A = -0.5f;
//    // if (target_B > 1.0f) target_B = 1.0f;
//    // if (target_B < -0.5f) target_B = -0.5f;
//    
//    // 4. 速度闭环
//    PID_Node_SetSetpoint(&pidMotor1Speed, target_A);
//    PID_Node_SetSetpoint(&pidMotor2Speed, target_B);
//    PID_Node_UpdateMeasurement(&pidMotor1Speed, Actual_Speed_A);
//    PID_Node_UpdateMeasurement(&pidMotor2Speed, Actual_Speed_B);
//    PID_ExecuteNode(&pidMotor1Speed, dt);
//    PID_ExecuteNode(&pidMotor2Speed, dt);
//    
//    // 5. 输出 PWM
//    Motor_setSpeed(&motor1, (int16_t)pidMotor1Speed.output);
//    Motor_setSpeed(&motor2, (int16_t)pidMotor2Speed.output);

//    static uint32_t last_time = 0;
//    if (HAL_GetTick() - last_time > 200) {
//        printf("yaw=%.1f, err=%.1f, steer=%.2f, tA=%.2f, tB=%.2f\n", 
//               current_yaw, pidAngle.data.error, steering, target_A, target_B);
//        last_time = HAL_GetTick();
//    }
//}
