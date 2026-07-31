#include "Task_Control.h"
#include <math.h>

static PID_Node          pid_pos, pid_vel;
static ZDT_Motor_Handle_t* motor;
static uint32_t stable_tick = 0;
static int      step        = 0;
static float    targets[3]  = {12.5f, 17.5f, 7.5f}; // cm, 绝对位置

#define FF_BASE    24.0f
#define VEL_MAX     4.0f   // 期望速度上限 cm/s

static float PxToCm(int32_t px) { return px * 0.1f; }

void Task3_Init(ZDT_Motor_Handle_t* m)
{
    motor = m;
    step  = 0;
    stable_tick = 0;

    /* 外环: 位置→期望速度 */
    PID_Node_Init(&pid_pos, "pos", 1.0f, 0.0f, 0.0f);
    PID_Node_SetSetpoint(&pid_pos, targets[0]);
    PID_Node_SetLimit(&pid_pos, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    =  0.0f,
        .output_max   =  4.0f, .output_min   = -4.0f,  // 温和期望速度
        .integral_max =  2.0f, .derivative_max =  5.0f,
        .deadband     =  0.0f,
    });

    /* 内环: 速度→电机角度, 温和增益 */
    PID_Node_Init(&pid_vel, "vel", 0.6f, 0.0f, 15.0f);
    PID_Node_SetEnabled(&pid_vel, true);
    PID_Node_SetLimit(&pid_vel, (PID_Limit){
        .setpoint_max =   4.0f, .setpoint_min =  -4.0f,
        .input_max    =   8.0f, .input_min    =  -8.0f,
        .output_max   =  12.0f, .output_min   = -12.0f,  // 减小输出范围
        .integral_max =  2.0f, .derivative_max = 10.0f,
        .deadband     =  0.0f,
    });

    ZDT_Enable(motor);
}

static uint32_t last_data_ms = 0;
static float    dead_pos_cm  = 12.5f;
static float    dead_vel     = 0.0f;
static bool     data_lost    = false;   // 长时间丢球标志
#define DATA_TIMEOUT_MS  150

void Task3_Update(float dt)
{
    if (step >= 3) return;

    float target   = targets[step];
    float ball_cm, ball_vel;
    uint32_t now = HAL_GetTick();

    if (g_ball_updated) {
        g_ball_updated = false;
        last_data_ms = now;

        if (data_lost) {
            /* 重获球: 复位PID积分, 重新初始化死推算 */
            PID_Node_ResetIntegral(&pid_pos);
            PID_Node_ResetIntegral(&pid_vel);
            data_lost = false;
        }

        ball_cm  = PxToCm(g_ball_pos);
        ball_vel = g_ball_vel * 0.1f;
        dead_pos_cm = ball_cm;
        dead_vel    = ball_vel;
    } else if (now - last_data_ms < DATA_TIMEOUT_MS) {
        /* 短暂丢帧: 死推算 */
        dead_vel *= 0.95f;
        dead_pos_cm += dead_vel * dt / 1000.0f;
        ball_cm  = dead_pos_cm;
        ball_vel = dead_vel;
    } else {
        /* 超时: 跳过PID, Task3_Control_Send直接回FF_BASE */
        data_lost = true;
        return;
    }

    if (ball_vel >  VEL_MAX * 2) ball_vel =  VEL_MAX * 2;
    if (ball_vel < -VEL_MAX * 2) ball_vel = -VEL_MAX * 2;

    PID_Node_SetSetpoint(&pid_pos, target);
    PID_Node_UpdateMeasurement(&pid_pos, ball_cm);
    PID_ExecuteNode(&pid_pos, dt);

    PID_Node_SetSetpoint(&pid_vel, 0.0f);
    PID_Node_UpdateMeasurement(&pid_vel, ball_vel);
    PID_ExecuteNode(&pid_vel, dt);

    /* 稳态判断 */
    // if (fabsf(ball_cm - target) <= 1.0f) {
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
    float angle;
    if (data_lost) {
        angle = FF_BASE;
    } else {
        float out = pid_vel.output;
        /* 抬杆侧(正输出)需要更大角度, 放杆侧(负输出)减权 */
        if (out > 0) out *= 1.1f;  // 抬杆放大
        else         out *= 0.7f;  // 放杆缩小
        angle = FF_BASE - out;
    }
    ZDT_MoveToClk(motor, ZDT_AngleToClk(angle));
}

bool     Task3_IsDone(void)    { return step >= 3; }
uint32_t Task3_GetStep(void)   { return step; }
float    Task3_GetTarget(void) { return targets[step]; }
float    Task3_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task3_GetOutput(void) { return pid_vel.output; }
