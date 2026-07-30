#include "ZDT_Motor_Serial.h"

/* ----- 内部发送 ----- */
static void __send(ZDT_Motor_Handle_t *h, uint8_t *cmd, uint16_t len)
{
    if (h && h->Tx) h->Tx(cmd, len);
}

/* ================================================================
   初始化
   ================================================================ */
void ZDT_Init(ZDT_Motor_Handle_t *h, uint8_t addr,
              void (*Tx)(uint8_t *pData, uint16_t Size))
{
    if (!h) return;
    h->addr = addr;
    h->Tx   = Tx;
    h->vel           = 100;
    h->acc           = 5;
    h->prev_raw      = 0.0f;
    h->unwrap_inited = false;
    h->acc_clk       = 0;
}

/* ================================================================
   使能 / 去使能
   帧: 地址 + 0xF3 + 0xAB + 使能(0x01/0x00) + 同步 + 校验
   ================================================================ */
void ZDT_Enable(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0xF3, 0xAB, 0x01, 0x00, 0x6B };
    __send(h, cmd, 6);
}

void ZDT_Disable(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0xF3, 0xAB, 0x00, 0x00, 0x6B };
    __send(h, cmd, 6);
}

/* ================================================================
   绝对位置控制 (云台主力函数)
   帧: addr+0xFD+方向+速度(H+L)+加速度+脉冲(4B)+绝对标志+同步+校验
   ================================================================ */
void ZDT_MoveToAngle(ZDT_Motor_Handle_t *h, float raw_deg)
{
    if (!h) return;

    /* ±180°跳变展开：检测相邻帧跳变>180°则补偿±360° */
    if (!h->unwrap_inited) {
        h->prev_raw      = raw_deg;
        h->unwrap_inited = true;
    }
    float delta = raw_deg - h->prev_raw;
    if      (delta >  180.0f) delta -= 360.0f;
    else if (delta < -180.0f) delta += 360.0f;
    h->prev_raw   = raw_deg;

    /* 浮点累积 (避免整数截断漂移) */
    h->acc_deg_f += delta;
    ZDT_MoveToClk(h, ZDT_AngleToClk(h->acc_deg_f));
}

void ZDT_MoveToClk(ZDT_Motor_Handle_t *h, int32_t clk)
{
    if (!h) return;

    uint8_t dir = (clk >= 0) ? ZDT_DIR_CW : ZDT_DIR_CCW;
    if (clk < 0) clk = -clk;

    uint8_t cmd[13];
    cmd[0]  = h->addr;
    cmd[1]  = 0xFD;
    cmd[2]  = dir;
    cmd[3]  = (uint8_t)(h->vel >> 8);
    cmd[4]  = (uint8_t)(h->vel);
    cmd[5]  = h->acc;
    cmd[6]  = (uint8_t)((uint32_t)clk >> 24);
    cmd[7]  = (uint8_t)((uint32_t)clk >> 16);
    cmd[8]  = (uint8_t)((uint32_t)clk >> 8);
    cmd[9]  = (uint8_t)((uint32_t)clk);
    cmd[10] = 0x01;   // 绝对位置模式
    cmd[11] = 0x00;   // 不启用多机同步
    cmd[12] = 0x6B;

    __send(h, cmd, 13);
}

/* ================================================================
   速度模式
   帧: addr+0xF6+方向+速度(H+L)+加速度+同步+校验
   ================================================================ */
void ZDT_VelMode(ZDT_Motor_Handle_t *h, uint8_t dir, uint16_t vel)
{
    if (!h) return;
    uint8_t cmd[] = {
        h->addr, 0xF6,
        dir,
        (uint8_t)(vel >> 8), (uint8_t)(vel),
        h->acc,
        0x00,
        0x6B
    };
    __send(h, cmd, 8);
}

/* ================================================================
   立即停止: addr+0xFE+0x98+同步+校验
   ================================================================ */
void ZDT_Stop(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0xFE, 0x98, 0x00, 0x6B };
    __send(h, cmd, 5);
}

/* ================================================================
   当前位置清零: addr+0x0A+0x6D+校验
   ================================================================ */
void ZDT_ZeroPos(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0x0A, 0x6D, 0x6B };
    __send(h, cmd, 4);
}

/* ================================================================
   回零
   ================================================================ */
void ZDT_SetHomeOrigin(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0x93, 0x88, 0x00, 0x6B };
    __send(h, cmd, 5);
}

void ZDT_TriggerHome(ZDT_Motor_Handle_t *h, uint8_t mode)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0x9A, mode, 0x00, 0x6B };
    __send(h, cmd, 5);
}

void ZDT_AbortHome(ZDT_Motor_Handle_t *h)
{
    if (!h) return;
    uint8_t cmd[] = { h->addr, 0x9C, 0x48, 0x6B };
    __send(h, cmd, 4);
}

/* ================================================================
   参数设置
   ================================================================ */
void ZDT_SetVel(ZDT_Motor_Handle_t *h, uint16_t vel)
{
    if (h) h->vel = vel;
}

void ZDT_SetAcc(ZDT_Motor_Handle_t *h, uint8_t acc)
{
    if (h) h->acc = acc;
}
