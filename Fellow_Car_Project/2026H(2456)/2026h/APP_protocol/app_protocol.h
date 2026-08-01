#pragma once

#include "protocol.h"

/* 帧类型定义 */
typedef enum {
    APP_FRAME_BALL_POS   = 0x10,  // 球位置 (float cm)
    APP_FRAME_SYS_STATE  = 0x11,  // 系统状态 + 计时
    APP_FRAME_CMD        = 0x20,  // 指令
    APP_FRAME_SEND_VEL   = 0x21
} App_FrameType;

/* 系统状态 */
typedef enum {
    SYS_IDLE        = 0,
    SYS_LINE_FOLLOW = 1,
    SYS_BALANCE     = 2,
    SYS_STOP        = 3,
} SysState;

extern UART_protocol_t g_app_protocol;

bool App_Protocol_Init(void);
void App_Protocol_Loop(void);

/* 发送接口 */
bool App_Protocol_SendBallPos(float pos_cm);
bool App_Protocol_SendSysState(SysState state, uint32_t elapsed_ms);
bool App_Protocol_SendVel(float average_vel);

