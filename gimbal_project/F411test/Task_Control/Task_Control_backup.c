#include "Task_Control.h"
#include "app_menu.h"

static ZDT_Motor_Handle_t* motor;

/* ---- 角度参数 ---- */
#define FF_BASE        26.0f   // 梁水平基准角度
#define TILT_RIGHT     21.0f   // 右倾(推球向+5)
#define TILT_LEFT      30.0f   // 左倾(推球向-5)
#define HOLD_ANGLE     26.0f   // 稳态保持角度

/* ---- 视觉辅助阈值 (单位: mm, g_ball_pos = pos_cm*10) ---- */
#define APPROACH_PLUS5   130    // 球接近+5cm的触发线 (3.0cm), 留余量给延迟
#define APPROACH_MINUS5  135    // 球接近-5cm的触发线 (-3.0cm)

/* ---- 时序保护 ---- */
#define O_TO_PLUS5_MS    80    // 右倾推球, 视觉触发→放平时间
#define HOLD_MS        1500    // 稳态最少保持时间
#define MAX_TILT_MS    3000    // 倾斜超时保护: 视觉丢了也能继续

typedef enum {
    S_IDLE = 0,
    S_O_TO_PLUS5,        // 右倾推球, 视觉触发→放平
    S_O_TO_WAIT,         // 等待视觉触发
    S_HOLD_PLUS5,        // 稳态+5
    S_PLUS5_TO_MINUS5,   // 左倾推球, 视觉触发→放平
    S_HOLD_MINUS5,       // 稳态-5
    S_DONE
} SeqState;

static SeqState   state       = S_IDLE;
static uint32_t   phase_start = 0;
static bool       started     = false;

void Task3_Init(ZDT_Motor_Handle_t* m)
{
    motor = m;
    state = S_IDLE;
    phase_start = 0;
    started = false;
    ZDT_Enable(motor);
    ZDT_MoveToClk(motor, ZDT_AngleToClk(FF_BASE));
}

void Task3_Start(void)
{
    state = S_O_TO_PLUS5;
    phase_start = HAL_GetTick();
    started = true;
}

void Task3_Stop(void)
{
    state = S_IDLE;
    started = false;
    ZDT_MoveToClk(motor, ZDT_AngleToClk(FF_BASE));
}

bool Task3_IsRunning(void)
{
    return started;
}

void Task3_Update(float dt)
{
    (void)dt;
    if (!started) return;
    if (state >= S_DONE) return;

    uint32_t elapsed = HAL_GetTick() - phase_start;
    int32_t  pos     = g_ball_pos;  // 视觉球位置, 单位 mm

    switch (state) {
    case S_O_TO_PLUS5:
        /* 视觉触发: 球接近+5就放平; 超时保护 */
        if (pos >= APPROACH_PLUS5) {
            state = S_HOLD_PLUS5;
            phase_start = HAL_GetTick();
        }
        break;
    case S_HOLD_PLUS5:
        if (elapsed >= O_TO_PLUS5_MS) {
            state = S_O_TO_WAIT;
            phase_start = HAL_GetTick();
        }
        break;
    case S_O_TO_WAIT:
        if (elapsed >= HOLD_MS) {
            state = S_PLUS5_TO_MINUS5;
            phase_start = HAL_GetTick();
        }
        break;
    case S_PLUS5_TO_MINUS5:
        /* 视觉触发: 球接近-5就放平 */
        if (pos <= APPROACH_MINUS5 || elapsed >= MAX_TILT_MS) {
            state = S_HOLD_MINUS5;
            phase_start = HAL_GetTick();
        }
        break;
    case S_HOLD_MINUS5:
        if (elapsed >= HOLD_MS) {
            state = S_DONE;
            phase_start = HAL_GetTick();
        }
        break;
    default:
        break;
    }
}

void Task3_Control_Send(void)
{
    if (!started) return;

    float angle = FF_BASE;
    switch (state) {
    case S_O_TO_PLUS5:       angle = TILT_RIGHT;  break;
    case S_HOLD_PLUS5:       angle = HOLD_ANGLE;   break;
    case S_O_TO_WAIT:        angle = FF_BASE;      break;
    case S_PLUS5_TO_MINUS5:  angle = TILT_LEFT;   break;
    case S_HOLD_MINUS5:      angle = HOLD_ANGLE;   break;
    default:
        angle = FF_BASE;
        Task3_Stop();
        App_Menu_ChangeRunning(false);
        break;
    }
    ZDT_MoveToClk(motor, ZDT_AngleToClk(angle));
}

/* ---- getters ---- */
bool     Task3_IsDone(void)    { return state >= S_DONE; }
uint32_t Task3_GetStep(void)   { return (uint32_t)state; }
float    Task3_GetTarget(void) { return 12.5f; }
float    Task3_GetCurrent(void){ return g_ball_pos * 0.1f; }  // 实时球位置(cm)
float    Task3_GetOutput(void) { return 0.0f; }
