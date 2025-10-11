#include "multikey.h"


// 自定义检测时间
static uint16_t Key_Debounce_Time = KEY_DEBOUNCE_TIME;                      // 抖动检测时间
static uint16_t Key_LongPress_Time = KEY_LONGPRESS_TIME;                    // 长按检测时间
static uint16_t Key_LongPress_Repeat_Time = KEY_LONGPRESS_REPEAT_TIME;      // 长按任务周期时间


/**
 * @brief    按键初始化
 * @details  结构体并未是匿名的结构体，你可以自己手动堆结构体进行初始化
 * @param    key       按键对象的结构体指针
 * @param    readPin   读取对应按键引脚的函数,未空则不进行初始化
 * @param    onPressed 单按的回调函数
 * @param    onLongPressed 长按的回调函数
 * @param    trigger   上升沿触发/下降沿触发
 */
void MulitKey_Init(MulitKey_t* key,KeyReadPinCallback readPin,KeyPressdCallback onPressed,KeyLongPressdCallback onLongPressed,BorderTrigger trigger)
{
    if(readPin == NULL) return;
    key->Border_trigger = trigger;
    key->readPin = readPin;
    key->onPressed = onPressed;
    key->onLongPressed = onLongPressed;

    key->state = KEY_IDLE;
    key->press_last_time = 0;
}

/**
 * @brief    按键扫描测试
 * @attention 注意一定确保沿边检测与实际电路的实现相符
 * @param    key       按键对象
 */
void MulitKey_Scan(MulitKey_t* key)
{
    uint8_t pin_state = key->readPin(key);          //通过用户自定义的读取引脚函数读取引脚当前状态
    
    switch(key->state)
    {
        case KEY_IDLE:                              // 按键无动作
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))     // 按键按下
            {
                key->state = KEY_DEBOUNCE;          // 进入抖动检测
                key->press_last_time = GetTick();   // 获取当前的tick值作为下次按钮时间检测基准
            }
            break;
        case KEY_DEBOUNCE:                          // 按键抖动状态
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))     // 按键按下
            {
                if(GetTick() - key->press_last_time > Key_Debounce_Time)    // 按键抖动检测
                {
                    key->state = KEY_PRESSED;       // 按键按下状态
                    if(key->onPressed) key->onPressed(key);     // 立刻触发按键单按动作
                    key->press_last_time = GetTick();           // 获取当前的tick值作为下次按钮时间检测基准
                }
            }
            else
            {
                key->state = KEY_IDLE;              // 按键抖动，返回空闲状态
            }
            break;
        case KEY_PRESSED:                           // 按键按下状态
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))// 按键按下
            {
                if(GetTick() - key->press_last_time >= Key_LongPress_Time) // 检测是否到达长按时间
                {
                    key->state = KEY_LONGPRESS;                            // 状态切成长按
                    if(key->onLongPressed) key->onLongPressed(key);        // 立刻执行长按任务
                    key->press_last_time = GetTick();                      // 获取当前的tick值作为下次按钮时间检测基准 
                }
            }
            else
            {
                key->state = KEY_IDLE; // 按键释放，返回空闲状态
            }
            break;
        case KEY_LONGPRESS:                         // 按键长按状态
            if((key->Border_trigger == RISE_BORDER_TRIGGER && !pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && pin_state)) // 按键按下
            {
                key->state = KEY_IDLE; // 按键释放，返回空闲状态
            }
            else
            {
                if(GetTick() - key->press_last_time >= Key_LongPress_Repeat_Time) //周期执行
                {
                    if(key->onLongPressed) key->onLongPressed(key);         // 执行长按任务
                    key->press_last_time = GetTick();
                }
            } 
            break;
        default:
            key->state = KEY_IDLE;
            break;
    }
}

/**
 * @brief    设置抖动检测时间
 * @attention 支持动态切换时间，通过此函数
 * @param    time      时间，单位为tick值(一般是ms)
 */
void MulitKey_SetDebounceTime(uint16_t time)
{
    Key_Debounce_Time = time;
}


/**
 * @brief    设置长按检测时间
 * @attention 支持动态切换时间，通过此函数
 * @param    time      时间，单位为tick值(一般是ms)
 */
void MulitKey_SetLongPressTime(uint16_t time)
{
    Key_LongPress_Time = time;
}


/**
 * @brief    设置长按周期检测时间
 * @attention 支持动态切换时间，通过此函数
 * @param    time      时间，单位为tick值(一般是ms)
 */
void MulitKey_SetLongPressRepeatTime(uint16_t time)
{
    Key_LongPress_Repeat_Time = time;
}
