#pragma once

#define MENU_USE_CMSIS_OS2     // 使用CMSIS-RTOS2 API
// #define MENU_USE_BARE_METAL    // 使用裸机系统(默认使用__disable_irq/__enable_irq)
// #define MENU_USE_CUSTOM        // 使用自定义原子操作函数

#include "main.h"
#include "u8g2.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


#if defined(MENU_USE_CMSIS_OS2)
    #include "cmsis_os2.h"
    #include "FreeRTOS.h"
    #define _disable_interrupt_func osKernelLock()
    #define _enable_interrupt_func osKernelUnlock();
#elif defined(MENU_USE_BARE_METAL)
    #define _disable_interrupt_func __disable_irq()
    #define _enable_interrupt_func __enable_irq()
#elif defined(MENU_USE_CUSTOM)
    //请加入宏定义
    #if !defined(_disable_interrupt_func) || !defined(_enable_interrupt_func)
        #error "MENU_USE_CUSTOM is defined but the custom atomic operation functions are not defined! Please define _disable_interrupt_func and _enable_interrupt_func."
    #endif
#else
    #error "Please define the target platform!!!"
#endif

#if !defined(_disable_interrupt_func) || !defined(_enable_interrupt_func)
    #error "Atomic operation macros are not correctly defined! Please check the platform configuration."
#endif

#define MENU_NODE 20


typedef enum {
    MENU_TYPE_SUB_MENU,   // 子菜单
    MENU_TYPE_FUNCTION,   // 执行功能
    MENU_TYPE_PARAM_INT,  // 整型参数
    MENU_TYPE_PARAM_ENUM, // 枚举型参数
    MENU_TYPE_TOGGLE,     // 开关选项
    MENU_TYPE_MAIN,         // 主界面
    MENU_TYPE_DISPLAY     // 仅显示信息
} menu_item_type_t;

typedef struct menu_item_s menu_item_t;

typedef struct menu_data_t menu_data_t;

/* ************* 初始化函数 ******************** */
menu_data_t* menu_data_init(menu_item_t* root);

/* **************** 菜单类型 *************************** */
menu_item_t* create_submenu_item(const char* text);//子菜单
menu_item_t* create_function_item(const char* text, void (*action_cb)(void));//回调菜单
menu_item_t* create_param_int_item(const char* text, int32_t* value_ptr, int min, int max, int step);//整形枚举菜单
menu_item_t* create_param_enum_item(const char* text,int32_t* value_ptr,const char** options,int options_nums);//字符串枚举菜单
menu_item_t* create_main_item(const char* text,menu_item_t* root,void (*main_display_cb)(u8g2_t* u8g2, menu_data_t* menu_data));
menu_item_t* create_toggle_item(const char* text,bool* value_ptr);//开关菜单

/* ********************* 按键映射 ****************************** */
void navigate_up(menu_data_t* menu_data);//上
void navigate_down(menu_data_t* menu_data);//下
void navigate_enter(menu_data_t* menu_data);//确认
void navigate_back(menu_data_t* menu_data);//返回

/* *********************** 连接函数 ********************************** */
void Link_next_sibling(menu_item_t* current,menu_item_t* next);
void Link_Parent_Child(menu_item_t* parent, menu_item_t* child);

/* ************************* 展示函数 ************************************ */
void show_menu(u8g2_t* u8g2, menu_data_t* menu_data, uint8_t max_display_count);
