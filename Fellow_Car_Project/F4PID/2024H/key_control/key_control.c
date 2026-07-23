#include "key_control.h"
#include "main.h"
#include "oled.h"
#include "control.h"

// 按键对象
static MulitKey_t key1;
static MulitKey_t key2;

// 显示模式（0-7，共8个模式）
static uint8_t display_mode = 0;

// ========== key1 回调 ==========
static uint8_t Key1_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin);
}

static void Key1_OnPressed(MulitKey_t* key)
{
    display_mode = (display_mode + 1) % 8;  // 0-7循环
    OLED_Clear();
}

static void Key1_OnLongPressed(MulitKey_t* key)
{
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

// ========== key2 回调 ==========
static uint8_t Key2_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin);
}

static void Key2_OnPressed(MulitKey_t* key)
{
    HAL_Delay(100);  // 消抖

    switch (display_mode) {
        case 3:
            // 如果已经在 SPEED_ONLY，再按一次回到 IDLE
            if (current_cascade_stage == CASCADE_SPEED_ONLY)
                current_cascade_stage = CASCADE_IDLE;
            else
                current_cascade_stage = CASCADE_SPEED_ONLY;
            break;

        case 4:
            if (current_cascade_stage == CASCADE_ANGLE_ONLY)
                current_cascade_stage = CASCADE_IDLE;
            else
                current_cascade_stage = CASCADE_ANGLE_ONLY;
            break;

        case 5:
            if (current_cascade_stage == CASCADE_SPEED_GRAY)
                current_cascade_stage = CASCADE_IDLE;
            else
                current_cascade_stage = CASCADE_SPEED_GRAY;
            break;

        case 6:
            if (current_cascade_stage == CASCADE_SPEED_ANGLE)
                current_cascade_stage = CASCADE_IDLE;
            else
                current_cascade_stage = CASCADE_SPEED_ANGLE;
            break;

        case 7:
            // STOP/IDLE 模式，强制停止
            current_cascade_stage = CASCADE_IDLE;
            break;

        default:
            break;
    }
}

// ========== 公共接口 ==========
void KeyControl_Init(void)
{
    MulitKey_Init(&key1, Key1_ReadPin, Key1_OnPressed, Key1_OnLongPressed, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key2, Key2_ReadPin, Key2_OnPressed, Key2_OnPressed, RISE_BORDER_TRIGGER);
}

void KeyControl_Scan(void)
{
    MulitKey_Scan(&key1);
    MulitKey_Scan(&key2);
}

uint8_t KeyControl_GetDisplayMode(void)
{
    return display_mode;
}
