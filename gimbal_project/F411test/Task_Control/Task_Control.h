#pragma once
#include "PID_Node.h"
#include "PID_Node.h"
#include "ZDT_Pulse_Control.h"
#include "app_protocol.h"
#include "main.h"

#define CENTER_CM  12.5f   /* 梁几何中点 */

float lookup_angle(float pos_cm);   /* 查表: 位置→平衡角(含偏移) */

extern float g_pos_offset;         /* 摄像头位置偏移(cm), 按键一键校准 */
void   Table_CalibrateOffset(float cam_cm, float unused);

void Task3_Init(void);
void Task3_Update(float dt);
void Task3_Control_Send(void);
void Task3_Start(void);
void Task3_Stop(void);
bool Task3_IsRunning(void);
bool Task3_IsDone(void);
uint32_t Task3_GetStep(void);
float Task3_GetTarget(void);
float Task3_GetCurrent(void);
float Task3_GetOutput(void);
float Task3_GetAngle(void);

/* Task4 简化版: 和Task3同文件, 单目标CENTER_CM */
void Task4_Simple_Init(void);
void Task4_Simple_Update(float dt);
void Task4_Simple_Control_Send(void);
void Task4_Simple_Start(void);
void Task4_Simple_Stop(void);
bool Task4_Simple_IsRunning(void);
