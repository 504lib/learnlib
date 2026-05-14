#include "./Control_Speed.h"
#include <math.h>

// ---- 内部静态变量，外部无法直接访问 ----
static PID_Node Speed_PID_node1, Speed_PID_node2;
static float Actual_Speed_A = 0.0f;
static float Actual_Speed_B = 0.0f;
static float total_distance_traveled = 0.0f;

// 电机句柄（由 Control_Init 传入）
static MotorAT4950* motor1_ptr = NULL;
static MotorAT4950* motor2_ptr = NULL;

/* 初始化 */
void Control_Init(MotorAT4950* m1, MotorAT4950* m2)
{
    motor1_ptr = m1;
    motor2_ptr = m2;

    PID_Node_Init(&Speed_PID_node1, "speed_node1", 2000.0f, 6.0f, 0.0f);
    PID_Node_Init(&Speed_PID_node2, "speed_node2", 2000.0f, 6.0f, 0.0f);
    PID_Node_SetEnabled(&Speed_PID_node1, true);
    PID_Node_SetEnabled(&Speed_PID_node2, true);
    PID_Node_SetIntegralAttenuationKp(&Speed_PID_node1, 0.97f);
    PID_Node_SetIntegralAttenuationKp(&Speed_PID_node2, 0.97f);

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

/* 速度环更新任务（每 20ms 调用） */
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