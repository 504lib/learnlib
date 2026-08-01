#include "Task_Control.h"
#include <math.h>

#define STEP_COUNT   2
#define FF_BASE      25.0f
#define STABLE_CM    1.0f
#define STABLE_MS    1000

static PID_Node pid;
static int      step        = 0;
static bool     started     = false;

static const float targets[STEP_COUNT] = { 17.5f, 7.5f };   // +5cm, -5cm

typedef struct { float P, D; } Gains;
static const Gains g_step[STEP_COUNT] = {
    { 0.8f,  15.0f },  // O->+5
    { 0.23f, 40.0f },  // +5->-5
};

static float PxToCm(int32_t px) { return px * 0.1f; }

void Task3_Init(void)
{
    step = 0;
    PID_Node_Init(&pid, "pd", g_step[0].P, 0.0f, g_step[0].D);
    PID_Node_SetSetpoint(&pid, targets[0]);
    PID_Node_SetLimit(&pid, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    =  0.0f,
        .output_max   = 10.0f, .output_min   = -10.0f,
        .integral_max =  0.0f, .derivative_max = 50.0f,
        .deadband     =  0.0f,
    });
    ZDT_Pulse_SetRamp(500, 1500, 3);
    ZDT_Pulse_Enable();
}

static uint32_t last_data_ms = 0;
static float    dead_pos_cm  = 12.5f;
static float    dead_vel     = 0.0f;
static bool     data_lost    = false;
#define DATA_TIMEOUT_MS  150

void Task3_Update(float dt)
{
    if (!started || step >= STEP_COUNT) return;

    float target   = targets[step];
    float ball_cm;
    uint32_t now = HAL_GetTick();

    /* 视觉数据获取 */
    if (g_ball_updated) {
        g_ball_updated = false;
        last_data_ms = now;
        if (data_lost) { PID_Node_ResetIntegral(&pid); data_lost = false; }
        ball_cm  = PxToCm(g_ball_pos);
        dead_pos_cm = ball_cm;
        dead_vel    = g_ball_vel * 0.1f;
    } else if (now - last_data_ms < DATA_TIMEOUT_MS) {
        dead_vel *= 0.95f;
        dead_pos_cm += dead_vel * dt / 1000.0f;
        ball_cm = dead_pos_cm;
    } else {
        data_lost = true;
        return;
    }

    /* 当前段 PID (卡住时 P 累加) */
    {
        float P = g_step[step].P;
        float err = target - ball_cm;
        static uint32_t stuck_ms = 0;
        if (fabsf(dead_vel) < 0.2f && fabsf(err) > 0.8f) {
            if (stuck_ms == 0) stuck_ms = now;
            if (now - stuck_ms < 5000) {
                P += 0.6f * (now - stuck_ms) / 400.0f;
                if (P > 7.0f) P = 7.0f;
            }
        } else {
            stuck_ms = 0;
        }
        PID_Node_SetKp(&pid, P);
        PID_Node_SetKd(&pid, g_step[step].D);
    }
    PID_Node_SetSetpoint(&pid, target);
    PID_Node_UpdateMeasurement(&pid, ball_cm);
    PID_ExecuteNode(&pid, dt);

    /* 稳态检测 */
    static uint32_t stable_since = 0;
    if (fabsf(target - ball_cm) <= STABLE_CM) {
        if (stable_since == 0) stable_since = now;
        else if (now - stable_since >= STABLE_MS) {
            step++;
            stable_since = 0;
            if (step < STEP_COUNT) {
                PID_Node_SetSetpoint(&pid, targets[step]);
                PID_Node_ResetIntegral(&pid);
            }
        }
    } else {
        stable_since = 0;
    }

    /* 输出 */
    float out = pid.output;
    if (out > 10.0f)  out = 10.0f;
    if (out < -10.0f) out = -10.0f;
    float angle = FF_BASE - out;
    ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(angle));
}

void Task3_Control_Send(void)
{
    if (data_lost) {
        ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(FF_BASE));
    }
}

void Task3_Start(void) {
    step = 0; started = true;
    ZDT_Pulse_SetPos(ZDT_Pulse_AngleToClk(FF_BASE));
    PID_Node_SetSetpoint(&pid, targets[0]);
    PID_Node_ResetIntegral(&pid);
}
void Task3_Stop(void)  { step = 0; started = false; }
bool Task3_IsRunning(void) { return started && step < STEP_COUNT; }
bool Task3_IsDone(void)    { return step >= STEP_COUNT; }
uint32_t Task3_GetStep(void)   { return step; }
float    Task3_GetTarget(void) { return (step < STEP_COUNT) ? targets[step] : 0.0f; }
float    Task3_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task3_GetOutput(void) { return pid.output; }
