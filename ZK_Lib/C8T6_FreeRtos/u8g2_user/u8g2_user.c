#include "u8g2_user.h"

void draw(u8g2_t *u8g2)
{
    u8g2_SetFontMode(u8g2, 1); /*字体模式选择*/
    u8g2_SetFontDirection(u8g2, 0); /*字体方向选择*/
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf); /*字库选择*/
    u8g2_DrawStr(u8g2, 0, 20, "U");
    
    u8g2_SetFontDirection(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_inb30_mn);
    u8g2_DrawStr(u8g2, 21,8,"8");
        
    u8g2_SetFontDirection(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(u8g2, 51,30,"g");
    u8g2_DrawStr(u8g2, 67,30,"\xb2");
    
    u8g2_DrawHLine(u8g2, 2, 35, 47);
    u8g2_DrawHLine(u8g2, 3, 36, 47);
    u8g2_DrawVLine(u8g2, 45, 32, 12);
    u8g2_DrawVLine(u8g2, 46, 33, 12);
  
    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1,54,"github.com/olikraus/u8g2");
}

uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;
        case U8X8_MSG_GPIO_CS: //SPI - CS
            HAL_GPIO_WritePin(CS_OLED_GPIO_Port, CS_OLED_Pin, (GPIO_PinState)arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            HAL_GPIO_WritePin(DC_OLED_GPIO_Port, DC_OLED_Pin, (GPIO_PinState)arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            HAL_GPIO_WritePin(RES_OLED_GPIO_Port, RES_OLED_Pin, (GPIO_PinState)arg_int);
            break;
    }
    return 1;
}

uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND:
            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)arg_ptr, arg_int);
            while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
            break;
        case U8X8_MSG_BYTE_INIT:
            // u8x8_gpio_and_delay(u8x8, U8X8_MSG_GPIO_AND_DELAY_INIT, arg_int, arg_ptr);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            HAL_GPIO_WritePin(DC_OLED_GPIO_Port,DC_OLED_Pin,(GPIO_PinState)arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8,u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, arg_int, NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, arg_int, NULL);
            u8x8_gpio_SetCS(u8x8,u8x8->display_info->chip_disable_level);
            break;
    }
    return 1;
}

void u8g2Init(u8g2_t* u8g2)
{
  u8g2_Setup_ssd1306_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi, u8x8_stm32_gpio_and_delay);
  u8g2_InitDisplay(u8g2);
  u8g2_SetPowerSave(u8g2, 0); // 唤醒显示屏
  u8g2_ClearDisplay(u8g2); // 清除显示
}

void drawProgressBar(u8g2_t *u8g2,uint8_t progress) 
{
    // 绘制进度条框架
    u8g2_DrawFrame(u8g2, 10, 25, 100, 10);
    
    if(progress >= 100)
    {
        progress = 100;
    }
    // 绘制进度填充
    u8g2_DrawBox(u8g2, 10, 25, progress, 10);
    
    // 显示进度百分比
    char buffer[10];
    sprintf(buffer, "%d%%", progress);
    u8g2_DrawStr(u8g2, 50, 20, buffer);
}

void updateProgressBar(uint8_t* progress,uint8_t step,uint8_t goal_progress) 
{
    static uint32_t lastUpdateTime = 0;
    uint32_t currentTime = osKernelGetTickCount();
    
    // 每100毫秒更新一次进度
    if (currentTime - lastUpdateTime >= 1) 
    {
        lastUpdateTime = currentTime;

        if (*progress < goal_progress) 
        {
            (*progress) += step;
        }
        else
        {
          *progress = goal_progress;
        }
    }
}

 void test()
 {
    static uint32_t count = 0;
	 char buffer[50];
	 sprintf(buffer, "test function %d\r\n", count++);
	 HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), 1000);
 }
