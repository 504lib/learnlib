#ifndef __TASKS_H
#define __TASKS_H

#include "../Protothreads/Protothreads.h"

// 全局 Protothread 变量
extern Protothread_t oled_pt;

// 任务函数
void OLED_Task(Protothread_t* pt);

// 集中初始化所有 Protothread
void Tasks_Init(void);

#endif
