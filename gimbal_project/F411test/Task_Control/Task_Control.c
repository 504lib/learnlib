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

void Task3_Init(ZDT_Motor_Handle_t* m)
{
    motor = m;
    step  = 0;
    stable_tick = 0;

    PID_Node_Init(&pid_ball, "ball", 0.8f, 0.2f, 0.3f);
    PID_Node_SetSetpoint(&pid_ball, targets[0]);
    PID_Node_SetLimit(&pid_ball, (PID_Limit){
        .setpoint_max =   5.0f, .setpoint_min =  -5.0f,
        .input_max    =   12.5f, .input_min    =  -12.5f,
        .output_max   =  25.0f, .output_min   =   -15.0f,
        .integral_max =  10.0f, .derivative_max = 15.0f,
        .deadband     =   0.0f,
    });

    ZDT_Enable(motor);
    ZDT_ZeroPos(motor);
    ZDT_MoveToClk(motor, 0);
}

void Task3_Update(float dt)
{
    if (step >= 3) return;

    float ball_cm   = PxToCm(g_ball_pos);
    float target_cm = targets[step];

    PID_Node_SetSetpoint(&pid_ball, target_cm);
    PID_Node_UpdateMeasurement(&pid_ball, ball_cm);
    PID_ExecuteNode(&pid_ball, dt);

    /* 稳态判断: 误差≤1cm 持续 1s→下一步 */
    if (fabs(ball_cm - target_cm) <= 1.0f) {
        if (stable_tick == 0) stable_tick = HAL_GetTick();
        if (HAL_GetTick() - stable_tick >= 1000) {
            step++;
            stable_tick = 0;
        }
    } else {
        stable_tick = 0;
    }
}

void Task3_Control_Send(void)
{
    ZDT_MoveToClk(motor, ZDT_AngleToClk(-pid_ball.output));
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
