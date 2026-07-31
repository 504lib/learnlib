#include "Task_Control.h"
#include <math.h>

static PID_Node          pid_pos, pid_vel;
static ZDT_Motor_Handle_t* motor;
static uint32_t stable_tick = 0;
static int      step        = 0;
static float    targets[3]  = {12.5f, 17.5f, 7.5f}; // cm, 绝对位置

#define FF_BASE    26.0f

static float PxToCm(int32_t px) { return px * 0.1f; }

void Task3_Init(ZDT_Motor_Handle_t* m)
{
    motor = m;
    step  = 0;
    stable_tick = 0;

    /* 外环: 位置→期望速度, P为主 */
    PID_Node_Init(&pid_pos, "pos", 0.0f, 0.0f, 0.0f);
    PID_Node_SetSetpoint(&pid_pos, targets[0]);
    PID_Node_SetLimit(&pid_pos, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    =  0.0f,
        .output_max   = 15.0f, .output_min   = -15.0f,
        .integral_max =  5.0f, .derivative_max = 10.0f,
        .deadband     =  0.0f,
    });

    /* 内环: 速度→电机角度, P+D */
    PID_Node_Init(&pid_vel, "vel",0.2f, 0.2f, 2.5f);
    PID_Node_SetEnabled(&pid_vel, true);  // 确保使能
    PID_Node_SetLimit(&pid_vel, (PID_Limit){
        .setpoint_max = 15.0f, .setpoint_min = -15.0f,
        .input_max    = 30.0f, .input_min    = -30.0f,
        .output_max   = 20.0f, .output_min   = -20.0f,
        .integral_max =  5.0f, .derivative_max = 15.0f,
        .deadband     =  0.0f,
    });

    ZDT_Enable(motor);
}

void Task3_Update(float dt)
{
    if (step >= 3) return;

    float ball_cm  = PxToCm(g_ball_pos);
    float ball_vel = g_ball_vel * 0.1f;  // mm/s → cm/s
    float target   = targets[step];

    /* 外环: 位置误差 → 期望速度 */
    PID_Node_SetSetpoint(&pid_pos, target);
    PID_Node_UpdateMeasurement(&pid_pos, ball_cm);
    PID_ExecuteNode(&pid_pos, dt);

    /* 内环: 期望速度 vs 实测速度 → 电机修正 */
    PID_Node_SetSetpoint(&pid_vel, 0.0f);
    PID_Node_UpdateMeasurement(&pid_vel, ball_vel);
    PID_ExecuteNode(&pid_vel, dt);

    /* 稳态判断 */
    if (fabsf(ball_cm - target) <= 1.0f) {
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
    float angle = FF_BASE - pid_vel.output;
    ZDT_MoveToClk(motor, ZDT_AngleToClk(angle));
}

bool     Task3_IsDone(void)    { return step >= 3; }
uint32_t Task3_GetStep(void)   { return step; }
float    Task3_GetTarget(void) { return targets[step]; }
float    Task3_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task3_GetOutput(void) { return pid_vel.output; }
