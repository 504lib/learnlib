#include "Log.h"

int fputc(int ch, FILE* s)
{
    HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,10);
    return ch;
}

void DMA_UART_printf(const char* fmt, ...)
{
    static char buffer[128]; // 定义一个足够大的缓冲区
    memset(buffer, 0, sizeof(buffer));
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    int len = strlen(buffer);
    if (len > 0)
    {
        if (len > sizeof(buffer)) len = sizeof(buffer); // 防止溢出
        HAL_UART_Transmit_DMA(&huart2, (uint8_t*)buffer, len);
    }
}






