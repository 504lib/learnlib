#include "control.h"
#include "PID_Node.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "mpu6050_user.h"
#include "state_machine.h"

// ============ 全局变量 ============
MPU6050_Data_t* mpu_control = NULL;   // MPU6050 数据指针
// ============ 路程环   ============
volatile float total_distance_traveled = 0.0f; // 单位：米(m)

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

#define DISTANCE_TO_C 0.65f  // 从A点到C点的预估距离(m)，需要测量

// 添加结构体定义
typedef struct {
    float straight_max_speed;
    float straight_end_speed;
    float straight_ramp_step;
    float tracking_max_speed;
    float tracking_init_speed;
    float tracking_ramp_step;
} SpeedConfig_t;

// 添加配置表（索引对应 TaskType_t）
static const SpeedConfig_t speed_configs[] = {
//直线最高速度,直线末端速度,加速步长,循迹最高速度,循迹初始速度
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // TASK_NONE
    {0.15f, 0.15f, 0.02f, 0.0f, 0.0f, 0.01f}, // TASK_1
    {0.15f, 0.15f, 0.02f, 0.15f, 0.15f, 0.01f}, // TASK_2
    {0.15f, 0.15f, 0.02f, 0.15f, 0.15f, 0.01f}, // TASK_3
    {0.45f, 0.15f, 0.02f, 0.25f, 0.15f, 0.01f}  // TASK_4（你调试好的值）
};

// 添加获取配置函数
static SpeedConfig_t GetSpeedConfig(void) {
    uint8_t idx = (uint8_t)sm.current_task;
    if (idx >= 5) idx = 1; // fallback
    return speed_configs[idx];
}

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
	PID_Node_Init(&pidAngle, "Angle", 0.005f, 0.00f, 0.04f);
	
	// ===== 新增：设置自定义误差计算回调 =====
    PID_Custom_Functions custom;
    custom.custom_error_calculation = AngleErrorCallback;
    PID_Node_SetCustomCallback(&pidAngle, custom);
	
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
	
	// 路程累积：通过两个轮子的平均编码器变化量来计算总路程，抵消转弯影响
	float avg_diff = (fabsf((float)diff_A) + fabsf((float)diff_B)) / 2.0f;
	float distance_this_cycle = (avg_diff * WHEEL_CIRCUMFERENCE) / EFFECTIVE_PPR;
	total_distance_traveled += distance_this_cycle;
	
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
	SpeedConfig_t cfg = GetSpeedConfig();
	
    switch (sm.current_action)
    {
		case ACT_STRAIGHT:
		{
			// 直线速度参数
			const float max_straight_speed = cfg.straight_max_speed;   // 直道最高速度
			const float end_speed = cfg.straight_end_speed;            // 末端期望速度（与循迹初始速度一致）
			const float speed_ramp_step = cfg.straight_ramp_step;      // 每周期增速步长
			static float current_speed = 0.0f;        // 当前实际输出的速度

			// 根据已行驶距离计算理论目标速度（线性降低）
			float progress = total_distance_traveled / DISTANCE_TO_C;
			if (progress > 1.0f) progress = 1.0f;
			float target_speed = end_speed + (max_straight_speed - end_speed) * (1.0f - progress);

			// 判断是否为首次起步（任务开始后的第一个直行段）
			bool is_first_start = (sm.stage_index == 0 && sm.lap_count == 0);

			if (is_first_start) {
				if (sm.flag_just_entered) {
					current_speed = 0.10f;   // 起步初始速度，避免冲击
				} else {
					// 平滑加速至目标速度
					if (current_speed < target_speed) {
						current_speed += speed_ramp_step;
						if (current_speed > target_speed) current_speed = target_speed;
					}
				}
			} else {
				// 非首次起步，直接使用目标速度，不进行斜坡
				current_speed = target_speed;
			}

			float base_speed = current_speed;

			// 角度 PID 计算（保持不变）
			float current_yaw = mpu_control->yaw;
			PID_Node_SetSetpoint(&pidAngle, sm.target_angle);
			PID_Node_UpdateMeasurement(&pidAngle, current_yaw);
			PID_ExecuteNode(&pidAngle, dt);
			float steering = pidAngle.output;

			target_A = base_speed - steering;
			target_B = base_speed + steering;
			break;
		}

		case ACT_TRACKING:
		{
			// 循迹目标速度（最高值）
			const float max_tracking_speed = cfg.tracking_max_speed;   // 根据实际测试调整
			const float speed_step = cfg.tracking_ramp_step;           // 每周期增速步长
			static float tracking_speed = 0.10f;      // 当前目标速度（静态变量，保持跨周期）

			if (sm.flag_just_entered) {
				// 刚进入循迹时，速度设为较低初始值
				tracking_speed = cfg.tracking_init_speed;                // 初始低速，避免冲出
				LOG_DEBUG("TRACKING start, init speed=%.2f", tracking_speed);
			} else {
				// 每周期逐渐增加速度，直到最大值
				if (tracking_speed < max_tracking_speed) {
					tracking_speed += speed_step;
					if (tracking_speed > max_tracking_speed) {
						tracking_speed = max_tracking_speed;
					}
				}
			}

			// 灰度 PID 计算
			PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
			PID_ExecuteNode(&pidGrayscale, dt);
			float speed_diff = pidGrayscale.output;

			target_A = tracking_speed - speed_diff;
			target_B = tracking_speed + speed_diff;
			break;
		}
		
        case ACT_CALIBRATION:
        {
            // 校准模式：低速前进，仅用灰度PID修正姿态，直到精确对准黑线
            float calibration_speed = 0.05f; // 使用很低的速度
            
			// 灰度 PID
			PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
			PID_ExecuteNode(&pidGrayscale, dt);
			float gray_steering = pidGrayscale.output;
			if (gray_steering > 0.1f) gray_steering = 0.1f;
			if (gray_steering < -0.1f) gray_steering = -0.1f;
			
			// 角度 PID：目标设为原始航向（回退修正）
			PID_Node_SetSetpoint(&pidAngle, sm.original_yaw);   // 使用 original_yaw
			PID_Node_UpdateMeasurement(&pidAngle, mpu_control->yaw);
			PID_ExecuteNode(&pidAngle, dt);
			float angle_steering = pidAngle.output;
			
			float total_steering = gray_steering + angle_steering;
			if (total_steering > 0.15f) total_steering = 0.15f;  // 可以在这里对steering做更强的限幅，保证动作柔和
			if (total_steering < -0.15f) total_steering = -0.15f;
			
			target_A = calibration_speed - total_steering;
			target_B = calibration_speed + total_steering;
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
