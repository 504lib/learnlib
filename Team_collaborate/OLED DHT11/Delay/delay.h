#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

// 微秒延时函数声明
void delay_us(uint32_t us);

// 毫秒延时函数声明
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
// 添加一个空行在这里
