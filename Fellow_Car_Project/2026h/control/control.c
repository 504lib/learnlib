#include "control.h"
#include "Motor_AT4950.h"
#include "grayscale.h"
#include "app_log.h"
#include "main.h"

/* ---- 全局变量 ---- */
#define CTRL_MODE_RUN   1
#define CTRL_MODE_STOP  0

uint8_t ctrl_mode = CTRL_MODE_STOP;

float Actual_Speed_A = 0.0f;
float Actual_Speed_B = 0.0f;
float Average_Speed = 0.0f;
float distance = 0.0f;
volatile uint8_t gray_byte = 0;
float gray_error = 0.0f;

/* PID 实例 */
PID_Node pidMotor1Speed;
PID_Node pidMotor2Speed;
PID_Node pidGrayscale;

/* 电机硬件句柄 (main.c 定义) */
extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

/* 车轮参数 */
#define WHEEL_CIRCUMFERENCE  0.1508f    // 轮周长 (m)
#define EFFECTIVE_PPR        1040       // 每转脉冲数

/* 速度参数 */
static float base_speed = 0.32f;    // 循迹速度 (m/s)

/* ---- 初始化 ---- */
void Control_Init(void)
{
    /* 灰度 PID */
    PID_Node_Init(&pidGrayscale, "Gray", 0.090, 0.0018f, 0.60f);//0.090, 0.0016f, 0.60f
    PID_Limit gray_limit = {
        .output_max     = 0.6f,
        .output_min     = -0.6f,
        .integral_max   = 2.0f,
        .derivative_max = 20.0f,
        .deadband       = 0.3f,
        .setpoint_min   = -1000.0f,
        .setpoint_max   = 1000.0f,
        .input_min      = -1000.0f,
        .input_max      = 1000.0f,
    };
    PID_Node_SetLimit(&pidGrayscale, gray_limit);
    PID_Node_SetIntegralAttenuationKp(&pidGrayscale, 0.95f);

    /* 左轮速度 PID */
    PID_Node_Init(&pidMotor1Speed, "M1_Speed", 2000.0f, 6.0f, 0.0f);
    PID_Node_SetLimit(&pidMotor1Speed, (PID_Limit){
        .setpoint_max = 5.0f, .setpoint_min = -5.0f,
        .input_max    = 5.0f, .input_min    = -5.0f,
        .output_max   = 1000.0f, .output_min = -1000.0f,
        .integral_max = 1000.0f, .derivative_max = 20.0f,
        .deadband     = 0.0f,
    });

    /* 右轮速度 PID */
    PID_Node_Init(&pidMotor2Speed, "M2_Speed", 2000.0f, 6.0f, 0.0f);
    PID_Node_SetLimit(&pidMotor2Speed, (PID_Limit){
        .setpoint_max = 5.0f, .setpoint_min = -5.0f,
        .input_max    = 5.0f, .input_min    = -5.0f,
        .output_max   = 1000.0f, .output_min = -1000.0f,
        .integral_max = 1000.0f, .derivative_max = 20.0f,
        .deadband     = 0.0f,
    });

    LOG_INFO("Control init done");
}

/* ---- 编码器速度反馈 (中断中调用) ---- */
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt)
{
    if (dt <= 0.0f) return;

    float dt_s = dt / 1000.0f;
    Actual_Speed_A = -(diff_A * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_s);
    Actual_Speed_B =  (diff_B * WHEEL_CIRCUMFERENCE) / (EFFECTIVE_PPR * dt_s);

    /* 低通滤波 */
    static float filtered_A = 0.0f, filtered_B = 0.0f;
    filtered_A = 0.8f * filtered_A + 0.2f * Actual_Speed_A;
    filtered_B = 0.8f * filtered_B + 0.2f * Actual_Speed_B;
    Actual_Speed_A = filtered_A;
    Actual_Speed_B = filtered_B;
    Average_Speed = (filtered_A + filtered_B) / 2.0f;
    distance += Average_Speed * dt_s;
}

/* ---- 主控制更新 (周期性调用) ---- */
void Control_Update(float dt)
{
    float target_A = 0.0f, target_B = 0.0f;

    switch (ctrl_mode) {

    case CTRL_MODE_RUN:
        PID_Node_UpdateMeasurement(&pidGrayscale, gray_error);
        PID_ExecuteNode(&pidGrayscale, dt);
        {
            float diff = pidGrayscale.output;
            target_A = base_speed - diff;
            target_B = base_speed + diff;
        }
        break;

    default:
        target_A = 0.0f;
        target_B = 0.0f;
        break;
    }

    /* 速度闭环 */
    PID_Node_SetSetpoint(&pidMotor1Speed, target_A);
    PID_Node_SetSetpoint(&pidMotor2Speed, target_B);
    PID_Node_UpdateMeasurement(&pidMotor1Speed, Actual_Speed_A);
    PID_Node_UpdateMeasurement(&pidMotor2Speed, Actual_Speed_B);
    PID_ExecuteNode(&pidMotor1Speed, dt);
    PID_ExecuteNode(&pidMotor2Speed, dt);

    /* PWM 输出 */
    Motor_setSpeed(&motor1, (int16_t)pidMotor1Speed.output);
    Motor_setSpeed(&motor2, (int16_t)pidMotor2Speed.output);
}

/* ---- 灰度窗口滤波 ---- */
uint8_t Control_GrayByte_Window_Filter(size_t window_size)
{
    static uint8_t last_gray = 0;
    static uint8_t count = 0;
    uint8_t current_gray = GPIOE->IDR & 0xFF;

    if (current_gray == last_gray) {
        count++;
        if (count >= window_size) {
            gray_byte = current_gray;
            count = 0;
        }
    } else {
        last_gray = current_gray;
        count = 1;
    }
    return gray_byte;
}

/* ---- 路口检测 ---- */
bool Control_IsCrossDetected(uint8_t threshold)
{
    uint8_t bits = gray_byte;
    int max_cont = 0, cur_cont = 0;
    for (int i = 0; i < 8; i++) {
        if ((bits & (1 << i)) == 0) {
            cur_cont++;
            if (cur_cont > max_cont) max_cont = cur_cont;
        } else {
            cur_cont = 0;
        }
    }
    return max_cont >= threshold;
}

/* ---- 启动 ---- */
void Control_Start(void)
{
    PID_Node_ResetIntegral(&pidMotor1Speed);
    PID_Node_ResetIntegral(&pidMotor2Speed);
    PID_Node_ResetIntegral(&pidGrayscale);
    ctrl_mode = CTRL_MODE_RUN;
}

/* ---- 停车 ---- */
void Control_Stop(void)
{
    ctrl_mode = CTRL_MODE_STOP;
    PID_Node_ResetIntegral(&pidMotor1Speed);
    PID_Node_ResetIntegral(&pidMotor2Speed);
    PID_Node_ResetIntegral(&pidGrayscale);
    Motor_setSpeed(&motor1, 0);
    Motor_setSpeed(&motor2, 0);
}

/* ---- 到达终点 (全白) ---- */
bool Control_IsAtEnd(void)
{
    return (gray_byte == 0xFF);
}

float Control_GetAverageSpeed(void)
{
    return Average_Speed;
}

float Control_GetCurrentDistance(void)
{
    return distance;
}
