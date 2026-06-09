/**
 * @file PID_Node.h
 * @author whyP762 (3046961251@qq.com)
 * @brief PID节点和链表的定义与接口   
 * @version 0.1
 * @date 2026-03-29
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef __PID_NODE_H__
#define __PID_NODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "./Log/Log.h"  // 重要依赖，确保日志功能可用

typedef struct feedforward
{
    float feedforward_Kp;                           // 前馈系数
    float (*feedforward_values)(float target);      // 前馈值获取函数,根据目标值返回前馈值
} feedforward;

typedef struct measured_callback
{
    float (*get_measured_value)(void);              // 获取测量值的回调函数
} measured_callback;

typedef enum
{
    FEEDFORWARD_TYPE_SINGLE = 0,        // 单一前馈，使用feedforward_Kp乘以测量值作为前馈输出
    FEEDFORWARD_TYPE_TABLE,             // 表格前馈，使用feedforward_values函数根据设定值获取前馈输出
    NO_FEEDFORWARD,                     // 无前馈
} FeedForwardType;
typedef struct PID_Node_Flag
{
    bool isInitialized;                 // 是否初始化
    bool isEnabled;                     // 是否启用
    bool isUserDefinedMeasuredValue;     // 是否使用用户定义的测量值回调函数
    bool isUserBaseValue;                 // 是否使用用户定义的基准值
    FeedForwardType feedforward_type;   // 前馈类型
} PID_Node_Flag;

typedef enum
{
    PID_SUCCESS = 0,                    // 成功
    PID_OUTPUT_OVERFLOW,                // 输出溢出
    PID_INVALID_PARAMETER,              // 无效参数
    PID_NO_ENOUGH_NODES,                // 没有足够的节点(暂时弃用)
} PID_RETURN_CORE;

typedef struct{
    float (*custom_error_calculation)(float setpoint, float measured_value); // 用户自定义误差计算函数指针
} PID_Custom_Functions;

typedef struct PID_Limit
{
    float setpoint_min;              // 设定值最小值
    float setpoint_max;              // 设定值最大值
    float input_min;                // 输入最小值
    float input_max;                // 输入最大值
    float output_min;               // 输出最小值
    float output_max;               // 输出最大值
    float integral_max;             // 积分最小值
    float derivative_max;           // 微分最大值
    float deadband;                 // 死区范围
} PID_Limit;

typedef struct PID_Data
{
    float error;                    // 当前误差
    float previous_error;           // 上一次误差
    float integral_sum;             // 积分值
    float derivative;               // 微分值
} PID_Data;

typedef struct
{
    float kp;                                       // 比例系数
    float ki;                                       // 积分系数
    float kd;                                       // 微分系数 
    float SetPointBaseValue;                        // 设定值基准值,如果启用则会加到设定值上
    float integral_attenuation_Kp;                  // 积分衰减系数
    feedforward feedforward;                        // 前馈参数
    measured_callback measured_callback;            // 测量值回调函数
} PID_Parameters;
    
typedef struct PID_Node 
{
    PID_Custom_Functions custom_functions;              // 用户自定义函数集合
    char name[20];                      // PID节点名称
    float setpoint;                     // 设定值
    float measured_value;               // 测量值
    float output;                       // PID输出
    PID_Parameters parameters;          // PID参数
    PID_Data data;                      // PID计算数据
    PID_Limit limit;                    // PID限制
    PID_Node_Flag flag;                 // PID节点状态标志
    struct PID_Node* prev;              // 指向上一个节点
    struct PID_Node* next;              // 指向下一个节点
} PID_Node;

typedef struct PID_Link
{
    PID_Node* head;                    // 链表头节点
    PID_Node* tail;                    // 链表尾节点
    size_t size;                       // 链表大小
}PID_Link;

/* ****API**** */

PID_RETURN_CORE PID_Link_Init(PID_Link* link);
PID_RETURN_CORE PID_Link_AddNode(PID_Link* link, PID_Node* node);
PID_RETURN_CORE PID_Link_RemoveNode(PID_Link* link, const char* name);
PID_RETURN_CORE PID_Link_InsertNode(PID_Link* link, PID_Node* node_prev, PID_Node* node);
PID_RETURN_CORE PID_Link_Output(PID_Link* link,float* tail_value);

PID_RETURN_CORE PID_Node_Init(PID_Node* node, const char* name, float kp, float ki, float kd);

PID_RETURN_CORE PID_Node_UpdateMeasurement(PID_Node* node, float measured_value);
PID_RETURN_CORE PID_ExecuteNode(PID_Node* node, float dt);
PID_RETURN_CORE PID_Node_Link_Update(PID_Link* link,float dt);

PID_RETURN_CORE PID_Node_SetEnabled(PID_Node* node, bool enabled);
PID_RETURN_CORE PID_Node_SetKp(PID_Node* node , float kp);
PID_RETURN_CORE PID_Node_SetKi(PID_Node* node , float Ki);
PID_RETURN_CORE PID_Node_SetKd(PID_Node* node , float Kd);
PID_RETURN_CORE PID_Node_SetSetpoint(PID_Node* node , float setpoint);
PID_RETURN_CORE PID_Node_SetIntegralAttenuationKp(PID_Node* node , float integral_attenuation_Kp);
PID_RETURN_CORE PID_Node_SetFeedForward(PID_Node* node, FeedForwardType feedforward_type, float feedforward_Kp, float (*feedforward_values_callback)(float target));
PID_RETURN_CORE PID_Node_SetLimit(PID_Node* node,PID_Limit limit);

PID_RETURN_CORE PID_Node_SetMaxInput(PID_Node* node, float max_input);
PID_RETURN_CORE PID_Node_SetMinInput(PID_Node* node, float min_input);

PID_RETURN_CORE PID_Node_SetMaxOutput(PID_Node* node, float max_output);
PID_RETURN_CORE PID_Node_SetMinOutput(PID_Node* node, float min_output);

PID_RETURN_CORE PID_Node_SetMaxIntegral(PID_Node* node, float max_integral);
PID_RETURN_CORE PID_Node_SetMaxDerivative(PID_Node* node, float max_derivative);
PID_RETURN_CORE PID_Node_SetDeadband(PID_Node* node, float deadband);
PID_RETURN_CORE PID_Node_ResetIntegral(PID_Node* node);

PID_RETURN_CORE PID_Node_SetUserDefinedMeasuredValue(PID_Node* node, bool isEnble ,float (*get_measured_value_callback)(void));
PID_RETURN_CORE PID_Node_SetUserBaseValue(PID_Node* node, bool isEnble , float base_value);

PID_RETURN_CORE PID_Node_SetCustomCallback(PID_Node* node, PID_Custom_Functions custom_functions);


void PID_AllNode(PID_Link* link, void (*operation)(PID_Node* node));

#endif 
