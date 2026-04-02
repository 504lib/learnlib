#include "key_control.h"
#include "main.h"  // 为了使用 HAL_GPIO_ReadPin 和 LED 控制等
#include "oled.h"

// 按键对象（静态，仅本文件可见）
static MulitKey_t key1;

// 显示模式变量（静态，通过函数访问）
static uint8_t display_mode = 0;

// 按键回调函数（声明为 static）
static uint8_t Key1_ReadPin(MulitKey_t* key)
{
    // 假设按键1接在 key1 引脚
    return HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin);
}

static void Key1_OnPressed(MulitKey_t* key)
{
    // 切换显示模式
    display_mode = (display_mode + 1) % 3;
    OLED_Clear();  // 清屏，下次刷新会显示新内容
}

static void Key1_OnLongPressed(MulitKey_t* key)
{
    // 长按翻转 LED
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

void KeyControl_Init(void)
{
    MulitKey_Init(&key1, Key1_ReadPin, Key1_OnPressed, Key1_OnLongPressed, RISE_BORDER_TRIGGER);
}

void KeyControl_Scan(void)
{
    MulitKey_Scan(&key1);
}

uint8_t KeyControl_GetDisplayMode(void)
{
    return display_mode;
}
