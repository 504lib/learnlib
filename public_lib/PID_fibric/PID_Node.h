#ifndef __PID_NODE_H__
#define __PID_NODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "./Log/Log.h"


typedef struct PID_Flag PID__Flag;

typedef struct PID_Node_Pointer
{
    struct PID_Node* next;
    struct PID_Node* parent;
}PID_Node_Pointer;

typedef struct PID_Threshold
{
    float max_output;
    float min_output;
    float dead_zone;
    float integral_limit;
    float differential_limit;
}PID_Threshold;

typedef struct PID_parameters
{
    float Kp;
    float Ki;
    float Kd;
    float dt;
}PID_parameters;

typedef struct PID_Calculate_Data
{
    float sum_error;
    float last_error;
    float current_error;
    float output;
    float last_output;
    float normalization_factor;
}PID_Calculate_Data;

typedef struct PID_Node
{
    PID_Node_Pointer pointer;
    PID_Flag flag;
    PID_Threshold threshold;
    PID_parameters parameters;
    PID_Calculate_Data calculate_data;
}PID_Node;


/* 初始化函数BEGIN */
bool PID_Node_Init(PID_Node* node);
bool PID_Node_Link(PID_Node* self,PID_Node* parent);
/* 初始化函数END */

/* 参数设置函数BEGIN */
bool PID_Node_SetKp(PID_Node* self,float Kp);
bool PID_Node_SetKi(PID_Node* self,float Ki);
bool PID_Node_SetKd(PID_Node* self,float Kd);
bool PID_Node_SetParameters(PID_Node* self,float Kp,float Ki,float Kd);
bool PID_Node_SetThreshold(PID_Node* self,PID_Threshold* threshold);
/* 参数设置函数END */

/* 状态查询和控制函数BEGIN */
bool PID_Node_IsInitialized(PID_Node* self);
bool PID_Node_IsEnabled(PID_Node* self);
/* 状态查询和控制函数END */

/* 使能控制函数BEGIN */
bool PID_Node_Enable(PID_Node* self);
bool PID_Node_Disable(PID_Node* self);
/* 使能控制函数END */

/* Get父母子BEGIN */
PID_Node* PID_Node_GetParent(PID_Node* self);
PID_Node* PID_Node_GetChild(PID_Node* self);
/* Get父母子END */


#endif 