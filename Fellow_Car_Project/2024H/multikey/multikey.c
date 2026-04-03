#include "multikey.h"

static uint16_t Key_Debounce_Time = KEY_DEBOUNCE_TIME;
static uint16_t Key_LongPress_Time = KEY_LONGPRESS_TIME;
static uint16_t Key_LongPress_Repeat_Time = KEY_LONGPRESS_REPEAT_TIME;

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

void MulitKey_Scan(MulitKey_t* key)
{
    uint8_t pin_state = key->readPin(key);
    
    switch(key->state)
    {
        case KEY_IDLE:
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))
            {
                key->state = KEY_DEBOUNCE;
                key->press_last_time = GetTick();
            }
            break;
        case KEY_DEBOUNCE:
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))
            {

                if(GetTick() - key->press_last_time > Key_Debounce_Time) 
                {
                    key->state = KEY_PRESSED;
                    if(key->onPressed) key->onPressed(key);
                    key->press_last_time = GetTick();
                }
            }
            else
            {
                key->state = KEY_IDLE; // 按键抖动，返回空闲状态
            }
            break;
        case KEY_PRESSED:
            if((key->Border_trigger == RISE_BORDER_TRIGGER && pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && !pin_state))
            {
                if(GetTick() - key->press_last_time >= Key_LongPress_Time) //
                {
                    key->state = KEY_LONGPRESS;
                    if(key->onLongPressed) key->onLongPressed(key);
                    key->press_last_time = GetTick();
                }
            }
            else
            {
                key->state = KEY_IDLE; // 按键释放，返回空闲状态
            }
            break;
        case KEY_LONGPRESS:
            if((key->Border_trigger == RISE_BORDER_TRIGGER && !pin_state) || (key->Border_trigger == FALL_BORDER_TRIGGER && pin_state))
            {
                key->state = KEY_IDLE; // 按键释放，返回空闲状态
            }
            else
            {
                if(GetTick() - key->press_last_time >= Key_LongPress_Repeat_Time) //
                {
                    if(key->onLongPressed) key->onLongPressed(key);
                    key->press_last_time = GetTick();
                }
            } 
            break;
        default:
            key->state = KEY_IDLE;
            break;
    }
}

void MulitKey_SetDebounceTime(uint16_t time)
{
    Key_Debounce_Time = time;
}

void MulitKey_SetLongPressTime(uint16_t time)
{
    Key_LongPress_Time = time;
}

void MulitKey_SetLongPressRepeatTime(uint16_t time)
{
    Key_LongPress_Repeat_Time = time;
}
