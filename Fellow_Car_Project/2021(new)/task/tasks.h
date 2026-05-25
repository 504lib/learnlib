#ifndef __TASKS_H
#define __TASKS_H

#include "Protothreads.h"

// 声明全局 Protothread 变量（在 tasks.c 中定义）
extern Protothread_t task1_pt;
extern Protothread_t task2_pt;
extern Protothread_t SerialTask_pt;
extern Protothread_t oled_pt;

// 声明任务函数
void task1(Protothread_t* pt);
void task2(Protothread_t* pt);
void SerialTask(Protothread_t* pt);
void OLED_Task(Protothread_t* pt);

// 初始化所有任务（集中调用 PT_INIT）
void Tasks_Init(void);

#endif
