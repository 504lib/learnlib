#pragma once

#include <main.h>

class LED
{
private:
    uint16_t GPIO_PIN;
    GPIO_TypeDef* GPIO_PORT; 
    bool isCommondAnode;
public:
    LED(GPIO_TypeDef* GPIO_PORT,uint16_t GPIO_PIN,bool isCommondAnode);
    ~LED();
    void LED_On();
    void LED_Off();
    void LED_Toggle();
    void LED_Blink(uint32_t delay_ms);
};
