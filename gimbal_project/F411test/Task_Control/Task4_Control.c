#include "Task4_Control.h"
#include "Task_Control.h"
#include <math.h>

/* ================================================================
   Task4: 车载平衡 — 前馈(车加速度) + PD(球位置)
   车加减速 → 前馈预倾梁抵消惯性 → PID 微调残差
   ================================================================ */
#define FF_BASE        25.0f    // 梁水平基准角
#define TARGET_CM      CENTER_CM    // 球目标 (0cm)

/* PID */
#define KP   0.5f
#define KI   0.0f
#define KD   10.0f

/* 前馈 */
#define K_FF 0.05f              // 加速度→角度系数 (需标定)

static PID_Node pid;
static bool     started = false;

static float PxToCm(int32_t px) { return px * 0.1f; }

/* ================================================================
   初始化
   ================================================================ */
void Task4_Init(void)
{
    PID_Node_Init(&pid, "t4", KP, KI, KD);
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_SetLimit(&pid, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    =  0.0f,
        .output_max   = 10.0f, .output_min   = -10.0f,
        .integral_max =  3.0f, .derivative_max = 30.0f,
        .deadband     =  0.0f,
    });
    ZDT_Pulse_SetRamp(500, 1500, 3);
    ZDT_Pulse_Enable();
}

/* ================================================================
   死推算
   ================================================================ */
static uint32_t last_data_ms = 0;
static float    dead_pos_cm  = CENTER_CM;
static float    dead_vel     = 0.0f;
static bool     data_lost    = false;
#define DATA_TIMEOUT_MS  150

/* ================================================================
   主控循环 (2ms)
   ================================================================ */
void Task4_Update(float dt)
{
    if (!started) return;

    float ball_cm;
    uint32_t now = HAL_GetTick();

    /* 视觉数据 */
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

    /* PID: 球位置 → 角度修正 */
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_UpdateMeasurement(&pid, ball_cm);
    PID_ExecuteNode(&pid, dt);

    /* 前馈: IMU 只在视觉帧更新时读一次 (~60Hz) */
    static float car_accel = 0;
    if (g_ball_updated) {
        short ax, ay, az;
        MPU_Get_Accelerometer(&ax, &ay, &az);
        static float ax_baseline = 0;
        ax_baseline += 0.01f * (ax - ax_baseline);
        car_accel = ax - ax_baseline;
    }
    float ff_angle = K_FF * car_accel;

    /* 合成 + 限幅 + 输出 */
    float out = pid.output + ff_angle;
    if (out > 10.0f)  out = 10.0f;
    if (out < -10.0f) out = -10.0f;

    float angle = FF_BASE - out;
    ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(angle));
}

/* ================================================================
   备用发送
   ================================================================ */
void Task4_Control_Send(void)
{
    if (data_lost) {
        ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(FF_BASE));
    }
}

/* ================================================================
   API
   ================================================================ */
void Task4_Start(void) {
    started = true;
    ZDT_Pulse_SetPos(ZDT_Pulse_AngleToClk(FF_BASE));
    PID_Node_SetSetpoint(&pid, TARGET_CM);
    PID_Node_ResetIntegral(&pid);
}
void Task4_Stop(void)  { started = false; }
bool Task4_IsRunning(void) { return started; }
bool Task4_IsDone(void)    { return false; }  // 四题无终止条件
uint32_t Task4_GetStep(void)   { return 0; }
float    Task4_GetTarget(void) { return TARGET_CM; }
float    Task4_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task4_GetOutput(void) { return pid.output; }
