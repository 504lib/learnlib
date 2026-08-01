#include "app_timer.h"
#include "app_log.h"
#include <stdio.h>

static uint32_t g_start_tick = 0;
static uint32_t g_elapsed_ms = 0;
static bool     g_running   = false;

void App_Timer_Init(void)
{
    g_start_tick = 0;
    g_elapsed_ms = 0;
    g_running    = false;
}

void App_Timer_Start(void)
{
    g_start_tick = HAL_GetTick();
    g_elapsed_ms = 0;
    g_running    = true;
}

void App_Timer_Stop(void)
{
    g_running = false;
}

void App_Timer_Update(void)
{
    if (!g_running) return;
    g_elapsed_ms = HAL_GetTick() - g_start_tick;
}

bool App_Timer_IsRunning(void)
{
    return g_running;
}

uint32_t App_Timer_GetElapsedMs(void)
{
    return g_elapsed_ms;
}

void App_Timer_GetString(char* buf, size_t len)
{
    // HAL_GetTick 溢出后回绕，但 32 位 ms 要 49 天才溢出，比赛不用管
    uint32_t total_ms = g_elapsed_ms;
    uint32_t total_s  = total_ms / 1000;
    uint8_t  min      = total_s / 60;
    uint8_t  sec      = total_s % 60;
    uint8_t  cs       = (total_ms % 1000) / 10;  // 百分秒 (10ms 精度)

    LOG_Snprintf(buf, len, "%01u:%02u.%02u", min, sec, cs);
}
