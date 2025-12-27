#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"

// 引脚定义
#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_PIN_0

// 返回状态
#define SUCCESS 1
#define ERROR   0

// DHT11数据结构
typedef struct
{
    uint8_t humi_int;      // 湿度整数部分
    uint8_t humi_deci;     // 湿度小数部分
    uint8_t temp_int;      // 温度整数部分
    uint8_t temp_deci;     // 温度小数部分
    uint8_t check_sum;     // 校验和
} DHT11_Data_TypeDef;

// 函数声明
void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data_TypeDef *DHT11_Data);

#endif
