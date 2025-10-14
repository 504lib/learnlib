/**
 * @file menu.c
 * @author whyP762 (3046961251@qq.com)
 * @brief 基于u8g2的菜单结构
 * @version 0.1
 * @date 2025-09-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "menu.h"

//记录当前的节点数量
static int menu_node_count = 0;

struct menu_item_s {
    // 基本属性
    const char* text;           // 显示文本
    uint8_t sub_menu_count;     // 子菜单数量
    menu_item_type_t type;      // 菜单类型
    
    // 导航指针
    menu_item_t* parent;        // 父节点
    menu_item_t* first_child;   // 第一个子节点
    menu_item_t* last_child;    // 最后一个子节点
    menu_item_t* next_sibling;  // 下一个兄弟节点
    menu_item_t* prev_sibling;  // 上一个兄弟节点
    
    // 类型相关的数据和回调
    union {
        // 对于功能和子菜单
        void (*action_cb)(void); // 功能回调
        
        // 对于参数
        struct {
            int temp;           // 临时变量，用于存储当前编辑的值
            int* value_ptr;      // 参数值指针
            int min;             // 最小值
            int max;             // 最大值
            int step;            // 步进值
        } param_int;
        
        struct {
            int value_ptr_temp; // 临时变量，用于存储当前编辑的值
            int* value_ptr;      // 当前值指针
            const char** options;// 选项字符串数组
            int option_count;    // 选项数量
        } param_enum;
        
        struct {
            bool temp;            // 临时变量，用于存储当前编辑的值
            bool* value_ptr;     // 开关状态指针
        } toggle;
        struct {
           void (*main_display_cb)(u8g2_t* u8g2, menu_data_t* menu_data);
        }main;
        
    } data;

    // 可选的回调函数
    void (*on_enter)(menu_item_t* item);    // 进入该项时调用
    void (*on_leave)(menu_item_t* item);    // 离开该项时调用
    void (*on_change)(menu_item_t* item);   // 值改变时调用
};

typedef struct
{
    bool isAmation;
    uint8_t start_x;
    uint8_t start_y;
    uint8_t start_heigh;
    uint8_t start_width;
    uint8_t current_x;
    uint8_t current_y;
    uint8_t current_heigh;
    uint8_t current_width;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t target_heigh;
    uint8_t target_width;
    bool update_flag;
}Animation;


struct menu_data_t{
    bool isSelectedParam; // 当前选中项是否为参数编辑状态
    menu_item_t* current_menu;    // 当前显示的菜单
    menu_item_t* selected_item;   // 当前选中的菜单项
    menu_item_t* first_visible;   // 当前显示的第一个菜单项
    uint8_t visible_count;        // 可见菜单项数量
    Animation animation;
};

//静态内存节点池
static menu_item_t Menu_Node[MENU_NODE] = {0};

/// @brief 全局唯一静态数据结构体
static menu_data_t menu_data = {0};

/**
 * @brief 选定框执行策略函数
 * 
 * @param u8g2 u8g2统一句柄
 * @param menu_data 数据结构体指针
 * @param x 选定框的x
 * @param y 选定框的y
 * @param item 所选节点
 * @param isSelected 是否执行了选中策略
 */
