#include "Motor.h"

typedef void (*Motor_handle)(Motor_Init_Struct* motor_init_struct,Direction dir,uint16_t pulse);


static void TT_Motor_func(Motor_Init_Struct* motor_init_struct,Direction dir,uint16_t pulse);

Motor_handle Motor_Handles[] = {
    TT_Motor_func
};

static void TT_Motor_func(Motor_Init_Struct* motor_init_struct,Direction dir,uint16_t pulse)
{
    if (dir == Motor_Forward)
    {
        motor_init_struct->IN1_GPIO_Write(true);
        motor_init_struct->Set_PWM(91);
    }
    else if(dir == Motor_Reverse)
    {
        motor_init_struct->IN1_GPIO_Write(false);
        motor_init_struct->Set_PWM( pulse / motor_init_struct->period);
    }
    else if(dir == Motor_Stop)
    {
        motor_init_struct->IN1_GPIO_Write(false);
        motor_init_struct->Set_PWM(0);
    }
        
}


void Motor_Init(Motor_Type motor_type,Motor_Init_Struct* motor_init_struct,
                void (*IN1_GPIO_Write)(bool state),
                void (*IN2_GPIO_Write)(bool state),
                void (*PWM_Start_Stop)(bool state),
                void (*Set_PWM)(uint32_t compare),
                uint16_t period)
{
    motor_init_struct->motor_type = motor_type;
    motor_init_struct->IN1_GPIO_Write = IN1_GPIO_Write;
    if (motor_type != TT_Motor)
    {
        motor_init_struct->IN2_GPIO_Write = IN2_GPIO_Write;
    }
    motor_init_struct->PWM_Start_OR_Stop = PWM_Start_Stop;
    motor_init_struct->Set_PWM = Set_PWM;
    motor_init_struct->period = period;
    motor_init_struct->IN1_GPIO_Write(false);
    if (motor_init_struct->motor_type == TT_Motor)
    {
        motor_init_struct->IN2_GPIO_Write = NULL;
        motor_init_struct->Set_PWM(0);
    }
    
}

void Motor_Excute(Motor_Init_Struct* motor_init_struct,Direction dir,uint16_t pulse)
{
    motor_init_struct->PWM_Start_OR_Stop(true);
    Motor_Handles[(uint8_t)dir](motor_init_struct,dir,pulse); 
}
