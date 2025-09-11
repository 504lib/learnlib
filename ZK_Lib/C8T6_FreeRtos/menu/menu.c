#include "menu.h"

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
            int* value_ptr;      // 参数值指针
            int min;             // 最小值
            int max;             // 最大值
            int step;            // 步进值
        } param_int;
        
        struct {
            int* value_ptr;      // 当前值指针
            const char** options;// 选项字符串数组
            int option_count;    // 选项数量
        } param_enum;
        
        struct {
            bool* value_ptr;     // 开关状态指针
        } toggle;
    } data;
    
    // 可选的回调函数
    void (*on_enter)(menu_item_t* item);    // 进入该项时调用
    void (*on_leave)(menu_item_t* item);    // 离开该项时调用
    void (*on_change)(menu_item_t* item);   // 值改变时调用
};

struct menu_data_t{
    menu_item_t* current_menu;    // 当前显示的菜单
    menu_item_t* selected_item;   // 当前选中的菜单项
    menu_item_t* first_visible;   // 当前显示的第一个菜单项
    uint8_t visible_count;        // 可见菜单项数量
};


menu_item_t Menu_Node[MENU_NODE] = {0};

menu_data_t menu_data = {0};

menu_data_t* menu_data_init(menu_item_t* root)
{
    menu_data.current_menu = root;
    menu_data.selected_item = root->first_child;
    menu_data.first_visible = root->first_child;
    menu_data.visible_count = 0;
    return &menu_data;
}

menu_item_t* create_submenu_item(const char* text)
{
    if (menu_node_count >= MENU_NODE) {
        return NULL; // 超过最大节点数
    }
    
    menu_item_t* item = &Menu_Node[menu_node_count++];
//    memset(item, 0, sizeof(menu_item_t));
    item->text = text;
    item->type = MENU_TYPE_SUB_MENU;
    return item;
}

menu_item_t* create_function_item(const char* text, void (*action_cb)(void))
{
    if (menu_node_count >= MENU_NODE) {
        return NULL; // 超过最大节点数
    }
    
    menu_item_t* item = &Menu_Node[menu_node_count++];
    item->text = text;
    item->type = MENU_TYPE_FUNCTION;
    item->data.action_cb = action_cb;
    return item;
}

void Link_Parent_Child(menu_item_t* parent, menu_item_t* child)
{
    if (parent == NULL || child == NULL) 
    {
        return;
    }
    child->parent = parent;
    parent->sub_menu_count++;
    if (parent->first_child == NULL) 
    {
        parent->first_child = child;
        parent->last_child = child;
    } 
    else 
    {
        parent->last_child->next_sibling = child;
        child->prev_sibling = parent->last_child;
        parent->last_child = child;
    }
}

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

void show_menu(u8g2_t* u8g2, menu_data_t* menu_data, uint8_t max_display_count) {
    osKernelLock();
    
    if (!menu_data->current_menu || !menu_data->current_menu->first_child) 
    {
        osKernelUnlock();
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
    
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
    
    // 显示当前菜单标题
    u8g2_DrawStr(u8g2, 32, 5, menu_data->current_menu->text);
    
    // 显示菜单项
    menu_item_t* item = menu_data->first_visible;
    uint8_t count = 0;
    int y = 20;
    
    while (item && count < max_display_count) 
    {
        if (item == menu_data->selected_item) 
        {
            // 高亮选中的项
            u8g2_DrawBox(u8g2, 0, y - 10, 128, 15);
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawStr(u8g2, 0, y, item->text);
            u8g2_SetDrawColor(u8g2, 1);
        } 
        else 
        {
            u8g2_DrawStr(u8g2, 0, y, item->text);
        }
        
        y += 15;
        item = item->next_sibling;
        count++;
    }
    
    menu_data->visible_count = count;
    u8g2_SendBuffer(u8g2);
    osKernelUnlock();
}

void navigate_up(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item) return;
    
    // 移动到上一个兄弟节点
    if (menu_data->selected_item->prev_sibling) 
    {
        menu_data->selected_item = menu_data->selected_item->prev_sibling;
        
        // 如果选中的项不在可见范围内，调整first_visible
        if (menu_data->selected_item == menu_data->first_visible->prev_sibling) 
        {
            menu_data->first_visible = menu_data->selected_item;
        }
    }
}

void navigate_down(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item) return;
    
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

void navigate_enter(menu_data_t* menu_data) 
{
    if (!menu_data->selected_item) return;
    
    // 如果是子菜单，进入子菜单
    if (menu_data->selected_item->type == MENU_TYPE_SUB_MENU && menu_data->selected_item->first_child) 
    {
        menu_data->current_menu = menu_data->selected_item;
        menu_data->selected_item = menu_data->current_menu->first_child;
        menu_data->first_visible = menu_data->selected_item;
    }
    // 如果是功能项，执行功能
    else if (menu_data->selected_item->type == MENU_TYPE_FUNCTION && menu_data->selected_item->data.action_cb) 
    {
        menu_data->selected_item->data.action_cb();
    }
}

void navigate_back(menu_data_t* menu_data) 
{
    if (!menu_data->current_menu || !menu_data->current_menu->parent) return;
    
    // 返回到父菜单
    menu_data->current_menu = menu_data->current_menu->parent;
    menu_data->selected_item = menu_data->current_menu->first_child;
    menu_data->first_visible = menu_data->selected_item;
}
