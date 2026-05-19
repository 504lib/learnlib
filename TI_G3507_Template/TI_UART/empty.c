/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>
#include "./SPI_OLED/oled.h"
#include "./DL_GetTick_Folder/DL_Tick.h"
#include "./multikey/multikey.h"
#include "./Protothreads/Protothreads.h"
#include "./Motor/Motor_AT4950.h"
#include "./grayscale/grayscale.h"
#include "./PID_Node/PID_Node.h"
#include "./Control_Speed/Control_Speed.h"
#include "./MPU_6050/mpu6050_user.h"
#include "./MPU_6050/mpu6050.h"
#include "./MPU_6050/MadgwickAHRS.h"
// #include "./Protothreads/"



// volatile uint32_t test_tick = 0;

// 编码器计数值（原始累计值）
volatile int32_t encoder1_raw = 0;
volatile int32_t encoder2_raw = 0;

// 速度反馈值（20ms 内的脉冲增量）
volatile int32_t encoder1_speed = 0;
volatile int32_t encoder2_speed = 0;

// 用于计算速度的中间变量
static int32_t last_enc1 = 0;
static int32_t last_enc2 = 0;

//灰度值全局变量
volatile uint8_t gray_byte = 0;
volatile float gray_error = 0.0f;

MotorAT4950 motor1;
MotorAT4950 motor2;

// #define ABS(a)      (a>0 ? a:(-a))

Protothread_t task1_pt = {0};
volatile unsigned char uart_data = 0;
char buffuer[256] = {0};
MulitKey_t key;

// void PID_Node_Initization()
// {
// }

// 底层 PWM/GPIO 回调实现（新增）
// 电机1方向控制：同时控制 AIN1 和 AIN2
void SetMotor1IN1(uint8_t level) {
    if (level)
        DL_GPIO_setPins(A4950_PORT, A4950_AIN1_PIN);
    else
        DL_GPIO_clearPins(A4950_PORT, A4950_AIN1_PIN);
}

// 电机2方向控制：同时控制 BIN1 和 BIN2
void SetMotor2IN1(uint8_t level) {
    if (level)
        DL_GPIO_setPins(A4950_PORT, A4950_BIN1_PIN);
    else
        DL_GPIO_clearPins(A4950_PORT, A4950_BIN1_PIN);
}

void SetMotor1PWM(uint16_t ccr) {
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, ccr, GPIO_PWM_0_C0_IDX);
}
void SetMotor2PWM(uint16_t ccr) {
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, ccr, GPIO_PWM_0_C1_IDX);
}

void uart0_send_char(char ch); //串口0发送单个字符
void uart0_send_string(char* str); //串口0发送字符串


void task1(Protothread_t* pt)
{
    static uint32_t times = 0;
    PT_BEGIN(pt);
    while (1) {
        // 显示 yaw 角
        MPU6050_Data_t* mpu = MPU6050_GetHandle();
        snprintf(buffuer, sizeof(buffuer), "yaw:%.1f", mpu->yaw);
        OLED_ShowString(0, 32, (uint8_t*)buffuer, 16, 1);
        snprintf(buffuer, sizeof(buffuer), "times:%u",times++);
        OLED_ShowString(0, 16, (uint8_t*)buffuer, 16, 1);  
        // // 显示左右轮实际速度
        // float Actual_Speed_A = Control_Speed_GetActualSpeed(CONTROL_SPEED1_OBJECT);
        // float Actual_Speed_B = Control_Speed_GetActualSpeed(CONTROL_SPEED2_OBJECT);
        // snprintf(buffuer, sizeof(buffuer), "L:%.3f R:%.3f", Actual_Speed_A, Actual_Speed_B);
        // OLED_ShowString(0, 16, (uint8_t*)buffuer, 16, 1);
        // snprintf(buffuer, sizeof(buffuer), "times:%u",times++);
        // OLED_ShowString(0, 32, (uint8_t*)buffuer, 16, 1);     

        // 串口输出 SerialPlot 格式
        // PID_Node* node1 = Control_Speed_PID_Hander(CONTROL_SPEED1_OBJECT);
        // printf("%.3f,%.3f,%.3f\n", node1->setpoint, Actual_Speed_A, Actual_Speed_B);
        OLED_Refresh();
        PT_WAIT_TICK(pt,100);
    }
    PT_END(pt);
}