static void adjust_type_visible(u8g2_t* u8g2,menu_data_t* menu_data,uint16_t x, uint16_t y, menu_item_t* item,bool isSelected)
{
	char buffer[20];//暂时的缓冲区
    uint16_t str_width = 0;//当前子选项字符长度的大小变量
    if (isSelected && !menu_data->isSelectedParam)
    {
        menu_data->animation.target_y = y - 10;
        menu_data->animation.target_x = x;
        // menu_data->animation.target_heigh = 15;
        // menu_data->animation.target_width = 128;
    }
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);//字体
    uint16_t item_text_width = u8g2_GetStrWidth(u8g2, item->text);
    int16_t offset_x = menu_data->animation.target_x - menu_data->animation.current_x;
    int16_t offset_y = menu_data->animation.target_y - menu_data->animation.current_y;
    int16_t offset_heigh = menu_data->animation.target_heigh - menu_data->animation.current_heigh;
    int16_t offset_width = menu_data->animation.target_width - menu_data->animation.current_width;
    if (menu_data->animation.isAmation && isSelected)
    {
            if ((offset_heigh <= 2 && offset_heigh >= -2) && (offset_y <=2 && offset_y >= -2) && (offset_x <= 2 && offset_x >= -2) && (offset_width <= 2 && offset_width >= -2))
            {
                menu_data->animation.current_heigh = menu_data->animation.target_heigh;
                menu_data->animation.current_x = menu_data->animation.target_x;
                menu_data->animation.current_y = menu_data->animation.target_y;
                menu_data->animation.current_width = menu_data->animation.current_width;
                menu_data->animation.isAmation = false;
            }
            // 线性插值
            if (menu_data->animation.update_flag)
            {
                if (!(offset_y <= 2 && offset_y >= -2))
                {
                    menu_data->animation.current_y = menu_data->animation.current_y + 2 * (offset_y > 0? 1 : -1);
                }
                if (!(offset_x <= 2 && offset_x >= -2))
                {
                    menu_data->animation.current_x = menu_data->animation.current_x + 10 * (offset_x > 0? 1 : -1);
                }
                if (!(offset_heigh <= 2 && offset_heigh >= -2))
                {
                    menu_data->animation.current_heigh = menu_data->animation.current_heigh + (offset_heigh > 0? 1 : -1);
                }
                if (!(offset_width <= 2 && offset_width >= -2))
                {
                    menu_data->animation.current_width = menu_data->animation.current_width + 1 * (offset_width > 0? 1 : -1);
                }
                menu_data->animation.update_flag = false;
                if (item->type == MENU_TYPE_TOGGLE && isSelected)
                {

                    LOG_INFO("cur_y:%d,target_y:%d",menu_data->animation.current_y,menu_data->animation.target_y);
                    LOG_INFO("cur_x:%d,target_x:%d",menu_data->animation.current_x,menu_data->animation.target_x);
                LOG_INFO("cur_heigh:%d,target_heigh:%d",menu_data->animation.current_heigh,menu_data->animation.target_heigh);
                LOG_INFO("cur_width:%d,target_width:%d",menu_data->animation.current_width,menu_data->animation.target_width);
                }
                
                
             }
    }
    switch (item->type)
    {
    case MENU_TYPE_SUB_MENU://菜单选项
        if(isSelected)//选中
        {
            menu_data->animation.target_x = 0;
            menu_data->animation.target_y = y - 10;
            menu_data->animation.target_heigh = 15;
            menu_data->animation.target_width = 128;
            //必须先画框再写字，不然渲染有错误
            u8g2_DrawBox(u8g2, menu_data->animation.target_x,menu_data->animation.current_y, 128, 15);
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawStr(u8g2, x, y, item->text);
            u8g2_SetDrawColor(u8g2, 1);
        } 
        else //未选中
        {
            u8g2_DrawStr(u8g2, x, y, item->text);
        }
        break;
    case MENU_TYPE_FUNCTION://函数选项
        if(isSelected)//选中
        {
            menu_data->animation.target_heigh = 15;
            menu_data->animation.target_width = 128;
            //必须先画框再写字，不然渲染有错误
            u8g2_DrawBox(u8g2, menu_data->animation.target_x,menu_data->animation.current_y, 128, 15);
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawStr(u8g2, x, y, item->text);
            u8g2_SetDrawColor(u8g2, 1);
        } 
        else //未选中
        {
            u8g2_DrawStr(u8g2, x, y, item->text);
        }
        break;
    case MENU_TYPE_PARAM_INT://整形枚举选项
        sprintf(buffer, "%d",*(item->data.param_int.value_ptr));//格式化字符串数据
        str_width = u8g2_GetStrWidth(u8g2, buffer);//计算字符串数据所占的像素数
        if(isSelected)//选中
        {
            menu_data->animation.target_heigh = 15;
            menu_data->animation.target_width = 128;
            if(menu_data->isSelectedParam)//选中并执行选参
            {
                // 进入参数编辑状态，显示不同的样式
                //OLED屏幕宽度约为128，但是我用的120是比较观察和好看
                //首先要让选中框聚焦于数字枚举的内容，所以要在120-字符串宽度的位置开始渲染，渲染宽度就为字符串的宽度
                sprintf(buffer, "%d",item->data.param_int.temp);//格式化字符串数据
                str_width = u8g2_GetStrWidth(u8g2, buffer);//计算字符串数据所占的像素数
                u8g2_DrawBox(u8g2, 120 - str_width, y - 10, str_width, 15);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);

            }
            else//未选中
            {
                // 普通选中状态
                // menu_data->animation.target_heigh = 15;
                // menu_data->animation.target_width = 128;
            //必须先画框再写字，不然渲染有错误
                u8g2_DrawBox(u8g2, menu_data->animation.target_x,menu_data->animation.current_y, 128, 15);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);
            }
        } 
        else //未选中
        {
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
        }
        break;
    case MENU_TYPE_PARAM_ENUM://字符串枚举
        // menu_data->animation.target_heigh = 15;
        // menu_data->animation.target_width = 128;
        sprintf(buffer, "%s",item->data.param_enum.options[*(item->data.param_enum.value_ptr)]);//格式化索引所在的字符串
        str_width = u8g2_GetStrWidth(u8g2, buffer);//计算长度
        // u8g2_DrawStr(u8g2, x, y, buffer);
        if(isSelected)
        {
            if(menu_data->isSelectedParam)
            {
                // 进入参数编辑状态，显示不同的样式
                sprintf(buffer, "%s",item->data.param_enum.options[item->data.param_enum.value_ptr_temp]);//格式化索引所在的字符串
                str_width = u8g2_GetStrWidth(u8g2, buffer);//计算长度
                u8g2_DrawBox(u8g2, 120 - str_width, y - 10, str_width, 15);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);
            }
            else
            {
                // 普通选中状态
                // menu_data->animation.target_heigh = 15;
                // menu_data->animation.target_width = 128;
                //必须先画框再写字，不然渲染有错误
                u8g2_DrawBox(u8g2, menu_data->animation.target_x,menu_data->animation.current_y, 128, 15);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);
            }
        } 
        else 
        {
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
        }
        break;
    case MENU_TYPE_TOGGLE://开关菜单
        sprintf(buffer, "%s",(*(item->data.toggle.value_ptr) ? "true" : "false"));//根据数值直接渲染字符串还
        str_width = u8g2_GetStrWidth(u8g2, buffer);
        if(isSelected)
        {
            if(menu_data->isSelectedParam)
            {
                // LOG_INFO("join_target_width:%d",menu_data->animation.target_width);
                sprintf(buffer, "%s",(item->data.toggle.temp ? "true" : "false"));//根据数值直接渲染字符串还
                str_width = u8g2_GetStrWidth(u8g2, buffer);
                menu_data->animation.target_x = 120 - str_width;
                menu_data->animation.target_y = y - 10;
                menu_data->animation.target_heigh = 15;
                menu_data->animation.target_width = str_width;
                //必须先画框再写字，不然渲染有错误
                u8g2_DrawBox(u8g2, menu_data->animation.current_x,menu_data->animation.current_y, menu_data->animation.current_width, menu_data->animation.current_heigh);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);
            }
            else
            {
                // 普通选中状态
                menu_data->animation.target_heigh = 15;
                menu_data->animation.target_width = 128;
                u8g2_DrawBox(u8g2, menu_data->animation.current_x,menu_data->animation.current_y,menu_data->animation.current_width, menu_data->animation.current_heigh);
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
                u8g2_SetDrawColor(u8g2, 1);
            }
        } 
        else 
        {
                u8g2_DrawStr(u8g2, x, y, item->text);
                u8g2_DrawStr(u8g2, 120 - str_width, y, buffer); // 只显示数值部分
        }
        break;

    default:
        break;
    }
}

