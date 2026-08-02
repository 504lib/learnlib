#include "app_menu.h"
#include "main.h"
#include "multikey.h"
#include "oled.h"
#include "log.h"
#include "Task_Control.h"

static MenuMode g_mode     = MENU_ZDT_TEST;
static bool     g_running[MENU_COUNT] = {0};

static const char* names[] = { "ZDT_TEST", "TASK3", "TASK4" };

/* ---- 按键 ---- */
static uint8_t read_k2(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET ? 1 : 0; }
static uint8_t read_k3(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET ? 1 : 0; }

static void on_k2(MulitKey_t* k) {
    (void)k;
    OLED_Clear();
    g_mode = (g_mode + 1) % MENU_COUNT;
}
static void on_k2_long(MulitKey_t* k) {
    (void)k;
    if (g_mode == MENU_TASK4)
        g_ball_target += 0.5f;   /* TASK4模式: 目标位置+0.5cm */
    else
        g_pos_offset  += 0.1f;   /* 其他模式: 校准偏移+0.1cm */
}

static void on_k3(MulitKey_t* k) {
    (void)k;
    OLED_Clear();
    g_running[g_mode] = !g_running[g_mode];
}
static void on_k3_long(MulitKey_t* k) {
    (void)k;
    if (g_mode == MENU_TASK4)
        g_ball_target -= 0.5f;   /* TASK4模式: 目标位置-0.5cm */
    else
        g_pos_offset  -= 0.1f;   /* 其他模式: 校准偏移-0.1cm */
}

static MulitKey_t mk2, mk3;

void App_Menu_Init(void)
{
    MulitKey_Init(&mk2, read_k2, on_k2, on_k2_long, FALL_BORDER_TRIGGER);
    MulitKey_Init(&mk3, read_k3, on_k3, on_k3_long, FALL_BORDER_TRIGGER);
    MulitKey_SetLongPressTime(&mk2, 1000);    /* 长按1秒才触发 */
    MulitKey_SetLongPressTime(&mk3, 1000);
}

void App_Menu_Process(void)
{
    MulitKey_Scan(&mk2);
    MulitKey_Scan(&mk3);
}

MenuMode App_Menu_GetMode(void)  { return g_mode; }
bool     App_Menu_IsRunning(void){ return g_running[g_mode]; }
void     App_Menu_Start(void)    { g_running[g_mode] = true; }
void     App_Menu_Stop(void)     { g_running[g_mode] = false; }

void App_Menu_GetModeNameAndStatus(char* name, size_t buffer_size)
{
    LOG_Snprintf(name, buffer_size, "%s %s", names[g_mode], g_running[g_mode] ? "RUN" : "STOP");
}
