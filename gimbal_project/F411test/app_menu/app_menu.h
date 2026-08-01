#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MENU_ZDT_TEST = 0,
    MENU_TASK3,
    MENU_TASK4,
    MENU_COUNT
} MenuMode;

void     App_Menu_Init(void);
void     App_Menu_Process(void);
MenuMode App_Menu_GetMode(void);

/* 每个模式独立运行状态 */
bool     App_Menu_IsRunning(void);             // 当前选中模式是否在跑
void     App_Menu_Start(void);                 // 启动当前模式
void     App_Menu_Stop(void);                  // 停止当前模式

void     App_Menu_GetModeNameAndStatus(char* name, size_t buffer_size);	
