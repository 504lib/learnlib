#include "Log.h"

int fputc(int ch, FILE* s)
{
    HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,HAL_MAX_DELAY);
    return ch;
}
