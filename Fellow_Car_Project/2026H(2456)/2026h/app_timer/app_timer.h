#ifndef __APP_TIMER_H__
#define __APP_TIMER_H__

#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "Log.h"

void App_Timer_Init(void);
void App_Timer_Start(void);
void App_Timer_Stop(void);
void App_Timer_Update(void);

bool App_Timer_IsRunning(void);
void App_Timer_GetString(char* buf, size_t len);   // "M:SS.CC"
uint32_t App_Timer_GetElapsedMs(void);

#endif
