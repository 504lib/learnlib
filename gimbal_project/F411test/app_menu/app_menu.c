#include "app_menu.h"
#include "main.h"
#include "multikey.h"
#include "oled.h"
#include "log.h"
#include "oled.h"
#include "ZDT_Motor_Serial.h"

static MenuMode  g_mode     = MENU_ZDT_TEST;
static bool      g_running  = false;


const char* names[] = { "ZDT_TEST", "BALL_PID" };

/* ---- 按键 ---- */
static uint8_t read_k2(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET ? 1 : 0; }
static uint8_t read_k3(MulitKey_t* k) { (void)k; return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET ? 1 : 0; }


static void on_k2(MulitKey_t* k) {
    (void)k;
    OLED_Clear();
    g_mode = (g_mode + 1) % MENU_COUNT;
}

static void on_k3(MulitKey_t* k) {
    (void)k;
    OLED_Clear();
    g_running = !g_running;
}

static MulitKey_t mk1, mk2, mk3;

/* ---- 初始化 ---- */
void App_Menu_Init(void)
{
    MulitKey_Init(&mk2, read_k2, on_k2, NULL, FALL_BORDER_TRIGGER);
    MulitKey_Init(&mk3, read_k3, on_k3, NULL, FALL_BORDER_TRIGGER);
}

/* ---- 每周期调用 ---- */
void App_Menu_Process(void)
{
    MulitKey_Scan(&mk2);
    MulitKey_Scan(&mk3);
}

/* ---- 获取状态 ---- */
MenuMode App_Menu_GetMode(void)  { return g_mode; }
bool     App_Menu_IsRunning(void) { return g_running; }
void     App_Menu_GetModeNameAndStatus(char* name,size_t buffer_size)
{
    LOG_Snprintf(name, buffer_size, "%s %s", names[g_mode], g_running ? "RUN" : "STOP");
}
