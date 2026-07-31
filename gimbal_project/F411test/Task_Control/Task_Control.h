#pragma once
#include "PID_Node.h"
#include "ZDT_Motor_Serial.h"
#include "app_protocol.h"
#include "main.h"

void Task3_Init(ZDT_Motor_Handle_t* motor);
void Task3_Update(float dt);
bool Task3_IsDone(void);
uint32_t Task3_GetStep(void);
float Task3_GetTarget(void);
float Task3_GetCurrent(void);
void Task3_Control_Send(void);
float Task3_GetOutput(void);