/**
 * @brief 菜单初始化
 * @attention 一定要先执行节点的父子兄弟联系后，才可以初始化，否则会出现节点初始化错误的情况
 * @param root 根节点，作为第一个节点
 * @return menu_data_t* 返回数据结构体指针
 */
menu_data_t* menu_data_init(menu_item_t* root)
{
    if (root == NULL) 
    {
        return NULL;
    }
    menu_data.isSelectedParam = false;//选中参数类型判断
    menu_data.current_menu = root;//当前所在的节点
    menu_data.selected_item = root->first_child;//默认选中第一个孩子
    menu_data.first_visible = root->first_child;//默认让第一个孩子在首置位显示
    menu_data.visible_count = 0;//默认屏幕可见的子菜单数为0
    menu_data.animation.start_heigh = 0;
    menu_data.animation.start_width = 0;
    menu_data.animation.start_x = 0;
    menu_data.animation.start_y = 0;
    menu_data.animation.isAmation = false;
    menu_data.animation.target_heigh = 0;
    menu_data.animation.target_width = 0;
    menu_data.animation.target_x = 0;
    menu_data.animation.target_y = 0;
    menu_data.animation.update_flag = false;
    return &menu_data;
}

/**
 * @brief Create a submenu item object
 * 
 * @param text 菜单名称
 * @return menu_item_t* 节点指针 
 */
