#include "./PID_Node.h"


typedef struct PID_Flag
{
    bool isInitialized;
    bool isEnabled;
}PID_Flag;

static bool PID_Node_SetKP(PID_Node* self,float Kp)
{
    if (!self)
    {
        return false;
    }
    
}

bool PID_Node_Init(PID_Node* node)
{
    if (!node)
    {
        return false;
    }
    node->flag.isEnabled = false;
    node->pointer.next = NULL;
    node->pointer.parent = NULL;
    node->threshold.max_output = 0.0f;
    node->threshold.min_output = 0.0f;
    node->threshold.dead_zone = 0.0f;
    node->threshold.integral_limit = 0.0f;
    node->threshold.differential_limit = 0.0f;
    node->parameters.Kp = 0.0f;
    node->parameters.Ki = 0.0f;
    node->parameters.Kd = 0.0f;
    node->parameters.dt = 0.0f;
    node->calculate_data.sum_error = 0.0f;
    node->calculate_data.last_error = 0.0f;
    node->calculate_data.current_error = 0.0f;
    node->calculate_data.output = 0.0f;
    node->calculate_data.last_output = 0.0f;
    node->calculate_data.normalization_factor = 1.0f;
    node->flag.isInitialized = true;
    return true;
}

bool PID_Node_Link(PID_Node* self,PID_Node* parent)
{
    if (!self || !parent || !self->flag.isInitialized || !parent->flag.isInitialized)
    {
        return false;
    }
    self->pointer.next = NULL;
    self->pointer.parent = parent;
    parent->pointer.next = self;
    return true;
}

bool PID_Node_SetKp(PID_Node* self,float Kp)
{
    if (!self)
    {
        return false;
    }
    self->parameters.Kp = Kp;
    return true;
}

bool PID_Node_SetKi(PID_Node* self,float Ki)
{
    if (!self)
    {
        return false;
    }
    self->parameters.Ki = Ki;
    return true;
}

bool PID_Node_SetKd(PID_Node* self,float Kd)
{
    if (!self)
    {
        return false;
    }
    self->parameters.Kd = Kd;
    return true;
}

bool PID_Node_SetParameters(PID_Node* self,float Kp,float Ki,float Kd)
{
    if (!self)
    {
        return false;
    }
    self->parameters.Kp = Kp;
    self->parameters.Ki = Ki;
    self->parameters.Kd = Kd;
    return true;
}

bool PID_Node_SetThreshold(PID_Node* self,PID_Threshold* threshold)
{
    if (!self || !threshold)
    {
        return false;
    }
    self->threshold.max_output = threshold->max_output;
    self->threshold.min_output = threshold->min_output;
    self->threshold.dead_zone = threshold->dead_zone;
    self->threshold.integral_limit = threshold->integral_limit;
    self->threshold.differential_limit = threshold->differential_limit;
    return true;
}


bool PID_Node_IsInitialized(PID_Node* self)
{
    if (!self)
    {
        return false;
    }
    return self->flag.isInitialized;
}

bool PID_Node_IsEnabled(PID_Node* self)
{
    if (!self)
    {
        return false;
    }
    return self->flag.isEnabled;
}

bool PID_Node_Enable(PID_Node* self)
{
    if (!self || !self->flag.isInitialized)
    {
        return false;
    }
    self->flag.isEnabled = true;
    if (self->pointer.next && self->pointer.next->threshold.max_output > 1e-6f 
        && self->threshold.min_output > 1e-6f)  // 修正条件：确保 max_output > 0
    {
        // 根据子节点 max_output 归一化：计算因子，将自己的输出范围映射到子节点输入范围
        float child_max_min_output = fabsf(self->pointer.next->threshold.max_output - self->pointer.next->threshold.min_output);
        float self_max_min_output = fabsf(self->threshold.max_output - self->threshold.min_output);
        if (child_max_min_output > 1e-6f && self_max_min_output > 1e-6f) // 确保分母不为零
        {
            self->calculate_data.normalization_factor = child_max_min_output / self_max_min_output;
        }
        else
        {
            self->calculate_data.normalization_factor = 1.0f; // 默认归一化因子为1，避免除以零
        }
    }
    
    return true;
}

bool PID_Node_Disable(PID_Node* self)
{
    if (!self || !self->flag.isInitialized)
    {
        return false;
    }
    self->flag.isEnabled = false;
    return true;
}

PID_Node* PID_Node_GetParent(PID_Node* self)
{
    if (!self || !self->flag.isInitialized)
    {
        return NULL;
    }
    return self->pointer.parent;
}

PID_Node* PID_Node_GetChild(PID_Node* self)
{
    if (!self || !self->flag.isInitialized)
    {
        return NULL;
    }
    return self->pointer.next;
}