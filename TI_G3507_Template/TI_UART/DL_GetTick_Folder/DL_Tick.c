#include "./DL_Tick.h"

volatile uint32_t tick = 0;

uint32_t DL_GetTick()
{
    return tick;
}


//定时器的中断服务函数 已配置为1秒的周期
void TIMER_0_INST_IRQHandler(void)
{
    
    //如果产生了定时器中断
    switch( DL_TimerG_getPendingInterrupt(TIMER_0_INST) )
    {
        case DL_TIMER_IIDX_ZERO://如果是0溢出中断
            tick++;
            //将LED灯的状态翻转
            break;

        default://其他的定时器中断
            break;
    }
}