uint8_t Key1ReadPinCallback(MulitKey_t* key)
{
    // 1. 读取原始位掩码值
    uint32_t raw_value = DL_GPIO_readPins(KEY_GROIP_PORT, KEY_GROIP_BOARD_KEY_PIN);
    
    // 2. 使用双重否定 !! 将非零值强制转换为1，零值保持为0
    return (uint8_t)(!!raw_value);
}

void Key1PressdCallback(MulitKey_t* key)
{
    // static bool isMotorOn = false;
    // isMotorOn = !isMotorOn;
    // DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
    // Motor_Set_PWM((isMotorOn) ? 2000 : 0, (isMotorOn) ? 2000 : 0);  // 左电机正转，右电机反转
    
}

int fputc(int ch, FILE *f)
{
    // 关键：使用阻塞式发送函数，直到字符成功发送
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)ch);
    return ch;
}

int fputs(const char *_ptr, FILE *_fp)
{
    uint16_t i, len;
    len = strlen(_ptr);
    for (i = 0; i < len; i++)
    {
        // 复用 fputc 逐个字符发送
        fputc(_ptr[i], _fp);
    }
    return len;
}

int puts(const char *_ptr)
{
    int count;
    // 先发送字符串本身
    count = fputs(_ptr, stdout);
    // 再发送一个换行符
    count += fputs("\n", stdout);
    return count;
}

