#pragma once
#include "PID_Node.h"
#include "ZDT_Pulse_Control.h"
#include "app_protocol.h"
#include "main.h"

void Task4_Init(void);
void Task4_Update(float dt);
void Task4_Control_Send(void);
void Task4_Start(void);
void Task4_Stop(void);

bool     Task4_IsRunning(void);
bool     Task4_IsDone(void);
uint32_t Task4_GetStep(void);
float    Task4_GetTarget(void);
float    Task4_GetCurrent(void);
float    Task4_GetOutput(void);
