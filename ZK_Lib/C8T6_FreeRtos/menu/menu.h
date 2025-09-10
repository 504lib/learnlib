#pragma once

#include "main.h"
#include "FreeRTOS.h"
#include "u8g2.h"
#include "stdbool.h"
#include <stdlib.h>
#include "cmsis_os2.h"

#define MENU_NODE 20

// #define 

typedef enum {
    MENU_TYPE_SUB_MENU,   // 子菜单
    MENU_TYPE_FUNCTION,   // 执行功能
    MENU_TYPE_PARAM_INT,  // 整型参数
    MENU_TYPE_PARAM_ENUM, // 枚举型参数
    MENU_TYPE_TOGGLE,     // 开关选项
    MENU_TYPE_DISPLAY     // 仅显示信息
} menu_item_type_t;

typedef struct menu_item_s menu_item_t;

typedef struct {
    menu_item_t* current_menu;    // 当前显示的菜单
    menu_item_t* selected_item;   // 当前选中的菜单项
    menu_item_t* first_visible;   // 当前显示的第一个菜单项
    uint8_t visible_count;        // 可见菜单项数量
} menu_data_t;

menu_item_t* create_submenu_item(const char* text);

void navigate_up(menu_data_t* menu_data);
void navigate_down(menu_data_t* menu_data);
void navigate_enter(menu_data_t* menu_data);
void navigate_back(menu_data_t* menu_data);

void Link_next_sibling(menu_item_t* current,menu_item_t* next);
void Link_Parent_Child(menu_item_t* parent, menu_item_t* child);

void show_menu(u8g2_t* u8g2, menu_data_t* menu_data, uint8_t max_display_count);
