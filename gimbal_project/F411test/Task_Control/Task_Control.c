#include "Task_Control.h"
#include <math.h>

#define STEP_COUNT   2
#define STABLE_CM    1.0f
#define STABLE_MS    1000

static PID_Node pid;
static int      step        = 0;
static bool     started     = false;

static const float targets[STEP_COUNT] = { 17.5f, 7.5f };

/* ================================================================
   查表: 球位置(cm) → 平衡角(°)
   双向逼近取平均, 12.5cm手工平滑
   ================================================================ */
static const float tbl_pos[] = { 4.5f, 6.5f, 8.5f, 10.5f, 12.5f, 14.5f, 16.5f, 18.5f };
static const float tbl_ang[] = { 31.75f, 31.60f, 30.10f, 28.80f, 28.10f, 27.40f, 26.50f, 25.10f };
#define TBL_N (sizeof(tbl_pos)/sizeof(tbl_pos[0]))

float g_pos_offset = 0.0f;   /* 摄像头位置偏移(cm), 按键一键校准 */

/* 查表: 先修正摄像头偏移, 再查平衡角 */
float lookup_angle(float cam_cm) {
    float pos_cm = cam_cm + g_pos_offset;
    if (pos_cm <= tbl_pos[0]) return tbl_ang[0];
    if (pos_cm >= tbl_pos[TBL_N-1]) return tbl_ang[TBL_N-1];
    for (int i = 0; i < TBL_N-1; i++) {
        if (pos_cm >= tbl_pos[i] && pos_cm <= tbl_pos[i+1]) {
            float t = (pos_cm - tbl_pos[i]) / (tbl_pos[i+1] - tbl_pos[i]);
            return tbl_ang[i] + t * (tbl_ang[i+1] - tbl_ang[i]);
        }
    }
    return tbl_ang[TBL_N/2];
}

/* 一键校准: 球放在梁中点(12.5cm), 按下即算偏移 */
void Table_CalibrateOffset(float cam_cm, float unused) {
    (void)unused;
    g_pos_offset = CENTER_CM - cam_cm;
}

typedef struct { float P, D; } Gains;
static const Gains g_step[STEP_COUNT] = {
    { 1.0f, 15.0f },  // O->+5
    { 3.0f, 15.0f },  // +5->-5
};

static float output      = 0.0f;
static float motor_angle = 28.0f;

static float PxToCm(int32_t px) { return px * 0.1f; }

void Task3_Init(void)
{
    step = 0;
    PID_Node_Init(&pid, "pd", g_step[0].P, 0.0f, g_step[0].D);
    PID_Node_SetSetpoint(&pid, targets[0]);
    PID_Node_SetLimit(&pid, (PID_Limit){
        .setpoint_max = 25.0f, .setpoint_min =  0.0f,
        .input_max    = 25.0f, .input_min    = -25.0f,
        .output_max   = 15.0f, .output_min   = -15.0f,
        .integral_max =  3.0f, .derivative_max = 50.0f,
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
    if (!started) return;

    /* 全部完成则保持最终目标 */
    int active = (step < STEP_COUNT) ? step : STEP_COUNT-1;
    float target = targets[active];
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

    /* 当前段 PD */
    PID_Node_SetKp(&pid, g_step[active].P * 0.2);
    PID_Node_SetKd(&pid, g_step[active].D);
    PID_Node_SetSetpoint(&pid, target);
    PID_Node_UpdateMeasurement(&pid, ball_cm);
    PID_ExecuteNode(&pid, dt);

    /* 第二段: 每次穿越目标线减P, 接近目标停计数, P不低于30% */
    if (step == 1) {
        static float  last_sign = 0;
        static int    osc_cnt   = 0;
        static int    prev_step = -1;
        if (step != prev_step) { osc_cnt = 0; last_sign = 0; prev_step = step; }
        float sign = (target - ball_cm > 0) ? 1.0f : -1.0f;
        if (last_sign != 0 && sign != last_sign && fabsf(target - ball_cm) > 1.5f)
            osc_cnt++;
        last_sign = sign;
        float scale = 1.0f / (1.0f + osc_cnt * 2.0f);
        if (scale < 0.3f) scale = 0.3f;
        PID_Node_SetKp(&pid, g_step[1].P * 0.2f * scale);
    }

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
    out -= (step == 0 ? 0.5f : 1.0f) * dead_vel;  // 第二段强刹
    if (out > 15.0f)  out = 15.0f;
    if (out < -15.0f) out = -15.0f;
    output      = out;
    motor_angle = lookup_angle(target) - out;
    ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(motor_angle));
}

void Task3_Control_Send(void)
{
    if (data_lost) {
        ZDT_Pulse_MoveToClk(ZDT_Pulse_AngleToClk(lookup_angle(12.5f)));
    }
}

void Task3_Start(void) {
    step = 0; started = true;
    ZDT_Pulse_SetPos(ZDT_Pulse_AngleToClk(lookup_angle(12.5f)));
    PID_Node_SetSetpoint(&pid, targets[0]);
    PID_Node_ResetIntegral(&pid);
}
void Task3_Stop(void)  { step = 0; started = false; }
bool Task3_IsRunning(void) { return started && step < STEP_COUNT; }
bool Task3_IsDone(void)    { return step >= STEP_COUNT; }
uint32_t Task3_GetStep(void)   { return step; }
float    Task3_GetTarget(void) { return (step < STEP_COUNT) ? targets[step] : 0.0f; }
float    Task3_GetCurrent(void){ return PxToCm(g_ball_pos); }
float    Task3_GetOutput(void) { return output; }
float    Task3_GetAngle(void)  { return motor_angle; }
