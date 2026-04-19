#include "./DL_Tick.h"

volatile uint32_t tick = 0;

uint32_t DL_GetTick()
{
    return tick;
}


void SysTick_Handler(void) 
{ 
    DL_SYSTICK_disableInterrupt();
    tick++; 
    DL_SYSTICK_enableInterrupt();
}

