#ifndef __APP_FSM_H__
#define __APP_FSM_H__

#include "HSM_Core.h"
#include "oled.h"

/* 事件 ID */
#define EV_START    0x01   // 按键启动
#define EV_STOP     0x02   // 停车
#define EV_CURVE    0x03   // 进入弯道
#define EV_STRAIGHT 0x04   // 进入直道

typedef enum
{
    MOTOR_ACTION_STOP = 0,
    MOTOR_ACTION_STRAIGHT = 1,
    MOTOR_ACTION_CURVE = 2,
} MotorAction_t;

void App_FSM_Init(void);
void App_FSM_Process(void);
void App_FSM_SendEvent(uint8_t event_id);


#endif
