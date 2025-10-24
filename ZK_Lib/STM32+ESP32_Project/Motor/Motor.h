#pragma once

#include "main.h"
#include "Log.h"

typedef enum
{
    TT_Motor = 0,
}Motor_Type;

typedef enum
{
    Motor_Forward = 0,
    Motor_Reverse,
    Motor_Stop
}Direction;

typedef struct
{
    Motor_Type motor_type;
    void (*IN1_GPIO_Write)(bool state);
    void (*IN2_GPIO_Write)(bool state);
    void (*PWM_Start_OR_Stop)(bool is_start);
    void (*Set_PWM)(uint32_t compare);
    uint16_t pulse;
    uint16_t period;
}Motor_Init_Struct;



void Motor_Init(Motor_Type motor_type,Motor_Init_Struct* motor_init_struct,
                void (*IN1_GPIO_Write)(bool state),
                void (*IN2_GPIO_Write)(bool state),
                void (*PWM_Start_Stop)(bool state),
                void (*Set_PWM)(uint32_t compare),
                uint16_t period);
void Motor_Excute(Motor_Init_Struct* motor_init_struct,Direction dir,uint16_t pulse);