menu_item_t* create_submenu_item(const char* text,void (*on_enter)(menu_item_t* item),void (*on_leave)(menu_item_t* item))
{
    if (menu_node_count >= MENU_NODE) {
        return NULL; // 超过最大节点数
    }
    //分配节点
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_SUB_MENU;
    item->on_enter = on_enter;
    item->on_leave = on_leave;

    return item;
}

/**
 * @brief Create a function item object
 * 
 * @param text 菜单名称
 * @param action_cb 回调函数，当执行enter操作触发
 * @return menu_item_t* 节点指针
 */
menu_item_t* create_function_item(const char* text, void (*action_cb)(void))
{
    if (menu_node_count >= MENU_NODE) {
        return NULL; // 超过最大节点数
    }
    //分配节点
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_FUNCTION;
    item->data.action_cb = action_cb;
    return item;
}

/**
 * @brief Create a param int item object
 * 
 * @param text 菜单名称
 * @param value_ptr 整形数值的地址
 * @param min 数值最小值
 * @param max 数值最大值
 * @param step 步进值，上/下所增/减的数值
 * @return menu_item_t* 节点指针
 */
menu_item_t* create_param_int_item(const char* text, int32_t* value_ptr, int min, int max, int step)
{
    if (menu_node_count >= MENU_NODE || value_ptr == NULL) {
        return NULL; // 超过最大节点数
    }
    //分配内存
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_PARAM_INT;
    item->data.param_int.value_ptr = value_ptr;
    item->data.param_int.min = min;
    item->data.param_int.max = max;
    item->data.param_int.step = step;
    return item;
}

/**
 * @brief Create a param enum item object
 * 
 * @param text 菜单文本
 * @param value_ptr 索引地址
 * @param options 字符串数组指针
 * @param options_nums 选项的数量
 * @return menu_item_t* 
 */
menu_item_t* create_param_enum_item(const char* text,int32_t* value_ptr,const char** options,int options_nums)
{
    if(menu_node_count >= MENU_NODE || value_ptr == NULL || options == NULL || options_nums < 0)
    {
        return NULL;
    }
    //分配内存
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_PARAM_ENUM;
    item->data.param_enum.value_ptr = value_ptr;
    item->data.param_enum.options = options;
    item->data.param_enum.option_count = options_nums;
    return item;

}


/**
 * @brief Create a toggle item object
 * 
 * @param text 菜单文本
 * @param value_ptr 开关值指针
 * @return menu_item_t* 节点指针
 */
menu_item_t* create_toggle_item(const char* text,bool* value_ptr)
{
    if(menu_node_count >= MENU_NODE)
    {
        return NULL;
    }
    //分配内存
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_TOGGLE;
    item->data.toggle.value_ptr = value_ptr;
    return item;

}

menu_item_t* create_main_item(const char* text,menu_item_t* root,void (*main_display_cb)(u8g2_t* u8g2, menu_data_t* menu_data))
{
    if (menu_node_count >= MENU_NODE) {
        return NULL; // 超过最大节点数
    }
    //分配节点
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_MAIN;
    item->first_child = root;
    item->first_child->parent = item;
    item->data.main.main_display_cb = main_display_cb;
    return item;
}

