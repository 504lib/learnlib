#pragma once

#include <stdint.h>
#include <stdbool.h>


#define MENU_USE_BARE_METAL_HAL
#if defined(MENU_USE_CMSIS_OS2)
    #include "cmsis_os2.h"
    #include "FreeRTOS.h"
    #define MULTIKEY_GET_TICK osKernelGetTickCount
#elif defined(MENU_USE_BARE_METAL_HAL)
    #include "main.h"
    #define MULTIKEY_GET_TICK HAL_GetTick
#elif defined(MENU_USE_CUSTOM)
    //请加入宏定义
    #if !defined(MULTIKEY_GET_TICK)
        #error "MENU_USE_CUSTOM is defined but the GetTick function are not defined! Please define _disable_interrupt_func and _enable_interrupt_func."
    #endif
#else
    #error "Please define the target platform!!!"
#endif

#if !defined(MULTIKEY_GET_TICK)
    #error "Atomic operation macros are not correctly defined! Please check the platform configuration."
#endif

typedef struct MulitKey_t MulitKey_t;
typedef void (*KeyPressdCallback)(MulitKey_t* key);
typedef void (*KeyLongPressdCallback)(MulitKey_t* key);
typedef uint8_t (*KeyReadPinCallback)(MulitKey_t* key);

typedef enum {
    KEY_IDLE = 0,
    KEY_DEBOUNCE,
    KEY_LONGPRESS,
    KEY_PRESSED,
} KeyState;

typedef enum{
    RISE_BORDER_TRIGGER = 0,
    FALL_BORDER_TRIGGER,
}BorderTrigger;

typedef struct{
    uint16_t Key_Debounce_Time;
    uint16_t Key_LongPress_Time;
    uint16_t Key_LongPress_Repeat_Time;
} MulitKey_Time_t;


struct MulitKey_t
{
    bool isEnable_LongPress_Repeat;
    BorderTrigger Border_trigger;
    KeyReadPinCallback readPin;
    KeyPressdCallback onPressed;
    KeyLongPressdCallback onLongPressed;
    KeyState state;
    MulitKey_Time_t time;
    uint32_t press_last_time;    // 按键开始时间戳
};



void MulitKey_Init(MulitKey_t* key,KeyReadPinCallback readPin,KeyPressdCallback onPressed,KeyLongPressdCallback onLongPressed,BorderTrigger trigger);
void MulitKey_Scan(MulitKey_t* key);
void MulitKey_EnableLongPressRepeat(MulitKey_t* key,bool enable);
void MulitKey_SetDebounceTime(MulitKey_t* key,uint16_t time);
void MulitKey_SetLongPressTime(MulitKey_t* key,uint16_t time);
void MulitKey_SetLongPressRepeatTime(MulitKey_t* key,uint16_t time);

