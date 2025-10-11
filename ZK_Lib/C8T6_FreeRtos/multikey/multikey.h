#pragma once

#define KEY_DEBOUNCE_TIME 10      // 消抖时间，单位为扫描周期数
#define KEY_LONGPRESS_TIME 500   // 长按时间，单位为扫描周期数
#define KEY_LONGPRESS_REPEAT_TIME 100 // 长按重复触发时间，单位为扫描周期数


/**
 * @brief    选用平台
 * @warning  注意，一定要选用对应平台的tick函数，我这里是freertos的tick函数
 */
#define MENU_USE_CMSIS_OS2      


#if defined(MENU_USE_CMSIS_OS2)
    #include "cmsis_os2.h"
    #include "FreeRTOS.h"
    #define GetTick osKernelGetTickCount
#elif defined(MENU_USE_BARE_METAL)
    #include "main.h"
    #define GetTick HAL_GetTick
#elif defined(MENU_USE_CUSTOM)
    //请加入宏定义
    #if !defined(GetTick)
        #error "MENU_USE_CUSTOM is defined but the GetTick function are not defined! Please define _disable_interrupt_func and _enable_interrupt_func."
    #endif
#else
    #error "Please define the target platform!!!"
#endif

#if !defined(GetTick)
    #error "Atomic operation macros are not correctly defined! Please check the platform configuration."
#endif



typedef struct MulitKey_t MulitKey_t;


typedef void (*KeyPressdCallback)(MulitKey_t* key);             // 按键单按回调函数
typedef void (*KeyLongPressdCallback)(MulitKey_t* key);         // 按键长按回调函数
typedef uint8_t (*KeyReadPinCallback)(MulitKey_t* key);         // 按键状态回调函数

typedef enum {
    KEY_IDLE = 0,           // 按键空闲
    KEY_DEBOUNCE,           // 按键抖动
    KEY_LONGPRESS,          // 按键长按
    KEY_PRESSED,            // 按键长按中
} KeyState;

typedef enum{
    RISE_BORDER_TRIGGER = 0,            // 上升沿触发
    FALL_BORDER_TRIGGER,                // 下降沿触发
}BorderTrigger;


struct MulitKey_t
{
    BorderTrigger Border_trigger;           // 边沿触发方式
    KeyReadPinCallback readPin;             // 读取回调函数
    KeyPressdCallback onPressed;            // 按键单按回调函数
    KeyLongPressdCallback onLongPressed;    // 按键长按回调函数
    KeyState state;                         // 按键当前状态
    uint32_t press_last_time;               // 按键开始时间戳
};


/* ****************************** API *******************************/
void MulitKey_Init(MulitKey_t* key,KeyReadPinCallback readPin,KeyPressdCallback onPressed,KeyLongPressdCallback onLongPressed,BorderTrigger trigger);
void MulitKey_Scan(MulitKey_t* key);
void MulitKey_SetDebounceTime(uint16_t time);
void MulitKey_SetLongPressTime(uint16_t time);
void MulitKey_SetLongPressRepeatTime(uint16_t time);
