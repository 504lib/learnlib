#include "Task4_Control.h"
#include "Task_Control.h"
#include "mpu6050_user.h"
#include <math.h>

/* ================================================================
   Task4: 车载平衡 — 查表基准 + PD(球位置) + IMU前馈
   ================================================================ */
#define TARGET_CM  CENTER_CM
#define KP         3.0f
#define KD         15.0f
#define K_FF       0.05f

static PID_Node pid;
static bool     started = false;

static float  output      = 0.0f;
static float  motor_angle = 28.0f;
static bool   force_cmd   = true;    /* Start后强制发一次指令 */

static float PxToCm(int32_t px) { return px * 0.1f; }

void Task4_Init(void)
{
    PID_Node_Init(&pid, "t4", KP, 0.0f, KD);
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_SetLimit(&pid, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    =  0.0f,
        .output_max   = 10.0f, .output_min   = -10.0f,
        .integral_max =  3.0f, .derivative_max = 30.0f,
        .deadband     =  0.0f,
    });
    ZDT_Pulse_SetRamp(500, 1500, 3);   /* 起步500 + 巡航8k + 100步斜坡 */
    ZDT_Pulse_Enable();
}

/* ---- 死推算 ---- */
static uint32_t last_data_ms = 0;
static float    dead_pos_cm  = CENTER_CM;
static float    dead_vel     = 0.0f;
static bool     data_lost    = false;
#define DATA_TIMEOUT_MS  150

void Task4_Update(float dt)
{
    if (!started) return;

    float ball_cm;
    uint32_t now = HAL_GetTick();

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

    PID_Node_SetKp(&pid, KP * 0.2f);
    PID_Node_SetKd(&pid, KD);
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_UpdateMeasurement(&pid, ball_cm);
    PID_ExecuteNode(&pid, dt);

    // MPU6050_Data_t* imu = MPU6050_GetHandle();
    float ff_angle = 0; // 暂时为0

    float out = pid.output + ff_angle;
    out -= 0.5f * dead_vel;
    if (out > 10.0f)  out = 10.0f;
    if (out < -10.0f) out = -10.0f;

    output      = out;
    motor_angle = lookup_angle(TARGET_CM) - out;

    /* 只在目标角变化超过阈值时才发新指令, 避免每2ms重置梯形斜坡 */
    // static int32_t last_cmd_clk = 0;
    // int32_t new_clk = ZDT_Pulse_AngleToClk(motor_angle);
    // if (force_cmd || abs(new_clk - last_cmd_clk) >= 3) {
    //     force_cmd = false;
    //     last_cmd_clk = new_clk;
        ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(motor_angle));
    // }
}

/* 断联兜底 */
void Task4_Control_Send(void)
{
    if (data_lost) {
        ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(lookup_angle(CENTER_CM)));
    }
}

/* ---- API ---- */
void Task4_Start(void) {
    started = true;
    force_cmd = true;
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_ResetIntegral(&pid);
}
void Task4_Stop(void)  { started = false; }
bool Task4_IsRunning(void) { return started; }
bool Task4_IsDone(void)    { return false; }
uint32_t Task4_GetStep(void)   { return 0; }
float    Task4_GetTarget(void) { return TARGET_CM; }
float    Task4_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task4_GetOutput(void) { return output; }
