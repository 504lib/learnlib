#include "Task_Control.h"

static PID_Node          pid_ball;
static ZDT_Motor_Handle_t* motor;
static uint32_t stable_tick = 0;
static int      step        = 0;
static float    targets[3]  = {0.0f, 5.0f, -5.0f}; // cm, 相对零点

/* 像素→cm 线性插值 (标定后替换) */
static float PxToCm(int32_t px)
{
    // 标定: -5cm=-85px, +5cm=+83px, 分左右线性
    if (px < 0)
        return px * (5.0f / 85.0f);   // 左半: 0.0588 cm/px
    else
        return px * (5.0f / 83.0f);   // 右半: 0.0602 cm/px
}

static float asym_error(float setpoint, float measurement)
{
    float err = setpoint - measurement;
    float abs_err = (err > 0) ? err : -err;

    /* 动态P: 误差大→压缩防过冲, 误差小→放大保精度 */
    float scale = 3.0f / (1.0f + abs_err * 1.5f);

    /* 方向不对称: 抬杆侧(负误差)需要更大增益 */
    float asym = (err > 0) ? 1.2f : 0.8f;

    return err * scale * asym;
}

void Task3_Init(ZDT_Motor_Handle_t* m)
{
    motor = m;
    step  = 0;
    stable_tick = 0;

    PID_Node_Init(&pid_ball, "ball", 0.7f, 0.1f, 0.5f);
    PID_Node_SetSetpoint(&pid_ball, targets[0]);
    PID_Node_SetLimit(&pid_ball, (PID_Limit){
        .setpoint_max =   5.0f, .setpoint_min =  -5.0f,
        .input_max    =   12.5f, .input_min    =  -12.5f,
        .output_max   =  45.0f, .output_min   =   -45.0f,
        .integral_max =  100.0f, .derivative_max = 15.0f,
        .deadband     =   0.0f,
    });
    PID_Node_SetCustomCallback(&pid_ball, (PID_Custom_Functions){
        .custom_error_calculation = asym_error,
    });

    ZDT_Enable(motor);
    // ZDT_ZeroPos(motor);
}

void Task3_Update(float dt)
{
    if (step >= 3) return;

    float ball_cm   = PxToCm(g_ball_pos);
    float target_cm = 5.0f;

    PID_Node_SetSetpoint(&pid_ball, target_cm);
    PID_Node_UpdateMeasurement(&pid_ball, ball_cm);
    PID_ExecuteNode(&pid_ball, dt);

    /* 稳态判断: 误差≤1cm 持续 1s→下一步 */
    // if (fabs(ball_cm - target_cm) <= 1.0f) {
    //     if (stable_tick == 0) stable_tick = HAL_GetTick();
    //     if (HAL_GetTick() - stable_tick >= 1000) {
    //         step++;
    //         stable_tick = 0;
    //     }
    // } else {
    //     stable_tick = 0;
    // }
}

void Task3_Control_Send(void)
{
    float output = pid_ball.output;

    /* 位置补偿: 球离电机越远(|px|越大), 同样倾角效果越弱, 放大输出 */
    // float px_abs = (float)(g_ball_pos > 0 ? g_ball_pos : -g_ball_pos);
    // float pos_gain = 1.0f + px_abs * 0.015f;
    // output *= pos_gain;

    // /* 椭圆球破静摩擦: 小误差卡边时加 kick */
    // float err = PxToCm(g_ball_pos) - 0.0f;
    // float abs_err = err > 0 ? err : -err;
    // if (abs_err > 0.2f && abs_err < 1.5f) {
    //     output += (output > 0 ? 5.0f : -5.0f);
    // }
    ZDT_MoveToClk(motor, ZDT_AngleToClk(26.0f - output));
}


bool Task3_IsDone(void)
{
    return step >= 3;
}

uint32_t Task3_GetStep(void)
{
    return step;
}

float Task3_GetTarget(void)
{
    return targets[step];
}

float Task3_GetOutput(void)
{
    return pid_ball.output;
}

float Task3_GetCurrent(void)
{
    return PxToCm(g_ball_pos);
}
