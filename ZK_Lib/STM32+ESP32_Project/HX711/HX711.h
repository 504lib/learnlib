#ifndef HX711_H
#define HX711_H

#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "Log.h"

#define _disable_interrupt_func         osKernelLock()
#define _enable_interrupt_func          osKernelUnlock();
#define Get_Tick                        osKernelGetTickCount
// 函数声明
void HX711_Tare(void);
int32_t HX711_GetWeight(void);
float HX711_GetFilteredWeight(void);

#endif
