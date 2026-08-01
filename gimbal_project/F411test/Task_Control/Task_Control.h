#pragma once
#include "PID_Node.h"
#include "PID_Node.h"
#include "ZDT_Pulse_Control.h"
#include "app_protocol.h"
#include "main.h"

#define CENTER_CM  12.5f   /* 梁几何中点 */

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
