#include "./Control_Speed.h"
#include <math.h>

// ---- 内部静态变量 ----
static PID_Node Speed_PID_node1, Speed_PID_node2;
static PID_Node pidGrayscale;                    // 灰度 PID 节点
static float Actual_Speed_A = 0.0f;
static float Actual_Speed_B = 0.0f;
static float total_distance_traveled = 0.0f;
static MotorAT4950* motor1_ptr = NULL;
static MotorAT4950* motor2_ptr = NULL;

static float base_speed = 0.15f;                // 基础巡线速度 m/s


// // 电机句柄（由 Control_Init 传入）
// static MotorAT4950* motor1_ptr = NULL;
// static MotorAT4950* motor2_ptr = NULL;

/* 初始化 */
void Control_Init(MotorAT4950* m1, MotorAT4950* m2)
{
    motor1_ptr = m1;
    motor2_ptr = m2;
    // ===== 灰度 PID 初始化 =====
    PID_Node_Init(&Speed_PID_node1, "speed_node1", 700.0f, 2.5f, 0.0f);
    PID_Node_Init(&Speed_PID_node2, "speed_node2", 700.0f, 2.5f, 0.0f);
    PID_Node_Init(&pidGrayscale, "Grayscale", 0.055f, 0.0001f, 1.0f);
    
    PID_Node_SetEnabled(&Speed_PID_node1, true);
    PID_Node_SetEnabled(&Speed_PID_node2, true);
    PID_Node_SetEnabled(&pidGrayscale, true);
    // PID_Node_SetSetpoint(&pidAngle, 0.0f);   // 目标角度为0度（保持当前航向）
    
    PID_Node_SetIntegralAttenuationKp(&Speed_PID_node1, 0.97f);
    PID_Node_SetIntegralAttenuationKp(&Speed_PID_node2, 0.97f);
    PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);
    // PID_Node_SetIntegralAttenuationKp(&pidAngle, 0.98f);
    //速度环限制
    PID_Node_SetLimit(&Speed_PID_node1, (PID_Limit){
        .deadband = 0.0f,
        .derivative_max = 1000,
        .input_max = 1.0f,
        .input_min = 0.0f,
        .integral_max = 1000,
        .output_max = 1000,
        .output_min = -1000,
        .setpoint_max = 0.5f,
        .setpoint_min = 0.0f
    });
    PID_Node_SetLimit(&Speed_PID_node2, (PID_Limit){
        .deadband = 0.0f,
        .derivative_max = 1000,
        .input_max = 1.0f,
        .input_min = 0.0f,
        .integral_max = 1000,
        .output_max = 1000,
        .output_min = -1000,
        .setpoint_max = 0.5f,
        .setpoint_min = 0.0f
    });
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
	// PID_Node_SetLimit(&pidGrayscale, gray_limit);
    // 	PID_Limit angle_limit = {
	// 	.setpoint_min = -180.0f, 
	// 	.setpoint_max = 180.0f,
	// 	.input_min = -180.0f, 
	// 	.input_max = 180.0f,
	// 	.output_max = 0.2f,      // 最大转向补偿速度差（m/s），根据base_speed调整
	// 	.output_min = -0.2f,
	// 	.integral_max = 20.0f,
	// 	.derivative_max = 20.0f,
	// 	.deadband = 1.0f          
	// };
    // PID_Node_SetLimit(&pidAngle, angle_limit);
}

/* 获取节点指针 */
PID_Node* Control_Speed_PID_Hander(Control_Speed_Select_Object_Option option)
{
    return (option == CONTROL_SPEED1_OBJECT) ? &Speed_PID_node1 : &Speed_PID_node2;
}

/* 设置目标速度 */
void Control_Speed_SetSetPoint(Control_Speed_Select_Object_Option option, float speed)
{
    PID_Node_SetSetpoint((option == CONTROL_SPEED1_OBJECT) ? &Speed_PID_node1 : &Speed_PID_node2, speed);
}
void Control_SetBaseSpeed(float speed)
{
    base_speed = speed;
}

/* 设置 PID 参数 */
void Control_Speed_SetPID(Control_Speed_Select_Object_Option option, float kp, float ki, float kd)
{
    PID_Node* node = (option == CONTROL_SPEED1_OBJECT) ? &Speed_PID_node1 : &Speed_PID_node2;
    PID_Node_SetKp(node, kp);
    PID_Node_SetKi(node, ki);
    PID_Node_SetKd(node, kd);
}

/* 获取实际速度 */
float Control_Speed_GetActualSpeed(Control_Speed_Select_Object_Option option)
{
    return (option == CONTROL_SPEED1_OBJECT) ? Actual_Speed_A : Actual_Speed_B;
}

/* 更新速度反馈（内部调用） */
void UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt > 0.0f) {
        float dt_ms = dt / 1000.0f;
        Actual_Speed_A =  (diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
        Actual_Speed_B =  -(diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_ms);
    }

    // 路程累积
    float avg_diff = (fabsf((float)diff_A) + fabsf((float)diff_B)) / 2.0f;
    total_distance_traveled += (avg_diff * WHEEL_CIRCUMFERENCE) / EFFECTIVE_PPR;

    // 低通滤波
    static float filtered_A = 0, filtered_B = 0;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;
}

/* 速度环更新任务（每 4ms 调用） */
void Control_UpdateSpeedPID(int32_t diff_A, int32_t diff_B, float dt)
{
    if (!motor1_ptr || !motor2_ptr) return;

    // 1. 更新速度反馈
    // UpdateSpeedFeedback(diff_A, diff_B, dt);

    // 2. 更新 PID 输入
    PID_Node_UpdateMeasurement(&Speed_PID_node1, Actual_Speed_A);
    PID_Node_UpdateMeasurement(&Speed_PID_node2, Actual_Speed_B);

    // 3. 执行 PID
    PID_ExecuteNode(&Speed_PID_node1, dt);
    PID_ExecuteNode(&Speed_PID_node2, dt);

    // 4. 输出到电机
    Motor_setSpeed(motor1_ptr, (int16_t)Speed_PID_node1.output);
    Motor_setSpeed(motor2_ptr, (int16_t)Speed_PID_node2.output);
}

/* 灰度环更新任务（每 20ms 调用） */
void Control_SetGrayTarget(float gray_error)
{
    if (!motor1_ptr || !motor2_ptr) return;
    // 灰度 PID 计算
    PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
    PID_ExecuteNode(&pidGrayscale, 20.0f);
    float speed_diff = pidGrayscale.output;
    // 计算左右轮目标速度
    float target_A = base_speed - speed_diff;
    float target_B = base_speed + speed_diff;
    if (target_A < 0.0f) target_A = 0.0f;
    if (target_B < 0.0f) target_B = 0.0f;
    // 设定新目标（速度环在 4ms 调用时会读取）
    PID_Node_SetSetpoint(&Speed_PID_node1, target_A);
    PID_Node_SetSetpoint(&Speed_PID_node2, target_B);
}