/**
 * @brief 连接父子节点函数
 * @attention 若父节点无子节点连接，直接作为第一子节点。若有子节点，作为最后子节点
 * @param parent 父节点
 * @param child 子节点
 */
void Link_Parent_Child(menu_item_t* parent, menu_item_t* child)
{
    if (parent == NULL || child == NULL) 
    {
        return;
    }
    child->parent = parent;
    parent->sub_menu_count++;
    if (parent->first_child == NULL) //无子节点，直接首尾指针都指向这个子节点
    {
        parent->first_child = child;
        parent->last_child = child;
    } 
    else //否则进行最后节点指针调换
    {
        parent->last_child->next_sibling = child;
        child->prev_sibling = parent->last_child;
        parent->last_child = child;
    }
}

/**
 * @brief 连接兄弟节点
 * @attention next节点的父节点会直接跟随当前节点的父节点
 * @param current 当前节点
 * @param next 要与之连接的节点
 */
void Link_next_sibling(menu_item_t* current,menu_item_t* next)
{
    if (current == NULL || next == NULL) 
    {
        return;
    }
    current->next_sibling = next;
    next->prev_sibling = current;
    if(current->parent)
    {
        next->parent = current->parent; // 兄弟节点的父节点相同
        current->parent->sub_menu_count++;
        if(current->parent->last_child == current)
        {
            current->parent->last_child = next; // 更新父节点的最后一个子节点
        }
    }
}

/**
 * @brief 菜单展示函数
 * @attention 关键函数，菜单的显示基于当前函数
 * @param u8g2 u8g2句柄
 * @param menu_data 数据结构体句柄
 * @param max_display_count 屏幕可展示最多的节点数量
 */
void show_menu(u8g2_t* u8g2, menu_data_t* menu_data, uint8_t max_display_count) {
    _disable_interrupt_func;//原子操作启动
    if(menu_data->current_menu && menu_data->current_menu->type == MENU_TYPE_MAIN)
    {
        //清除缓冲区，让屏幕重新绘制
        u8g2_ClearBuffer(u8g2);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        if(menu_data->current_menu->data.main.main_display_cb)
        {
            menu_data->current_menu->data.main.main_display_cb(u8g2,menu_data);
        }
        else
        {
            u8g2_DrawStr(u8g2, 0, 32, "No main_display_cb");
        }
        //显示当前菜单标题
        u8g2_SendBuffer(u8g2);//绘制菜单
        _enable_interrupt_func;//结束原子操作
        return;
    }
    if (!menu_data->current_menu || !menu_data->current_menu->first_child) //如果当前菜单和子节点一个没有，直接返回
    {
        _enable_interrupt_func;
        return;
    }
    
    // 如果没有选中的项，选择第一项
    if (!menu_data->selected_item) 
    {
        menu_data->selected_item = menu_data->current_menu->first_child;
    }
    
    // 如果没有设置first_visible，设置为第一项
    if (!menu_data->first_visible) 
    {
        menu_data->first_visible = menu_data->current_menu->first_child;
    }
    //清除缓冲区，让屏幕重新绘制
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);

    // 显示当前菜单标题
    u8g2_DrawStr(u8g2, 32, 7, menu_data->current_menu->text);

    // 显示菜单项
    menu_item_t* item = menu_data->first_visible;
    uint8_t count = 0;
    int y = 20;
   
    
    while (item && count < max_display_count) 
    {
        if (item == menu_data->selected_item) 
        {
            adjust_type_visible(u8g2,menu_data,0,y, item,true);
        } 
        else 
        {
            adjust_type_visible(u8g2,menu_data,0, y, item,false);
        }
    

        y += 15;
        item = item->next_sibling;
        count++;
    }
    
    menu_data->visible_count = count;
    u8g2_SendBuffer(u8g2);//绘制菜单
    _enable_interrupt_func;//结束原子操作
}

/**
 * @brief 向上浏览操作
 * 
 * @param menu_data 菜单数据句柄
 */
