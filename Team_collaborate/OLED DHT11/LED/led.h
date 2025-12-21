#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"
void FlashLED(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
  HAL_Delay(100);
  HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
}


#endif