int main(void)
{
    SYSCFG_DL_init();
    //清除串口中断标志
    // NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    // NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
    //使能串口中断
    
    printf("串口初始化完成\n");
    // 使能 GPIO 中断
    NVIC_ClearPendingIRQ(ENCODER_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_INT_IRQN);
    printf("编码器初始化完成\n");
    // DL_TimerA_startCounter(TIMER_1_INST);// 启动 TIMER_1
    // DL_TimerG_startCounter(TIMER_0_INST);

    OLED_Init();
    OLED_Clear();
    OLED_DisPlay_On();
    // ========== ★ MPU6050 静止校准 ==========
    OLED_ShowString(0, 0, (uint8_t*)"Calibrating", 16, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Keep Still!", 16, 1);
    OLED_Refresh();
    DL_I2C_enableController(I2C_6050_INST);   // ★ 使能 I2C 控制器
    int res = MPU_Init();                    // 初始化 MPU6050（需 I2C 已就绪）
    snprintf(buffuer, sizeof(buffuer), "current res:%d",res);
    OLED_ShowString(0, 32, (uint8_t*)buffuer, 16, 1);
    OLED_Refresh();
    delay_cycles(CPUCLK_FREQ);
    MPU6050_Calibrate(200);      // 采样 200 次，约 1 秒，期间保持静止
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Calib Done!", 16, 1);
    OLED_Refresh();
    OLED_Clear();

    // ★ 校准完成后才启动控制定时器（避免 ISR 里 MPU6050_Update 和校准抢 I2C）
    // NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerG_startCounter(TIMER_0_INST);
    printf("屏幕初始化完成\n");
    // 初始化 A4950 电机（Auto_Reload = 10000 - 1 = 9999，与 PWM timerCount=10000 对应）
    MotorInit_AT46950(&motor1, SetMotor1PWM, SetMotor1IN1, 9999);
    MotorInit_AT46950(&motor2, SetMotor2PWM, SetMotor2IN1, 9999);
    SetDefaultDirection(&motor1, Low_Level);
    SetDefaultDirection(&motor2, High_Level);
    // 改为0，让PID从零开始闭环控制
    // Motor_setSpeed(&motor1, 0);
    // Motor_setSpeed(&motor2, 0);
    // DL_GPIO_setPins(A4950_PORT, A4950_BIN1_PIN);               // 方向
    // DL_TimerG_setCaptureCompareValue(PWM_0_INST, 8000, GPIO_PWM_0_C0_IDX); // PWM
    // 初始化速度环，传入电机句柄
    Control_Init(&motor1, &motor2);
    Control_SetBaseSpeed(0.35f);
    // Control_Speed_SetPID(CONTROL_SPEED1_OBJECT, 700.0f, 2.5f, 0.0f);
    // Control_Speed_SetPID(CONTROL_SPEED2_OBJECT, 700.0f, 2.5f, 0.0f);
    // Control_Speed_SetSetPoint(CONTROL_SPEED1_OBJECT, 0.3f);
    // Control_Speed_SetSetPoint(CONTROL_SPEED2_OBJECT, 0.3f);
    
    MulitKey_Init(&key,Key1ReadPinCallback,Key1PressdCallback,Key1PressdCallback,FALL_BORDER_TRIGGER);
    // volatile uint32_t cnt = 0;
    PT_INIT(&task1_pt);
    uart0_send_string("uart0 start work\r\n");
    printf("测试\n");

    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    // Motor_Set_PWM(-3000, -3000);
    // DL_TimerG_setCaptureCompareValue(PWM_0_INST, 3000, GPIO_PWM_0_C1_IDX);
    while (1)
    {
        // printf("current tick_systick : %u\n",DL_SYSTICK_getValue());
        // delay_cycles(CPUCLK_FREQ / 2);
        MulitKey_Scan(&key);
        task1(&task1_pt);
        static uint32_t last_enc_print = 0;
        static bool isReverse = false;
        if (DL_GetTick() - last_enc_print >= 1000) {
            // printf("E1_raw=%ld, E2_raw=%ld\n", encoder1_raw, encoder2_raw);
            if (isReverse) {
                DL_GPIO_clearPins(LED_PORT, LED_LED_2_PIN);
            }
            else {
                DL_GPIO_setPins(LED_PORT, LED_LED_2_PIN);
            }
            isReverse = !isReverse;
            last_enc_print = DL_GetTick();
        }
    }
}

//串口发送单个字符
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_0_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_0_INST, ch);
}
//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}

//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_0_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
            //将发送过来的数据保存在变量中
            uart_data = DL_UART_Main_receiveData(UART_0_INST);
            //将保存的数据再发送出去
            // uart0_send_char(uart_data);
            // printf("receive data:%c\n",uart_data);
            break;

        default://其他的串口中断
            break;
    }
}

// void TIMER_1_INST_IRQHandler(void)
// {
//     if (DL_TimerG_getPendingInterrupt(TIMER_1_INST) == DL_TIMER_IIDX_ZERO) {
        // 读取当前原始计数值
        // int32_t curr1 = encoder1_raw;
        // int32_t curr2 = encoder2_raw;

        // // 计算增量（20ms 内的脉冲数）
        // encoder1_speed = curr1 - last_enc1;
        // encoder2_speed = curr2 - last_enc2;

        // // 更新上一次计数值
        // last_enc1 = curr1;
        // last_enc2 = curr2;

        // 可选：调用您的控制函数
        // Control_UpdateSpeedFeedback(encoder1_speed, encoder2_speed, 20);
        // 或者直接在这里使用 Motor_setSpeed 进行 PID 控制
//     }
// }

