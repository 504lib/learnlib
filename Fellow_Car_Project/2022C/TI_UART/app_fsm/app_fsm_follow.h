#pragma once

#include "../HSM/HSM_Core.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================
// 跟随车事件
// ============================================================
enum Follow_EventID {
    EV_BT_GO     = 0x20,   // 收到出发命令（载荷: 任务号）
    EV_BT_FORK   = 0x21,   // 收到岔路决策（载荷: 0=外圈, 1=内圈）
    EV_BT_STOP   = 0x22,   // 收到停止命令
    EV_FOLLOW_DONE = 0x23, // 跟随完成
};

// ============================================================
// API
// ============================================================
void Follow_FSM_Init(void);
void Follow_FSM_Run(void);
HSM*  Follow_FSM_GetHandle(void);
bool  Follow_IsRunning(void);
