// dht11.c
#include "dht11.h"
#include "gpio.h"  // ??GPIO HAL????
#include "main.h"  // 包含delay_us函数

// GPIO模式切换
static void DHT11_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 读取一个字节
static uint8_t Read_Byte(void)
{
    uint8_t i, temp = 0;
    
    for (i = 0; i < 8; i++)
    {
        // 等待低电平结束
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET);
        
        // 延时40us
        delay_us(40);
        
        // 判断数据位
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
        {
            // 等待高电平结束
            while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET);
            
            temp |= (uint8_t)(0x01 << (7 - i));
        }
        else
        {
            temp &= (uint8_t)~(0x01 << (7 - i));
        }
    }
    
    return temp;
}

// 初始化
void DHT11_Init(void)
{
    DHT11_Mode_Out_PP();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

// 读取DHT11数据（阻塞式）
uint8_t DHT11_Read(DHT11_Data_TypeDef *DHT11_Data)
{
    // 发送开始信号
    DHT11_Mode_Out_PP();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    delay_ms(18);
    
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);
    
    // 切换到输入模式
    DHT11_Mode_IPU();
    
    // 等待DHT11响应
    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
    {
        // 等待低电平结束
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET);
        
        // 等待高电平结束
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET);
        
        // 读取5个字节数据
        DHT11_Data->humi_int = Read_Byte();
        DHT11_Data->humi_deci = Read_Byte();
        DHT11_Data->temp_int = Read_Byte();
        DHT11_Data->temp_deci = Read_Byte();
        DHT11_Data->check_sum = Read_Byte();
        
        // 切换回输出模式
        DHT11_Mode_Out_PP();
        HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
        
        // 校验数据
        if (DHT11_Data->check_sum == DHT11_Data->humi_int + DHT11_Data->humi_deci + 
                                     DHT11_Data->temp_int + DHT11_Data->temp_deci)
        {
            return SUCCESS;
        }
        else
        {
            return ERROR;
        }
    }
    
    return ERROR;
}
