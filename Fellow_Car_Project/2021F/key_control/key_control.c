#include "key_control.h"
#include "main.h"  // 为了使用 HAL_GPIO_ReadPin 和 LED 控制等
#include "oled.h"
#include "app_protocol.h"

#define CMD_DELIVERY_CONFIRM  0x04

// 按键对象（静态，仅本文件可见）
static MulitKey_t key1;
static MulitKey_t key2;

// 显示模式变量（静态，通过函数访问）
static uint8_t display_mode = 0;

// ------------------ key1按键回调函数
static uint8_t Key1_ReadPin(MulitKey_t* key)
{
    // 假设按键1接在 key1 引脚
    return HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin);
}

static void Key1_OnPressed(MulitKey_t* key)
{
    display_mode = (display_mode + 1) % 7;// 切换显示模式
    OLED_Clear();  
}

// ------------------ key2按键回调函数
static uint8_t Key2_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin);
}

//static void Key2_OnPressed(MulitKey_t* key)
//{
//	HAL_Delay(100);
//	if (display_mode == 3)
//	{
//		SM_StartTask(TASK_1);
//	}
//	if (display_mode == 4)
//	{
//		SM_StartTask(TASK_2);
//	}
//	if (display_mode == 5)
//	{
//		SM_StartTask(TASK_3);
//	}
//	if (display_mode == 6)
//	{
//		SM_StartTask(TASK_4);
//	}	
//}

static void Key2_OnPressed(MulitKey_t* key)
{
    App_CmdEnqueue(CMD_DELIVERY_CONFIRM);
}

static void Key1_OnLongPressed(MulitKey_t* key)
{
    // 长按翻转 LED
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

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
