#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MENU_ZDT_TEST = 0,   // 按键手动控角度
    MENU_BALL_PID,       // 第三题: 0→+5→-5
    MENU_COUNT
} MenuMode;

void App_Menu_Init(void);
void App_Menu_Process(void);
MenuMode App_Menu_GetMode(void);
bool     App_Menu_IsRunning(void);
void     App_Menu_GetModeNameAndStatus(char* name,size_t buffer_size);