void navigate_up(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item) return;
//    int32_t* value_ptr = NULL;
    menu_data->animation.isAmation = true;
    if(menu_data->isSelectedParam)//枚举菜单被选中后，执行数值映射
    {
        if(menu_data->selected_item->type == MENU_TYPE_PARAM_INT)//整形菜单
        {
            // 增加参数值
            // if(value_ptr)
            // {
                menu_data->selected_item->data.param_int.temp += menu_data->selected_item->data.param_int.step;//步进值递增
                if(menu_data->selected_item->data.param_int.temp > menu_data->selected_item->data.param_int.max)//下限
                {
                    menu_data->selected_item->data.param_int.temp = menu_data->selected_item->data.param_int.max;
                }
                return;
        }
        else if (menu_data->selected_item->type == MENU_TYPE_PARAM_ENUM)//字符串枚举菜单
        {
            // if(value_ptr)
            // {
                menu_data->selected_item->data.param_enum.value_ptr_temp += 1;
                if(menu_data->selected_item->data.param_enum.value_ptr_temp > menu_data->selected_item->data.param_enum.option_count - 1)//下限
                {
                    menu_data->selected_item->data.param_enum.value_ptr_temp = menu_data->selected_item->data.param_enum.option_count - 1;
                }
                return;
        }
        else if(menu_data->selected_item->type == MENU_TYPE_TOGGLE)//开关节点
        {
            menu_data->selected_item->data.toggle.temp ^= 1;//取反
            return;
        }

    }
    // 移动到上一个兄弟节点
    if (menu_data->selected_item->prev_sibling) //前提一定有上兄弟节点才能浏览
    {
        menu_data->selected_item = menu_data->selected_item->prev_sibling;
        
        // 如果选中的项不在可见范围内，调整first_visible
        if (menu_data->selected_item == menu_data->first_visible->prev_sibling) 
        {
            menu_data->first_visible = menu_data->selected_item;
        }
    }
}

/**
 * @brief 向下浏览操作
 * 
 * @param menu_data 菜单数据句柄
 */
void navigate_down(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item) return;
    
    menu_data->animation.isAmation = true;
    if(menu_data->isSelectedParam)//枚举菜单被选中后，执行数值映射
    {
        if(menu_data->selected_item->type == MENU_TYPE_PARAM_INT)//整形菜单
        {
            // 增加参数值
            // if(value_ptr)
            // {
                menu_data->selected_item->data.param_int.temp -= menu_data->selected_item->data.param_int.step;//步进值递增
                if(menu_data->selected_item->data.param_int.temp < menu_data->selected_item->data.param_int.min)//下限
                {
                    menu_data->selected_item->data.param_int.temp = menu_data->selected_item->data.param_int.min;
                }
                return;
            // }
        }
        else if(menu_data->selected_item->type == MENU_TYPE_PARAM_ENUM)//字符串枚举菜单
        {
            // 增加参数值
            // if(value_ptr)
            // {
                menu_data->selected_item->data.param_enum.value_ptr_temp -= 1;
                if(menu_data->selected_item->data.param_enum.value_ptr_temp < 0)//下限
                {
                    menu_data->selected_item->data.param_enum.value_ptr_temp = 0;
                }
                return;
            // }
        }
        else if(menu_data->selected_item->type == MENU_TYPE_TOGGLE)//开关菜单
        {
            menu_data->selected_item->data.toggle.temp ^= 1;//取反
            return;
        }


    }


    // 移动到下一个兄弟节点
    if (menu_data->selected_item->next_sibling) 
    {
        menu_data->selected_item = menu_data->selected_item->next_sibling;
        
        // 如果选中的项不在可见范围内，调整first_visible
        menu_item_t* last_visible = menu_data->first_visible;
        for (int i = 1; i < menu_data->visible_count && last_visible; i++) 
        {
            last_visible = last_visible->next_sibling;
        }
        
        if (menu_data->selected_item == last_visible->next_sibling) 
        {
            menu_data->first_visible = menu_data->first_visible->next_sibling;
        }
    }
}

/**
 * @brief enter操作
 * 
 * @param menu_data 菜单数据句柄
 */
