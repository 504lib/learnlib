#ifndef __APP_FSM_H
#define __APP_FSM_H

#include "HSM_Core.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================
// 事件定义
// ============================================================
enum App_EventID {
    EV_CAM_START        = 0x01,   // 摄像头: 识别到数字 → 去 WaitKey2
    EV_TURN_LEFT        = 0x02,   // 左转指令
    EV_TURN_RIGHT       = 0x03,   // 右转指令
    EV_DELIVERY_CONFIRM = 0x04,   // 送药确认
    EV_TURN_DONE        = 0x10,   // 转弯/调头角度到位 (内部事件)
    EV_BT_GO            = 0x11,   // 蓝牙: 直接出发 → 去 GoToCross
	EV_KEY2_TRIGGER     = 0x12,   // 按键触发
    EV_RESET            = 0xFF,   // 系统复位
};

// ============================================================
// 动作类型（与命令码对应，用于 ActionStack）
// ============================================================
#define ACTION_STRAIGHT  0x01
#define ACTION_LEFT      0x02
#define ACTION_RIGHT     0x03

// ============================================================
// 公共 API
// ============================================================
void App_FSM_Init(void);
void App_FSM_Run(void);
bool App_FSM_IsRunning(void);
HSM* App_FSM_GetHandle(void);

#endif
