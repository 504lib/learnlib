#ifndef __PROTOTHREADS_H_
#define __PROTOTHREADS_H_

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

typedef struct{
    uint32_t line;
    uint32_t ticks;
} Protothread_t;


// 用户需在包含本头文件前定义 GET_TICKS 宏，例如：
 #define GET_TICKS() HAL_GetTick()

static inline void PT_INIT(Protothread_t* pt){
    pt->line = 0;
    pt->ticks = 0;
}

#define PT_BEGIN(pt) switch((pt)->line){ case 0:

#define PT_WAIT_UNTIL(pt, condition) \
    do{ \
        (pt)->line = __LINE__; \
        case __LINE__: \
        if(!(condition)) return; \
    }while(0)

#if defined(GET_TICKS)

#define PT_WAIT_TICK(pt,time) do{ PT_WAIT_UNTIL(pt, GET_TICKS() - (pt)->ticks >= (time));  (pt)->ticks = GET_TICKS();}while(0)

#endif

#define PT_END(pt) }

#endif
