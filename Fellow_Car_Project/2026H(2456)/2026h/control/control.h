#ifndef __CONTROL_H
#define __CONTROL_H

#include "PID_Node.h"

extern uint8_t ctrl_mode;

/* 速度反馈 */
extern float Actual_Speed_A;
extern float Actual_Speed_B;

/* 灰度传感器 */
extern volatile uint8_t gray_byte;
extern float gray_error;

/* PID 实例 */
extern PID_Node pidMotor1Speed;
extern PID_Node pidMotor2Speed;
extern PID_Node pidGrayscale;

/* 初始化 */
void Control_Init(void);
void Control_Update(float dt);
void Control_UpdateSpeedFeedback(int32_t diff_A, int32_t diff_B, float dt);

/* 灰度滤波 */
uint8_t Control_GrayByte_Window_Filter(size_t window_size);

/* 路口检测 */
bool Control_IsCrossDetected(uint8_t threshold);

/* 到达终点 (全白) */
void Control_Start(void);
void Control_Stop(void);
void Control_SetRampEnabled(bool en);
void Control_SetDecelDistance(float target_distance);
void Control_CheckDecel(float current_dist);
bool Control_IsAtEnd(void);
float Control_GetAverageSpeed(void);
float Control_GetCurrentDistance(void);

#endif
