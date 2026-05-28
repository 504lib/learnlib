#include "key_control.h"
#include "main.h"
#include "oled.h"
#include "app_protocol.h"
#include "usart.h"
#include "HSM_Core.h"
#include "app_fsm.h"

#define CMD_DELIVERY_CONFIRM  0x04

static volatile bool key2_start_flag = false;
static volatile bool system_reset_flag = false;

// 按键对象（静态）
static MulitKey_t key1;
static MulitKey_t key2;
static MulitKey_t key3;
static MulitKey_t key4;

static uint8_t display_mode = 0;

// ------------------ key1 按键回调（显示模式切换，原 key3 功能）--------------------
static uint8_t Key1_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin);
}

static void Key1_OnPressed(MulitKey_t* key)
{
    display_mode = (display_mode + 1) % 7;
    OLED_Clear();
}

static void Key1_OnLongPressed(MulitKey_t* key)
{
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

// ------------------ key2 按键回调（短按触发摄像头）--------------------
static uint8_t Key2_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin);
}

static void Key2_OnPressed(MulitKey_t* key)
{
//    key2_start_flag = true;
	// Uart_Protocol_Transmit_Frame(App_GetProtocolInstance_Cam(), NULL, CMD_KEY_TRIGGER, 0);
    App_SendCamFrame(CMD_KEY_TRIGGER, NULL, 0);
    HSM_SendEvent(App_FSM_GetHandle(),(HSM_Event_Package){.HSM_Event_ID = EV_KEY2_TRIGGER});
    HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
}

// ------------------ key3 按键回调（系统复位）--------------------
static uint8_t Key3_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin);
}

static void Key3_OnPressed(MulitKey_t* key)
{
    system_reset_flag = true;
    Uart_Protocol_Transmit_Frame(App_GetProtocolInstance_Cam(), NULL, CMD_RESET, 0);
    LOG_INFO("Key3 pressed -> send CMD_RESET(0x%02X) to camera and MCU", CMD_RESET);
}

// ------------------ key4 按键回调 --------------------
static uint8_t Key4_ReadPin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(key4_GPIO_Port, key4_Pin);
}

static void Key4_OnPressed(MulitKey_t* key)
{
    App_CmdEnqueue(CMD_DELIVERY_CONFIRM);
}

static void Key4_OnLongPressed(MulitKey_t* key)
{
    LOG_INFO("Key4 long pressed (no action, use key2 instead)");
}

bool KeyControl_IsStartTriggered(void) {
    return key2_start_flag;
}

void KeyControl_ClearStartTrigger(void) {
    key2_start_flag = false;
}

bool KeyControl_IsSystemReset(void) {
    return system_reset_flag;
}

void KeyControl_ClearSystemReset(void) {
    system_reset_flag = false;
}

void KeyControl_Init(void)
{
    MulitKey_Init(&key1, Key1_ReadPin, Key1_OnPressed, Key1_OnLongPressed, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key2, Key2_ReadPin, Key2_OnPressed, NULL, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key3, Key3_ReadPin, Key3_OnPressed, NULL, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key4, Key4_ReadPin, Key4_OnPressed, Key4_OnLongPressed, RISE_BORDER_TRIGGER);
}

void KeyControl_Scan(void)
{
    MulitKey_Scan(&key1);
    MulitKey_Scan(&key2);
    MulitKey_Scan(&key3);
    MulitKey_Scan(&key4);
}

uint8_t KeyControl_GetDisplayMode(void)
{
    return display_mode;
}
