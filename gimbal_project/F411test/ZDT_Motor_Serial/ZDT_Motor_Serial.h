#ifndef __ZDT_MOTOR_SERIAL_H__
#define __ZDT_MOTOR_SERIAL_H__

#include <stdint.h>
#include <stdbool.h>

/* 32细分下 6400 clk = 360° */
#define ZDT_MICROSTEP       32
#define ZDT_CLK_PER_REV     (200 * ZDT_MICROSTEP)

/* 回零模式 */
#define ZDT_HOME_NEAREST    0x00
#define ZDT_HOME_DIR        0x01
#define ZDT_HOME_COLLISION  0x02
#define ZDT_HOME_LIMIT_SW   0x03

#define ZDT_DIR_CW  0x00
#define ZDT_DIR_CCW 0x01

typedef struct
{
    uint8_t  addr;
    uint16_t vel;       // RPM
    uint8_t  acc;       // 0=直接启动, 1-255=曲线加减速

    float   prev_raw;       // 上一帧原始角度(内部用于±180°跳变展开)
    bool    unwrap_inited;   // 展开初始化标志
    float   acc_deg_f;       // 浮点累积绝对角度(°), 避免整数截断漂移
    int32_t acc_clk;        // 保留兼容

    void   (*Tx)(uint8_t *pData, uint16_t Size);
} ZDT_Motor_Handle_t;

// === 初始化 ===
void ZDT_Init(ZDT_Motor_Handle_t *h, uint8_t addr,
              void (*Tx)(uint8_t *pData, uint16_t Size));

// === 使能 ===
void ZDT_Enable (ZDT_Motor_Handle_t *h);
void ZDT_Disable(ZDT_Motor_Handle_t *h);

// === 位置控制（绝对模式）===
void ZDT_MoveToAngle(ZDT_Motor_Handle_t *h, float angle_deg);
void ZDT_MoveToClk  (ZDT_Motor_Handle_t *h, int32_t clk);

// === 速度模式 ===
void ZDT_VelMode(ZDT_Motor_Handle_t *h, uint8_t dir, uint16_t vel);

// === 立即停止 ===
void ZDT_Stop(ZDT_Motor_Handle_t *h);

// === 位置清零 ===
void ZDT_ZeroPos(ZDT_Motor_Handle_t *h);

// === 回零 ===
void ZDT_SetHomeOrigin(ZDT_Motor_Handle_t *h, bool save_to_flash);
void ZDT_TriggerHome  (ZDT_Motor_Handle_t *h, uint8_t mode);
void ZDT_AbortHome    (ZDT_Motor_Handle_t *h);

// === 参数 ===
void ZDT_SetVel(ZDT_Motor_Handle_t *h, uint16_t vel);
void ZDT_SetAcc(ZDT_Motor_Handle_t *h, uint8_t acc);

// === 工具 ===
static inline int32_t ZDT_AngleToClk(float deg)
    { return (int32_t)(deg * (float)ZDT_CLK_PER_REV / 360.0f); }

static inline float ZDT_ClkToAngle(int32_t clk)
    { return (float)clk * 360.0f / (float)ZDT_CLK_PER_REV; }

#endif
