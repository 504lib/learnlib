#ifndef MOTOR_AT4950_H
#define MOTOR_AT4950_H

#include <stdint.h>


typedef enum 
{
    Low_Level = 0,
    High_Level = 1
}DefaultDirection_T;

typedef void(*SetComparePWM)(uint16_t ccr_value);
typedef void(*SetMotorIN1Level)(uint8_t level); ;


typedef struct MotorAT4950
{
    DefaultDirection_T Default_Direction;
    uint16_t Auto_Reload;
    int16_t Ccr;
    SetComparePWM Set_pwm;
    SetMotorIN1Level Set_IN1_Level;
}MotorAT4950;


void MotorInit_AT46950(MotorAT4950* motor4950 ,SetComparePWM setcomparePWM,SetMotorIN1Level setmotorIN1Level,uint16_t Auto_Reload);
void SetDefaultDirection(MotorAT4950* motor4950,DefaultDirection_T level);
void Motor_setSpeed(MotorAT4950* motor4950,int16_t duty);

#endif /* MOTOR_AT4950_H */
