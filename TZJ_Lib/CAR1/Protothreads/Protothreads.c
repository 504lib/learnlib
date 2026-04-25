//#include <windows.h>
#define GET_TICKS() HAL_GetTick()
#include "Protothreads.h"
#include <stdio.h>

void task3(Protothread_t* pt){
    PT_BEGIN(pt);
    while(1){
        printf("Task 3 is running,ticks: %lu\n", GET_TICKS());
        PT_WAIT_TICK(pt, 500); // 每隔2000毫秒执行一次
    }
    PT_END(pt);
}

void task1(Protothread_t* pt){
    PT_BEGIN(pt);
    pt->ticks = GET_TICKS(); // 记录当前时间戳
    while(1){
        printf("Task 1 is running,ticks: %lu\n", GET_TICKS());
        // PT_WAIT_UNTIL(pt,GET_TICKS() - pt->ticks >= 1000); // 每隔1000毫秒执行一次
        // pt->ticks = GET_TICKS(); // 记录当前时间戳
        PT_WAIT_TICK(pt, 1000); // 每隔1000毫秒执行一次
    }
    PT_END(pt);
}

void task2(Protothread_t* pt){
    PT_BEGIN(pt);
    pt->ticks = GET_TICKS(); // 记录当前时间戳
    while(1){
        printf("Task 2 is running,ticks: %lu\n", GET_TICKS());
        // PT_WAIT_UNTIL(pt, GET_TICKS() - pt->ticks >= 5000); // 每隔1500毫秒执行一次
        // pt->ticks = GET_TICKS(); // 记录当前时间戳
        PT_WAIT_TICK(pt, 10000);
    }
    PT_END(pt);
}

//int main(){
//    Protothread_t pt1, pt2,pt3;
//    PT_INIT(&pt1);
//    PT_INIT(&pt2);
//    PT_INIT(&pt3);
//    while(1){
//        task1(&pt1);
//        task2(&pt2);
//        task3(&pt3);
//    }
//    return 0;
//}