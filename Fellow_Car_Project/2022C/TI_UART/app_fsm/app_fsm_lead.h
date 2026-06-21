#pragma once

#include "../HSM/HSM_Core.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================
// 领头车事件
// ============================================================
enum Lead_EventID {
    EV_KEY_CONFIRM        = 0x10,
    EV_ALL_BLACK          = 0x11,
    EV_FORK_APPEAR        = 0x12,
    EV_FORK_PASSED        = 0x13,
    EV_FINISH             = 0x15,
    EV_KEY_SWITCH         = 0xFF,
};

// ============================================================
// API
// ============================================================
void    Lead_FSM_Init(void);
void    Lead_FSM_Run(void);
HSM*    Lead_FSM_GetHandle(void);
uint8_t Lead_GetCurrentTask(void);
bool    Lead_IsRunning(void);
