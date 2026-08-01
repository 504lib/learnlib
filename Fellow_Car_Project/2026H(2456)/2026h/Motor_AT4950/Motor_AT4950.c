#include "Motor_AT4950.h"
#include <stdint.h>



void MotorInit_AT46950(MotorAT4950* motor4950 ,SetComparePWM setcomparePWM,SetMotorIN1Level setmotorIN1Level,uint16_t Auto_Reload)
{
    if (!motor4950) { return; }
    motor4950->Auto_Reload   = Auto_Reload;
    motor4950->Set_pwm       = setcomparePWM;
    motor4950->Set_IN1_Level = setmotorIN1Level;
    motor4950->Ccr           = 0;
    motor4950->Default_Direction = High_Level;
}


void SetDefaultDirection(MotorAT4950* motor4950,DefaultDirection_T level)
{
    if (!motor4950) { return; }
    motor4950->Default_Direction = level;
}

void Motor_setSpeed(MotorAT4950* motor4950, int16_t duty)
{
    if (!motor4950 || !motor4950->Set_pwm || !motor4950->Set_IN1_Level) {
        return;
    }

    // 1) 饱和并取绝对值作为占空
    if (duty > 1000) duty = 1000;
    else if (duty < -1000) duty = -1000;
    uint16_t absDuty = (duty >= 0) ? (uint16_t)duty : (uint16_t)(-duty);

    // 2) 方向仅由符号决定；再按“方向极性”映射到 IN1 电平
    uint8_t forward = (duty >= 0) ? 1 : 0;
    uint8_t in1_level = forward ? (motor4950->Default_Direction ? 1 : 0)
                                : (motor4950->Default_Direction ? 0 : 1);
    motor4950->Set_IN1_Level(in1_level);

    // 3) 计算 CCR，并按“PWM 有效电平”决定是否互补
    motor4950->Ccr = (uint16_t)((motor4950->Auto_Reload * absDuty) / 1000);
    uint16_t ccr_out = (in1_level == 0)
                     ? motor4950->Ccr
                     : (motor4950->Auto_Reload - motor4950->Ccr);

    motor4950->Set_pwm(ccr_out);
}