//定时器的中断服务函数 已配置为1秒的周期
void TIMER_0_INST_IRQHandler(void)
{
    static uint32_t count = 0;
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            count++;
            // ★ 每 5ms：MPU6050 姿态更新（Madgwick 200Hz）
            if (count % 5 == 0) {
                MPU6050_Update();
            }
            // ---------- 每 4ms：速度环 PID + PWM 输出 ----------
            if (count % 4 == 0) {
                Control_UpdateSpeedPID(0, 0, 4.0f);   // 使用固定 dt=4，内部只用 Actual_Speed_A/B
            }

            // ---------- 每 20ms：更新速度反馈 + 灰度环 ----------
            if (count % 20 == 0) {
                // 计算 20ms 编码器增量（此时的 last_enc 还是 20ms 前的值）
                int32_t curr1 = encoder1_raw;
                int32_t curr2 = encoder2_raw;
                encoder1_speed = curr1 - last_enc1;   // 保留此变量供调试打印
                encoder2_speed = curr2 - last_enc2;
                last_enc1 = curr1;
                last_enc2 = curr2;

                // 1.更新实际速度（使用 20ms 增量）
                UpdateSpeedFeedback(encoder1_speed, encoder2_speed, 20.0f);

                // 2.读取灰度并计算误差
                // gray_byte = DL_GPIO_readPins(GPIOB, 0xFF) & 0xFF;
                // gray_error = CalculateGrayError_Advanced(gray_byte);

                // 3.灰度环计算并设定新目标
                // Control_SetGrayTarget(gray_error);
                // 2.角度环：yaw → steering → 叠加到速度目标
                MPU6050_Data_t* mpu = MPU6050_GetHandle();
                float steering = Control_UpdateAngle(mpu->yaw, 20.0f);
                Control_ApplyAngleSteering(steering);
            }
            break;

        default:
            break;
    }
}

    volatile bool Enc1A_edgeFlag = true;  // true:等待上升沿, 0:等待下降沿
    volatile bool Enc1B_edgeFlag = true;
    volatile bool Enc2A_edgeFlag = true;
    volatile bool Enc2B_edgeFlag = true;

void GROUP1_IRQHandler(void)
{
    uint32_t int_status = DL_GPIO_getEnabledInterruptStatus(ENCODER_PORT,
                                    ENCODER_E1A_PIN | ENCODER_E1B_PIN |
                                    ENCODER_E2A_PIN | ENCODER_E2B_PIN);

    // 读取当前所有相关引脚的电平（一次性读取，避免多次调用）
    uint32_t pin_levels = DL_GPIO_readPins(ENCODER_PORT,
                                    ENCODER_E1A_PIN | ENCODER_E1B_PIN |
                                    ENCODER_E2A_PIN | ENCODER_E2B_PIN);

    // 提取各引脚状态（1 为高电平，0 为低电平）
    uint8_t e1a = (pin_levels & ENCODER_E1A_PIN) ? 1 : 0;
    uint8_t e1b = (pin_levels & ENCODER_E1B_PIN) ? 1 : 0;
    uint8_t e2a = (pin_levels & ENCODER_E2A_PIN) ? 1 : 0;
    uint8_t e2b = (pin_levels & ENCODER_E2B_PIN) ? 1 : 0;

    // ---------- 电机1 编码器处理 ----------
    if (int_status & (ENCODER_E1A_PIN | ENCODER_E1B_PIN)) {
        // 使用简单的 XOR 逻辑判断方向
        // 当 A 相触发中断时，若 A == B 则为正向，否则反向
        if (int_status & ENCODER_E1A_PIN) {
            if (e1a == e1b)
                encoder1_raw++;
            else
                encoder1_raw--;
        }
        // 当 B 相触发中断时，若 A != B 则为正向，否则反向
        if (int_status & ENCODER_E1B_PIN) {
            if (e1a != e1b)
                encoder1_raw++;
            else
                encoder1_raw--;
        }
    }

    // ---------- 电机2 编码器处理 ----------
    if (int_status & (ENCODER_E2A_PIN | ENCODER_E2B_PIN)) {
        if (int_status & ENCODER_E2A_PIN) {
            if (e2a == e2b)
                encoder2_raw++;
            else
                encoder2_raw--;
        }
        if (int_status & ENCODER_E2B_PIN) {
            if (e2a != e2b)
                encoder2_raw++;
            else
                encoder2_raw--;
        }
    }

    // 清除所有已处理的中断标志
    DL_GPIO_clearInterruptStatus(ENCODER_PORT, int_status);
}