void navigate_enter(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item && menu_data->current_menu->type == MENU_TYPE_MAIN) return;
    // 如果是子菜单，进入子菜单
    menu_data->animation.isAmation = true;   
    if (menu_data->selected_item->type == MENU_TYPE_SUB_MENU && menu_data->selected_item->first_child) //进入子节点
    {  
        menu_data->current_menu = menu_data->selected_item;
        menu_data->selected_item = menu_data->current_menu->first_child;
        menu_data->first_visible = menu_data->selected_item;
        if (menu_data->current_menu->on_enter)
        {
            menu_data->current_menu->on_enter(menu_data->current_menu);
        } 
    }
    // 如果是功能项，执行功能
    else if (menu_data->selected_item->type == MENU_TYPE_FUNCTION && menu_data->selected_item->data.action_cb) //回调函数节点
    {
        menu_data->selected_item->data.action_cb();
    }
    else if (menu_data->selected_item->type == MENU_TYPE_PARAM_INT) //整形枚举菜单
    {
        // 切换参数编辑状态
        menu_data->isSelectedParam ^= 1;
        if(menu_data->isSelectedParam)//进入参数编辑状态
        {
            menu_data->selected_item->data.param_int.temp = *(menu_data->selected_item->data.param_int.value_ptr);//将当前值赋值给临时变量
        }
        else//退出参数编辑状态，保存数据
        {
            *(menu_data->selected_item->data.param_int.value_ptr) = menu_data->selected_item->data.param_int.temp;
        }
    }
    else if (menu_data->selected_item->type == MENU_TYPE_PARAM_ENUM)//字符串枚举菜单
    {
        menu_data->isSelectedParam ^= 1;
        if(menu_data->isSelectedParam)//进入参数编辑状态
        {
            menu_data->selected_item->data.param_enum.value_ptr_temp = *(menu_data->selected_item->data.param_enum.value_ptr);//将当前值赋值给临时变量
        }
        else//退出参数编辑状态，保存数据
        {
            *(menu_data->selected_item->data.param_enum.value_ptr) = menu_data->selected_item->data.param_enum.value_ptr_temp;
        }
    }
    else if (menu_data->selected_item->type == MENU_TYPE_TOGGLE)//开关菜单节点
    {
        // if (menu_data->isSelectedParam)
        // {
        //     *(menu_data->selected_item->data.toggle.value_ptr) ^= 1;
        // }
        // else
        // {
            menu_data->isSelectedParam ^= 1;
            if(menu_data->isSelectedParam)//进入参数编辑状态
            {
                menu_data->selected_item->data.toggle.temp = *(menu_data->selected_item->data.toggle.value_ptr);//将当前值赋值给临时变量
            }
            else//退出参数编辑状态，保存数据
            {
                *(menu_data->selected_item->data.toggle.value_ptr) = menu_data->selected_item->data.toggle.temp;
            } 
    }
    else if (menu_data->current_menu->type == MENU_TYPE_MAIN && menu_data->current_menu->first_child)
    {
        menu_data->current_menu = menu_data->current_menu->first_child;
        menu_data->selected_item = menu_data->current_menu->first_child;
        menu_data->first_visible = menu_data->selected_item;
    }

}

/**
 * @brief 返回函数
 * 
 * @param menu_data 菜单数据节点
 */
void navigate_back(menu_data_t* menu_data) 
{
    if (!menu_data->current_menu) return;
    menu_data->animation.isAmation = true;   
    // 返回到父菜单
    if(menu_data->isSelectedParam)
    {
        // 如果当前在参数编辑状态，退出编辑状态
        menu_data->isSelectedParam = false;
        return;
    }
    if(menu_data->current_menu->parent)
    {
        if (menu_data->current_menu->on_leave)
        {
            menu_data->current_menu->on_leave(menu_data->current_menu);
        }
        menu_data->current_menu = menu_data->current_menu->parent;
        menu_data->selected_item = menu_data->current_menu->first_child;
        menu_data->first_visible = menu_data->selected_item;
        
    }
}

void update_animation(menu_data_t* menu_data)
{
    menu_data->animation.update_flag = true;
}
