#include "app_log.h"
#include "usart.h"

static void __log_uart_tx(const char* buffer, size_t buffer_size)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, buffer_size, HAL_MAX_DELAY);
}

void App_Log_Init(void)
{
    LOG_Init(__log_uart_tx);
#ifdef DEBUG
    LOG_Set_Level(LOG_LEVEL_DEBUG);
#else
    LOG_Set_Level(LOG_LEVEL_INFO);
#endif
    LOG_INFO("Log system initialized");
}
