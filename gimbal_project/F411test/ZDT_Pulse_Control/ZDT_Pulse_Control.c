#include "ZDT_Pulse_Control.h"

/*
 * CubeMX: PB8→TIM4_CH3(AF2), PB9→GPIO_Output, PA1→GPIO_Output
 *         TIM4→Internal Clock, PSC=0, ARR=999, CH3=PWM, NVIC enable
 * 接线(共阳): COM→VCC, PUL-→PB8, DIR-→PB9, EN-→PA1
 */

/* ---- 引脚 ---- */
#define STEP_PORT   GPIOB
#define STEP_PIN    GPIO_PIN_8
#define DIR_PORT    GPIOB
#define DIR_PIN     GPIO_PIN_9
#define EN_PORT     GPIOA
#define EN_PIN      GPIO_PIN_1
#define ALM_PORT    GPIOA
#define ALM_PIN     GPIO_PIN_6
#define PEND_PORT   GPIOA
#define PEND_PIN    GPIO_PIN_15

extern TIM_HandleTypeDef htim4;

/* 梯形加减速参数 */
static uint32_t cfg_start_freq  = 500;     // 起步频率 Hz  (31 RPM)
static uint32_t cfg_target_freq = 2000;    // 巡航频率 Hz (125 RPM)
static uint32_t cfg_ramp_steps  = 200;     // 加减速步数 (每段)

static volatile int32_t remaining;
static volatile int32_t position;
static volatile int32_t total_steps;       // 本次移动总步数
static volatile bool    busy;
static volatile bool    alarm;
static uint8_t          current_dir;

/* ---- 内部: 根据步数计算当前频率 ---- */
static uint32_t _calc_freq(int32_t done)
{
    int32_t left = remaining;
    uint32_t f;

    if (done < (int32_t)cfg_ramp_steps) {
        /* 加速段 */
        f = cfg_start_freq + (cfg_target_freq - cfg_start_freq) * done / cfg_ramp_steps;
    } else if (left < (int32_t)cfg_ramp_steps) {
        /* 减速段 */
        f = cfg_start_freq + (cfg_target_freq - cfg_start_freq) * left / cfg_ramp_steps;
    } else {
        f = cfg_target_freq;
    }
    if (f < 1500)  f = 1500;
    if (f > 160000) f = 160000;
    return f;
}

/* 应用频率到硬件, 每 20 步调一次减轻开销 */
static void _apply_freq(uint32_t f)
{
    static uint8_t skip = 0;
    if (++skip < 20) return;
    skip = 0;

    uint32_t arr = (100000000UL / f) - 1;
    if (arr > 65535) arr = 65535;
    __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, arr / 2);
}

/* ---- PWM 启停 ---- */
static void _start_pwm(void)
{
    if (alarm) return;
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    /* 先设起步频率 */
    {
        uint32_t arr = (100000000UL / cfg_start_freq) - 1;
        if (arr > 65535) arr = 65535;
        __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, arr / 2);
    }
    busy = true;
    HAL_TIM_PWM_Start_IT(&htim4, TIM_CHANNEL_3);
}

static void _stop_pwm(void)
{
    HAL_TIM_PWM_Stop_IT(&htim4, TIM_CHANNEL_3);
    busy = false;
}

/* ================================================================
   初始化
   ================================================================ */
void ZDT_Pulse_Init(void)
{
    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;

    g.Pin = DIR_PIN;
    HAL_GPIO_Init(DIR_PORT, &g);
    g.Pin = EN_PIN;
    HAL_GPIO_Init(EN_PORT, &g);

    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(EN_PORT,  EN_PIN,  GPIO_PIN_RESET);

    /* ALM 输入 */
    g.Mode = GPIO_MODE_IT_FALLING;
    g.Pull = GPIO_PULLUP;
    g.Pin  = ALM_PIN;
    HAL_GPIO_Init(ALM_PORT, &g);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    /* PEND 输入 (可选) */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = PEND_PIN;
    HAL_GPIO_Init(PEND_PORT, &g);

    /* 初始频率 (会被 SetFreq/SetRamp 覆盖) */
    ZDT_Pulse_SetFreq(cfg_target_freq);

    position    = 0;
    remaining   = 0;
    total_steps = 0;
    busy        = false;
    alarm       = false;
    current_dir = ZDT_PULSE_DIR_CW;
}

/* ================================================================
   使能
   ================================================================ */
void ZDT_Pulse_Enable(void)  { HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET); }
void ZDT_Pulse_Disable(void) { _stop_pwm(); HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET); }

/* ================================================================
   频率 & 斜坡
   ================================================================ */
void ZDT_Pulse_SetFreq(uint32_t freq_hz)
{
    if (freq_hz < 1500)  freq_hz = 1500;
    if (freq_hz > 160000) freq_hz = 160000;
    cfg_target_freq = freq_hz;
    cfg_start_freq  = freq_hz / 3;   // 起步 = 巡航的 1/3
    if (cfg_start_freq < 1500) cfg_start_freq = 1500;
}

void ZDT_Pulse_SetRamp(uint32_t start_hz, uint32_t cruise_hz, uint32_t ramp_len)
{
    cfg_start_freq  = start_hz;
    cfg_target_freq = cruise_hz;
    cfg_ramp_steps  = ramp_len;
}

void ZDT_Pulse_SetDir(uint8_t dir)
{
    current_dir = dir;
    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN,
        (dir == ZDT_PULSE_DIR_CW) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/* ================================================================
   移动
   ================================================================ */
void ZDT_Pulse_MoveRel(int32_t steps)
{
    if (steps == 0 || alarm) return;
    ZDT_Pulse_SetDir((steps > 0) ? ZDT_PULSE_DIR_CW : ZDT_PULSE_DIR_CCW);
    remaining   = (steps > 0) ? steps : -steps;
    total_steps = remaining;
    _start_pwm();
}

void ZDT_Pulse_MoveToClk(int32_t clk)
{
    int32_t delta = clk - position;
    ZDT_Pulse_MoveRel(delta);
}

/* ================================================================
   急停
   ================================================================ */
void ZDT_Pulse_Stop(void)
{
    _stop_pwm();
    remaining = 0;
}

/* ================================================================
   状态
   ================================================================ */
bool    ZDT_Pulse_IsDone(void)      { return !busy; }
bool    ZDT_Pulse_IsInPosition(void){ return HAL_GPIO_ReadPin(PEND_PORT, PEND_PIN) == GPIO_PIN_RESET; }
bool    ZDT_Pulse_IsAlarm(void)     { return alarm; }
void    ZDT_Pulse_ClearAlarm(void)  { alarm = false; }
int32_t ZDT_Pulse_GetPos(void)      { return position; }
void    ZDT_Pulse_SetPos(int32_t clk){ position = clk; }

/* ================================================================
   回调 — 每个脉冲计数 + 梯形加减速
   ================================================================ */
void ZDT_Pulse_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    (void)htim;
    if (!busy) return;

    if (current_dir == ZDT_PULSE_DIR_CW) position++;
    else                                 position--;

    remaining--;
    int32_t done = total_steps - remaining;
    _apply_freq(_calc_freq(done));

    if (remaining <= 0) {
        _stop_pwm();
        remaining = 0;
    }
}

/* ================================================================
   ALM 报警
   ================================================================ */
void ZDT_Pulse_EXTI_Callback(uint16_t pin)
{
    if (pin == ALM_PIN) {
        _stop_pwm();
        alarm = true;
    }
}
