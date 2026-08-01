#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* 细分 — 必须和驱动器菜单 MStep 一致 */
#define ZDT_PULSE_MICROSTEP   32
#define ZDT_PULSE_CLK_PER_REV (200 * ZDT_PULSE_MICROSTEP)   // 6400

/* 方向 */
#define ZDT_PULSE_DIR_CW   0
#define ZDT_PULSE_DIR_CCW  1

void ZDT_Pulse_Init(void);
void ZDT_Pulse_Enable(void);
void ZDT_Pulse_Disable(void);

/* speed: Hz (step frequency), range ~1.5k–160k */
void ZDT_Pulse_SetFreq(uint32_t freq_hz);
void ZDT_Pulse_SetRamp(uint32_t start_hz, uint32_t cruise_hz, uint32_t ramp_steps);
void ZDT_Pulse_SetDir(uint8_t dir);

/* 相对/绝对移动 (非阻塞) */
void ZDT_Pulse_MoveRel(int32_t steps);
void ZDT_Pulse_MoveToClk(int32_t clk);

/* 急停 + 状态 */
void     ZDT_Pulse_Stop(void);
bool     ZDT_Pulse_IsDone(void);         // 脉冲发完
bool     ZDT_Pulse_IsInPosition(void);   // 驱动到位信号 (PEND)
bool     ZDT_Pulse_IsAlarm(void);        // 驱动报警 (ALM)
void     ZDT_Pulse_ClearAlarm(void);
int32_t  ZDT_Pulse_GetPos(void);
void     ZDT_Pulse_SetPos(int32_t clk);

/* HAL 回调 */
void ZDT_Pulse_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void ZDT_Pulse_EXTI_Callback(uint16_t pin);  // 在 HAL_GPIO_EXTI_Callback 中调用

/* 角度换算 */
static inline int32_t ZDT_Pulse_AngleToClk(float deg) {
    return (int32_t)(deg * (float)ZDT_PULSE_CLK_PER_REV / 360.0f);
}
static inline float ZDT_Pulse_ClkToAngle(int32_t clk) {
    return (float)clk * 360.0f / (float)ZDT_PULSE_CLK_PER_REV;
}
