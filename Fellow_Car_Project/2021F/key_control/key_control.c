#include "key_control.h"
#include "main.h"  // HAL_GPIO_ReadPin 和 LED 控制等
#include "oled.h"
#include "app_protocol.h"
#include "usart.h"  

#define CMD_DELIVERY_CONFIRM  0x04
#define CMD_KEY2_LONG         0x10   // 发给摄像头的key2长按信号（协议帧type）

// 按键对象（静态，仅本文件可见）
static MulitKey_t key1;
static MulitKey_t key2;
static MulitKey_t key3;
static MulitKey_t key4;

// 显示模式变量（静态，通过函数访问）
static uint8_t display_mode = 0;

// ------------------ key1按键回调函数
static uint8_t Key3_ReadPin(MulitKey_t* key)
{
    // 假设按键1接在 key1 引脚
    return HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin);
}

static void Key3_OnPressed(MulitKey_t* key)
{
    display_mode = (display_mode + 1) % 7;// 切换显示模式
    OLED_Clear();  
}

// ------------------ key2按键回调函数
static uint8_t Key4_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key4_GPIO_Port, key4_Pin);
}
//static uint8_t Key2_ReadPin(MulitKey_t* key)
//{
//    return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin);
//}
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

static void Key4_OnPressed(MulitKey_t* key)
{
    App_CmdEnqueue(CMD_DELIVERY_CONFIRM);
}

static void Key4_OnLongPressed(MulitKey_t* key)
{
    // 长按key4: 通过协议层发送 CMD_KEY2_LONG 给摄像头
    Uart_Protocol_Transmit_Frame(App_GetProtocolInstance(), NULL, CMD_KEY2_LONG, 0);
    LOG_INFO("Key4 long pressed, sent CMD_KEY2_LONG(0x%02X) to camera", CMD_KEY2_LONG);
}

static void Key3_OnLongPressed(MulitKey_t* key)
{
    // 长按翻转 LED
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

void KeyControl_Init(void)
{
    MulitKey_Init(&key3, Key3_ReadPin, Key3_OnPressed, Key3_OnLongPressed, RISE_BORDER_TRIGGER);
	MulitKey_Init(&key4, Key4_ReadPin, Key4_OnPressed, Key4_OnLongPressed, RISE_BORDER_TRIGGER);
}

void KeyControl_Scan(void)
{
    MulitKey_Scan(&key3);
	MulitKey_Scan(&key4);
}

uint8_t KeyControl_GetDisplayMode(void)
{
    return display_mode;
}
