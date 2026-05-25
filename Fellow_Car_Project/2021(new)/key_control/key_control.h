#ifndef __KEY_CONTROL_H
#define __KEY_CONTROL_H

#include "multikey.h"
#include <stdbool.h>
//#include "f_sm.h"

#define CMD_KEY_TRIGGER  0x10   // key2 触发信号命令码
#define CMD_RESET        0x20   // key3 系统复位命令码

// 初始化按键（在 main 中调用）
void KeyControl_Init(void);

// 按键扫描（在主循环中调用）
void KeyControl_Scan(void);

// 获取显示模式（供 OLED 显示使用）
uint8_t KeyControl_GetDisplayMode(void);

// key2 启动标志（供 FSM 等待 key2 按下）
bool KeyControl_IsStartTriggered(void);
void KeyControl_ClearStartTrigger(void);

// key3 系统复位标志（供 FSM 和 camera 复位）
bool KeyControl_IsSystemReset(void);
void KeyControl_ClearSystemReset(void);

#endif
