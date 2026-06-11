#include "key_control.h"
#include "ti_msp_dl_config.h"
#include "./SPI_OLED/oled.h"

#define DISPLAY_MODE_MAX  6   // 0~6 共7个页面

static MulitKey_t key1;
static MulitKey_t key2;
static MulitKey_t key4;

static uint8_t  display_mode = 0;
static volatile bool confirm_flag = false;

// ============================================================
// KEY1 (PB21) —— 预留
// ============================================================
static uint8_t Key1_ReadPin(MulitKey_t* key)
{
    (void)key;
    uint32_t raw = DL_GPIO_readPins(KEY_GROIP_PORT, KEY_GROIP_BOARD_KEY_PIN);
    return (uint8_t)(!!raw);
}

static void Key1_OnPressed(MulitKey_t* key)
{
    (void)key;
}

static void Key1_OnLongPressed(MulitKey_t* key)
{
    (void)key;
    DL_GPIO_togglePins(LED_PORT, LED_LED_2_PIN);
}

// ============================================================
// KEY2 (PB13) —— 切换显示模式
// ============================================================
static uint8_t Key2_ReadPin(MulitKey_t* key)
{
    (void)key;
    uint32_t raw = DL_GPIO_readPins(KEY_GROIP_PORT, KEY_GROIP_KEY2_PIN);
    return (uint8_t)(!!raw);
}

static void Key2_OnPressed(MulitKey_t* key)
{
    (void)key;
    display_mode = (display_mode + 1) % (DISPLAY_MODE_MAX + 1);
    OLED_Clear();
}

// ============================================================
// KEY4 (PB27) —— 确认/启动任务
// ============================================================
static uint8_t Key4_ReadPin(MulitKey_t* key)
{
    (void)key;
    uint32_t raw = DL_GPIO_readPins(KEY_GROIP_PORT, KEY_GROIP_KEY4_PIN);
    return (uint8_t)(!!raw);
}

static void Key4_OnPressed(MulitKey_t* key)
{
    (void)key;
    if (display_mode >= 3 && display_mode <= 6) {
        confirm_flag = true;
    }
}

// ============================================================
// 公共 API
// ============================================================
void KeyControl_Init(void)
{
    MulitKey_Init(&key1, Key1_ReadPin, Key1_OnPressed, Key1_OnLongPressed, FALL_BORDER_TRIGGER);
    MulitKey_Init(&key2, Key2_ReadPin, Key2_OnPressed, NULL,               FALL_BORDER_TRIGGER);
    MulitKey_Init(&key4, Key4_ReadPin, Key4_OnPressed, NULL,               FALL_BORDER_TRIGGER);
    display_mode = 0;
    confirm_flag = false;
}

void KeyControl_Scan(void)
{
    MulitKey_Scan(&key1);
    MulitKey_Scan(&key2);
    MulitKey_Scan(&key4);
}

uint8_t KeyControl_GetDisplayMode(void)
{
    return display_mode;
}

bool KeyControl_IsConfirmed(void)
{
    return confirm_flag;
}

void KeyControl_ClearConfirm(void)
{
    confirm_flag = false;
}
