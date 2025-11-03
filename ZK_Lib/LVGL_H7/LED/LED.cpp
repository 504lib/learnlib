#include "LED.h"

LED::LED(GPIO_TypeDef* GPIO_PORT,uint16_t GPIO_PIN,bool isCommondAnode)
    : GPIO_PIN(GPIO_PIN), GPIO_PORT(GPIO_PORT), isCommondAnode(isCommondAnode)
{
    // Constructor implementation (if needed)
}

LED::~LED()
{
    // Destructor implementation (if needed)
}

void LED::LED_On()
{
    HAL_GPIO_WritePin(GPIO_PORT, GPIO_PIN, (isCommondAnode? GPIO_PIN_RESET : GPIO_PIN_SET));
}

void LED::LED_Off()
{
    HAL_GPIO_WritePin(GPIO_PORT, GPIO_PIN, (isCommondAnode? GPIO_PIN_SET : GPIO_PIN_RESET));
}

void LED::LED_Toggle()
{
    HAL_GPIO_TogglePin(GPIO_PORT, GPIO_PIN);
}

void LED::LED_Blink(uint32_t delay_ms)
{
    LED_On();
    HAL_Delay(delay_ms);
    LED_Off();
    HAL_Delay(delay_ms);
}
