#ifndef __KEY_CONTROL_H
#define __KEY_CONTROL_H

#include "multikey.h"

// 初始化按键（在 main 中调用）
void KeyControl_Init(void);

// 按键扫描（在主循环中调用）
void KeyControl_Scan(void);

// 获取显示模式（供 OLED 显示使用）
uint8_t KeyControl_GetDisplayMode(void);

#endif
