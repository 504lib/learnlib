#include "./PID_Node.h"

inline static void SetMeasuredValue(PID_Node* node, float measured_value);
inline static void SetOutput(PID_Node* node, float output);
inline static void SetError(PID_Node* node, float error);
static void UpdateMeasuredValueFromPrev(PID_Node* node);
static bool HandleInvalidOutputRange(PID_Node* node);
static bool HandleDisabledNode(PID_Node* node);
static float ComputeError(PID_Node* node);
static float UpdateIntegral(PID_Node* node, float error, float dt);
static float ComputeDerivative(PID_Node* node, float error, float dt);
static float ApplyFeedforward(PID_Node* node, float pid_output);
static inline bool SearchTargetStringNode(PID_Link* link, const char* name, PID_Node** out_node);
static inline bool IsNodeInList(PID_Link* link, PID_Node* node);
static void PID_ExecuteNode(PID_Node* node, float dt);



/**
 * @brief    更新当前节点的测量值，从前驱节点的输出映射而来
 * 
 * @param    node      function of param
 */
static void UpdateMeasuredValueFromPrev(PID_Node* node)
{
    if (!node->prev) return;

    float prev_range = node->prev->limit.output_max - node->prev->limit.output_min;
    if (fabsf(prev_range) < 1e-6f) return;  // 前驱范围无效，保持原测量值

    float offset_ratio = (node->prev->output - node->prev->limit.output_min) / prev_range;
    float range = node->limit.output_max - node->limit.output_min;
    float new_measured = node->limit.output_min + offset_ratio * range;
    SetMeasuredValue(node, new_measured);
}

/**
 * @brief    处理输出范围无效的情况，如果输出范围过小则直接将测量值作为输出，并返回true表示已处理
 * 
 * @param    node      PID节点 
 * @return   true      已经处理了无效输出范围的情况，调用者应该直接返回
 * @return   false     输出范围有效，调用者可以继续执行PID计算
 */
static bool HandleInvalidOutputRange(PID_Node* node)
{
    if (fabsf(node->limit.output_max - node->limit.output_min) >= 1e-6f)
        return false;  // 范围有效

    LOG_WARN("PID_ExecuteNode: Invalid output limits for node '%s', output range is too small", node->name);
    SetOutput(node, node->measured_value);
    return true;
}

/**
 * @brief    检测节点是否使能，如果节点被禁用则直接将测量值作为输出，并返回true表示已处理
 * 
 * @param    node      PID节点
 * @return   true      节点被禁用，已经处理了禁用节点的情况，调用者应该直接返回
 * @return   false     节点使能，调用者可以继续执行PID计算
 */
static bool HandleDisabledNode(PID_Node* node)
{
    if (node->flag.isEnabled)
        return false;

    LOG_DEBUG("PID_ExecuteNode: Node '%s' is disabled, skipping execution", node->name);
    SetOutput(node, node->measured_value);
    return true;
}

/**
 * @brief    计算当前误差，并更新节点的误差数据。误差计算包含死区处理，如果误差在死区范围内则视为0。
 * 
 * @param    node      PID节点
 * @return   float     当前误差值（可能已经经过死区处理）
 */
static float ComputeError(PID_Node* node)
{
    float error = node->setpoint - node->measured_value;
    if (error < node->limit.deadband && error > -node->limit.deadband)
        error = 0.0f;
    SetError(node, error);
    return error;
}

/**
 * @brief    更新积分值，并计算积分项的贡献。包含积分衰减机制，当误差非常小时会逐渐衰减积分值以防止积分风up。
 * 
 * @param    node      PID节点
 * @param    error     当前误差值
 * @param    dt        时间增量，单位tick,通常是ms
 * @return   float     积分项的贡献值（已经乘以积分系数ki）
 */
static float UpdateIntegral(PID_Node* node, float error, float dt)
{
    if (fabsf(error) < 1e-6f)
    {
        // 误差极小，积分衰减
        node->data.integral_sum *= node->parameters.integral_attenuation_Kp;
    }
    else
    {
        node->data.integral_sum += error * dt;
        // 积分限幅
        if (fabsf(node->data.integral_sum) > node->limit.integral_max)
        {
            node->data.integral_sum = (node->data.integral_sum > 0) ?
                                      node->limit.integral_max : -node->limit.integral_max;
        }
    }
    return node->parameters.ki * node->data.integral_sum;
}

/**
 * @brief    计算微分项的贡献。包含微分死区处理，如果误差变化非常小则微分项视为0。同时对微分值进行限幅以防止微分风up。
 * 
 * @param    node      PID节点
 * @param    error     当前误差值
 * @param    dt        时间增量，单位tick,通常是ms
 * @return   float     微分项的贡献值（已经乘以微分系数kd）
 */
static float ComputeDerivative(PID_Node* node, float error, float dt)
{
    float derivative;
    if (fabsf(error - node->data.previous_error) < 1e-6f)
    {
        derivative = 0.0f;
    }
    else
    {
        derivative = (error - node->data.previous_error) / dt;
        if (fabsf(derivative) > node->limit.derivative_max)
        {
            derivative = (derivative > 0) ? node->limit.derivative_max : -node->limit.derivative_max;
        }
    }
    node->data.derivative = derivative;
    return node->parameters.kd * derivative;
}

/**
 * @brief    请求前馈值并将其应用到PID输出上。根据节点的前馈类型，可能会使用单一前馈系数乘以测量值，或者调用前馈表函数获取前馈值。如果没有有效的前馈配置，则直接返回原PID输出。
 * 
 * @param    node      PID节点
 * @param    pid_output PID计算得到的输出值（不包含前馈）
 * @return   float     最终的输出值（包含前馈贡献）
 */
static float ApplyFeedforward(PID_Node* node, float pid_output)
{
    if (node->flag.feedforward_type == FEEDFORWARD_TYPE_SINGLE)
    {
        return pid_output + node->parameters.feedforward.feedforward_Kp * node->measured_value;
    }
    else if (node->flag.feedforward_type == FEEDFORWARD_TYPE_TABLE &&
             node->parameters.feedforward.feedforward_values)
    {
        float ff = node->parameters.feedforward.feedforward_values(node->setpoint);
        return pid_output + ff;
    }
    else // NO_FEEDFORWARD or unknown
    {
        return pid_output;
    }
}

/**
 * @brief    在链表中搜索指定名称的节点，如果找到则返回true并通过out_node输出节点指针，否则返回false。
 * 
 * @param    link      link链表指针
 * @param    name      要搜索的节点名称
 * @param    out_node  如果找到节点，输出该节点的指针；如果未找到或参数无效，则不修改out_node
 * @return   true      找到名称匹配的节点，并且如果out_node不为NULL，则通过out_node输出该节点的指针
 * @return   false     未找到名称匹配的节点，或者参数无效（如link或name为NULL），此时out_node不被修改
 */
inline static bool SearchTargetStringNode(PID_Link* link, const char* name, PID_Node** out_node)
{
    if (!link || !name)
    {
        LOG_WARN("SearchTargetStringNode: Invalid parameter - link or name is NULL");
        return false;
    }
    PID_Node* current = link->head;
    while (current)
    {
        if (strcmp(current->name, name) == 0)
        {
            if (out_node)
            {
                *out_node = current;
            }
            return true;
        }
        current = current->next;
    }
    return false;
}


/**
 * @brief    检查指定节点是否在链表中。通过遍历链表检查节点指针是否存在于链表的节点中。如果找到匹配的节点指针则返回true，否则返回false。
 * 
 * @param    link      link链表指针
 * @param    node      要检查的节点指针
 * @return   true      找到节点指针在链表中存在
 * @return   false     未找到节点指针在链表中存在，或者参数无效（如link或node为NULL）
 */
inline static bool IsNodeInList(PID_Link* link, PID_Node* node)
{
    if (!link || !node)
    {
        LOG_WARN("IsNodeInList: Invalid parameter - link or node is NULL");
        return false;
    }
    PID_Node* current = link->head;
    while (current)
    {
        if (current == node)
        {
            return true;
        }
        current = current->next;
    }
    return false;
}


/**
 * @brief    执行单个PID节点的计算逻辑。包含测量值更新、输出范围检查、节点使能检查、误差计算、PID计算以及前馈应用等完整的执行流程。通过分步骤处理不同的情况，使得代码结构清晰且易于维护。
 * 
 * @param    node      PID节点
 * @param    measured_value 当前节点的测量值（通常来自前驱节点的输出映射）
 */
inline static void SetMeasuredValue(PID_Node* node, float measured_value)
{
    if (!node)
    {
        LOG_WARN("SetMeasuredValue: Invalid parameter - node is NULL");
        return;
    }
    node->measured_value = measured_value;
}


/**
 * @brief    设置PID节点的输出值，并进行输出限幅处理。如果输出值超过设定的最大或最小限制，则将输出值限制在范围内。否则直接设置输出值。
 * 
 * @param    node      PID节点
 * @param    output    要设置的输出值，可能会被限幅处理
 */
inline static void SetOutput(PID_Node* node, float output)
{
    if (!node)
    {
        LOG_WARN("SetOutput: Invalid parameter - node is NULL");
        return;
    }
    if (output > node->limit.output_max)
    {
        node->output = node->limit.output_max; // 输出限幅
    }
    else if (output < node->limit.output_min)
    {
        node->output = node->limit.output_min; // 输出限幅
    }
    else
    {
        node->output = output;
    }
}

/**
 * @brief    设置PID节点的当前误差值，并更新节点的数据结构。这个函数主要用于将计算得到的误差值保存到节点的数据中，以供后续的积分和微分计算使用。
 * 
 * @param    node      PID节点
 * @param    error     当前误差值，通常是设定值与测量值的差值，可能已经经过死区处理
 */
inline static void SetError(PID_Node* node, float error)
{
    if (!node)
    {
        LOG_WARN("SetError: Invalid parameter - node is NULL");
        return;
    }
    node->data.error = error;
}

/**
 * @brief    执行单个PID节点的计算逻辑。包含测量值更新、输出范围检查、节点使能检查、误差计算、PID计算以及前馈应用等完整的执行流程。通过分步骤处理不同的情况，使得代码结构清晰且易于维护。
 * 
 * @param    node      PID节点
 * @param    dt        时间增量，单位tick,通常是ms
 */
static void PID_ExecuteNode(PID_Node* node, float dt)
{
    if (dt <= 0.0f)
    {
        LOG_WARN("PID_ExecuteNode: dt must be positive");
        return;
    }
    if (!node)
    {
        LOG_WARN("PID_ExecuteNode: node is NULL");
        return;
    }

    // 1. 更新测量值（无论后续如何处理，都要先从前驱获取最新值）
    UpdateMeasuredValueFromPrev(node);

    // 2. 输出范围无效 -> 直通测量值并返回
    if (HandleInvalidOutputRange(node))
        return;

    // 3. 节点禁用 -> 直通测量值并返回
    if (HandleDisabledNode(node))
        return;

    // 4. 计算误差（会更新 node->data.error）
    float error = ComputeError(node);

    // 5. 比例项
    float proportional = node->parameters.kp * error;

    // 6. 积分项（会更新积分和）
    float integral_contrib = UpdateIntegral(node, error, dt);

    // 7. 微分项（会更新导数）
    float derivative_contrib = ComputeDerivative(node, error, dt);

    // 8. PID 输出
    float pid_output = proportional + integral_contrib + derivative_contrib;

    // 9. 前馈
    float final_output = ApplyFeedforward(node, pid_output);

    // 10. 设置输出并保存本次误差供下次微分使用
    SetOutput(node, final_output);
    node->data.previous_error = error;
}

/**
 * @brief    遍历链表中的所有节点，并对每个节点执行指定的操作函数。这个函数提供了一种方便的方式来对链表中的每个PID节点进行统一的处理，例如更新、打印状态等。调用者需要提供一个操作函数，该函数接受一个PID_Node指针作为参数，对该节点执行所需的操作。
 * 
 * @param    link      PID节点链表
 * @param    operation 对每个节点执行的操作函数，函数接受一个PID_Node指针作为参数
 */
void PID_AllNode(PID_Link* link, void (*operation)(PID_Node* node))
{
    if (!link || !operation)
    {
        LOG_WARN("PID_AllNode: Invalid parameter - link or operation is NULL");
        return;
    }
    PID_Node* current = link->head;
    while (current)
    {
        operation(current);
        current = current->next;
    }
}

/**
 * @brief    获取当前节点的下一个节点指针。如果当前节点为NULL，则返回NULL。调用者可以使用这个函数来遍历链表中的节点。
 * 
 * @param    node      PID节点
 * @return   PID_Node* 下一个节点的指针，如果当前节点为NULL或没有下一个节点，则返回NULL
 */
static PID_Node* PID_GetNext(PID_Node* node)
{
    if (!node)
    {
        LOG_WARN("PID_GetNext: Invalid parameter - node is NULL");
        return NULL;
    }
    return node->next;
}

/**
 * @brief    获取当前节点的上一个节点指针。如果当前节点为NULL，则返回NULL。调用者可以使用这个函数来遍历链表中的节点。
 * 
 * @param    node      PID节点
 * @return   PID_Node* 上一个节点的指针，如果当前节点为NULL或没有上一个节点，则返回NULL
 */
static PID_Node* PID_GetPrev(PID_Node* node)
{
    if (!node)
    {
        LOG_WARN("PID_GetPrev: Invalid parameter - node is NULL");
        return NULL;
    }
    return node->prev;
}

/**
 * @brief    初始化PID链表。将链表的头节点和尾节点指针设置为NULL，大小设置为0。调用者需要在使用链表之前调用这个函数来确保链表处于初始状态。
 * 
 * @param    link      PID链表指针
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER
 */
PID_RETURN_CORE PID_Link_Init(PID_Link* link)
{
    if (!link)
    {
        LOG_WARN("PID_Link_Init: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    }
    link->head = NULL;
    link->tail = NULL;
    link->size = 0;
    return PID_SUCCESS;
}


/**
 * @brief    向PID链表中添加一个节点。将新节点添加到链表的末尾，并更新链表的大小。调用者需要确保要添加的节点已经正确初始化，并且链表已经通过PID_Link_Init函数初始化。
 * 
 * @param    link      PID链表指针
 * @param    node      要添加的PID节点指针，调用者需要确保该节点已经正确初始化，并且没有被添加到其他链表中
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER，如果链表中已经存在同名节点也返回PID_INVALID_PARAMETER
 */
PID_RETURN_CORE PID_Link_AddNode(PID_Link* link, PID_Node* node)
{
    if(!link)
    {
        LOG_WARN("PID_Link_AddNode: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    } 
    if(!node)
    {
        LOG_WARN("PID_Link_AddNode: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (!node->flag.isInitialized)
    {
        LOG_WARN("PID_Link_AddNode: Invalid parameter - node '%s' is not initialized", node->name);
        return PID_INVALID_PARAMETER;
    }
    const bool has_target = SearchTargetStringNode(link, node->name, NULL); // 检查是否有同名节点,如果有则不添加
    if (has_target)
    {
            LOG_WARN("PID_Link_AddNode: Node with name '%s' already exists in the link", node->name);
            return PID_INVALID_PARAMETER;
    }
    if(!link->head && !link->tail)
    {
        link->head = node;
        link->tail = node;
    }
    else
    {
        link->tail->next = node;
        node->prev = link->tail;
        link->tail = node;
    }
    link->size++;
    return PID_SUCCESS;
}

/**
 * @brief    从PID链表中移除一个节点。根据节点名称搜索链表中的节点，如果找到则将其从链表中移除，并更新链表的连接关系和大小。调用者需要确保链表已经通过PID_Link_Init函数初始化，并且要移除的节点名称存在于链表中。
 * 
 * @param    link      PID链表指针
 * @param    name      要移除的节点名称，调用者需要确保该名称存在于链表中，否则函数将返回PID_INVALID_PARAMETER
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER，如果链表中没有足够的节点可供移除则返回PID_NO_ENOUGH_NODES
 */
PID_RETURN_CORE PID_Link_RemoveNode(PID_Link* link, const char* name)
{
    if (!link)
    {
        LOG_WARN("PID_Link_RemoveNode: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    }
    if(!link->head || !link->tail || link->size == 0)
    {
        LOG_WARN("PID_Link_RemoveNode: No nodes to remove - link is empty");
        return PID_NO_ENOUGH_NODES;
    }
    PID_Node* current = link->head;
    while (current)
    {
        if (strcmp(current->name, name) == 0)
        {
            if (current->prev)
            {
                current->prev->next = current->next;
            }
            else
            {
                link->head = current->next;
            }
            if (current->next)
            {
                current->next->prev = current->prev;
            }
            else
            {
                link->tail = current->prev;
            }
            current->prev = NULL;
            current->next = NULL;
            link->size--;
            return PID_SUCCESS;
        }
        current = current->next;
    }
    LOG_WARN("PID_Link_RemoveNode: Node with name '%s' not found", name);
    return PID_INVALID_PARAMETER;
}


/**
 * @brief    从链表中的指定节点后插入一个新节点。根据node_prev参数指定的节点，在其后插入新的节点，并更新链表的连接关系和大小。调用者需要确保链表已经通过PID_Link_Init函数初始化，node_prev节点存在于链表中，并且要插入的节点已经正确初始化且没有被添加到其他链表中。
 * 
 * @param    link      PID链表指针
 * @param    node_prev 要插入新节点的前驱节点，调用者需要确保该节点存在于链表中，否则函数将返回PID_INVALID_PARAMETER
 * @param    node      要插入的新节点，调用者需要确保该节点已经正确初始化，并且没有被添加到其他链表中，否则函数将返回PID_INVALID_PARAMETER
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER，如果链表中没有足够的节点可供插入则返回PID_NO_ENOUGH_NODES
 */
PID_RETURN_CORE PID_Link_InsertNode(PID_Link* link, PID_Node* node_prev, PID_Node* node)
{
    if (!link)
    {
        LOG_WARN("PID_Link_TargetNode: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (!node_prev)
    {
        LOG_WARN("PID_Link_TargetNode: Invalid parameter - node_prev is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (!node)
    {
        LOG_WARN("PID_Link_TargetNode: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    } 
    if (!node->flag.isInitialized)
    {
        LOG_WARN("PID_Link_TargetNode: Invalid parameter - node '%s' is not initialized", node->name);
        return PID_INVALID_PARAMETER;
    }
    const bool has_target = SearchTargetStringNode(link, node->name, NULL); // 检查是否有同名节点,如果有则不添加
    if (has_target)
    {
        LOG_WARN("PID_Link_InsertNode: Node with name '%s' already exists in the link", node->name);
        return PID_INVALID_PARAMETER;
    }
    if(!link->head || !link->tail || link->size == 0)
    {
        LOG_WARN("PID_Link_TargetNode: No nodes to target - link is empty");
        return PID_NO_ENOUGH_NODES;
    }
    PID_Node* current = link->head;
    while (current)
    {
        if (current == node_prev)
        {
            PID_Node* next_node = current->next;
            current->next = node;
            node->prev = current;
            if (next_node)
            {
                node->next = next_node;
                next_node->prev = node;
            }
            else
            {
                link->tail = node;
            }
            link->size++;
            return PID_SUCCESS;
        }
        current = current->next;
    }
    LOG_WARN("PID_Link_TargetNode: node_prev not found in the link");
    return PID_INVALID_PARAMETER;
}


/**
 * @brief    设置PID节点的使能状态。如果enabled参数为true，则启用该节点；如果enabled参数为false，则禁用该节点。调用者需要确保node参数指向一个有效的PID节点，否则函数将返回PID_INVALID_PARAMETER。
 * 
 * @param    node      PID节点指针，调用者需要确保该指针指向一个有效的PID节点，否则函数将返回PID_INVALID_PARAMETER
 * @param    enabled   要设置的使能状态，true表示启用节点，false表示禁用节点
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER
 */
PID_RETURN_CORE PID_Node_SetEnabled(PID_Node* node, bool enabled)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetEnabled: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->flag.isEnabled = enabled;
    return PID_SUCCESS;
}


/**
 * @brief    PID节点初始化函数。根据提供的参数初始化PID节点的各项属性，包括名称、PID参数、积分衰减系数、前馈配置、数据结构、限制范围以及标志位等。调用者需要确保node参数指向一个有效的PID节点，并且name参数指向一个有效的字符串，否则函数将返回PID_INVALID_PARAMETER。
 * 
 * @param    node      PID节点指针，调用者需要确保该指针指向一个有效的PID节点，否则函数将返回PID_INVALID_PARAMETER
 * @param    name      节点名称，调用者需要确保该参数指向一个有效的字符串，否则函数将返回PID_INVALID_PARAMETER
 * @param    kp        函数的比例系数
 * @param    ki        函数的积分系数
 * @param    kd        函数的微分系数
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER
 */
PID_RETURN_CORE PID_Node_Init(PID_Node* node, const char* name, float kp, float ki, float kd)
{
    if (!node)
    {
        LOG_WARN("PID_Node_Init: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->setpoint = 0.0f;
    node->measured_value = 0.0f;
    node->output = 0.0f;
    node->parameters.kp = kp;
    node->parameters.ki = ki;
    node->parameters.kd = kd;
    node->parameters.integral_attenuation_Kp = 0.9f;
    node->parameters.feedforward.feedforward_Kp = 0.0f;
    node->parameters.feedforward.feedforward_values = NULL;
    node->data.error = 0.0f;
    node->data.previous_error = 0.0f;
    node->data.integral_sum = 0.0f;
    node->data.derivative = 0.0f;
    node->limit.output_min = -1000.0f;
    node->limit.output_max = 1000.0f;
    node->limit.integral_max = 1000.0f;
    node->limit.derivative_max = 1000.0f;
    node->limit.deadband = 0.0f;
    node->flag.isInitialized = true;
    node->flag.isEnabled = true;
    node->flag.feedforward_type = NO_FEEDFORWARD;
    node->prev = NULL;
    node->next = NULL;
    return PID_SUCCESS;
}


/**
 * @brief    更新PID链表中所有节点的输出值。根据输入的测量值和时间增量，遍历链表中的每个节点并执行PID计算逻辑来更新其输出值。调用者需要确保链表已经通过PID_Link_Init函数初始化，并且链表中至少有一个节点，否则函数将返回PID_NO_ENOUGH_NODES。
 * 
 * @param    L         PID链表指针，调用者需要确保该指针指向一个已经初始化的PID链表，并且链表中至少有一个节点，否则函数将返回PID_NO_ENOUGH_NODES
 * @param    head_measured_value 链表头节点的测量值，调用者需要根据实际情况提供正确的测量值，这个值通常来自于系统的传感器或其他数据源
 * @param    dt        时间增量，单位tick,通常是ms，调用者需要确保该值为正数，否则函数将返回PID_INVALID_PARAMETER
 * @return   PID_RETURN_CORE 返回函数执行结果，成功返回PID_SUCCESS，参数无效返回PID_INVALID_PARAMETER，如果链表中没有足够的节点可供更新则返回PID_NO_ENOUGH_NODES
 */
PID_RETURN_CORE PID_Node_Update(PID_Link* L, float head_measured_value,float dt)
{
    if (dt <= 0.0f)
    {
        LOG_WARN("PID_Node_Update: Invalid parameter - dt should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    if (!L)
    {
        LOG_WARN("PID_Node_Update: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    }
    if(!L->head || !L->tail || L->size == 0)
    {
        LOG_WARN("PID_Node_Update: No nodes to update - link is empty");
        return PID_NO_ENOUGH_NODES;
    }
    PID_Node* current = L->head;
    current->measured_value = head_measured_value; // 将头节点的测量值设置为输入的测量值
    while (current)
    {
        PID_ExecuteNode(current, dt);
        current = current->next;
    }
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的比例系数Kp。
 * @param node PID节点指针
 * @param kp   比例系数
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetKp(PID_Node* node , float kp)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetKp: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->parameters.kp = kp;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的积分系数Ki。
 * @param node PID节点指针
 * @param Ki   积分系数
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetKi(PID_Node* node , float Ki)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetKi: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->parameters.ki = Ki;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的微分系数Kd。
 * @param node PID节点指针
 * @param Kd   微分系数
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetKd(PID_Node* node , float Kd)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetKd: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->parameters.kd = Kd;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的设定值。
 * @param node     PID节点指针
 * @param setpoint 设定值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetSetpoint(PID_Node* node , float setpoint)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetSetpoint: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->setpoint = setpoint;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的积分衰减系数。
 * @param node                    PID节点指针
 * @param integral_attenuation_Kp 积分衰减系数，范围0~1
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetIntegralAttenuationKp(PID_Node* node , float integral_attenuation_Kp)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetIntegralAttenuationKp: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (integral_attenuation_Kp > 1.0f || integral_attenuation_Kp < 0.0f)
    {
        LOG_WARN("PID_Node_SetIntegralAttenuationKp: Invalid parameter - integral_attenuation_Kp should be between 0 and 1");
        return PID_INVALID_PARAMETER;
    }
    node->parameters.integral_attenuation_Kp = integral_attenuation_Kp;
    return PID_SUCCESS;
}


/**
 * @brief 配置PID节点的前馈参数。
 * @param node                      PID节点指针
 * @param feedforward_type          前馈类型
 * @param feedforward_Kp            单一前馈系数
 * @param feedforward_values_callback 前馈表回调函数
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetFeedForward(PID_Node* node, FeedForwardType feedforward_type, float feedforward_Kp, float (*feedforward_values_callback)(float target))
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetFeedForward: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (feedforward_type == FEEDFORWARD_TYPE_SINGLE)
    {
        node->flag.feedforward_type = FEEDFORWARD_TYPE_SINGLE;
        node->parameters.feedforward.feedforward_Kp = feedforward_Kp;
        node->parameters.feedforward.feedforward_values= NULL;
        return PID_SUCCESS; 
    }
    else if (feedforward_type == FEEDFORWARD_TYPE_TABLE)
    {
        if (feedforward_values_callback)
        {
            node->flag.feedforward_type = FEEDFORWARD_TYPE_TABLE;
            node->parameters.feedforward.feedforward_Kp = 0.0f; // 表格前馈不使用单一前馈系数
            node->parameters.feedforward.feedforward_values = feedforward_values_callback;
            return PID_SUCCESS;
        }
        else
        {
            LOG_WARN("PID_Node_SetFeedForward: Invalid parameter - feedforward_values_callback should not be NULL for table feedforward");
            node->flag.feedforward_type = NO_FEEDFORWARD; // 如果前馈表无效,禁用前馈
            node->parameters.feedforward.feedforward_Kp = 0.0f;
            node->parameters.feedforward.feedforward_values = NULL;
            return PID_INVALID_PARAMETER;
        }
        
    }
    else if (feedforward_type == NO_FEEDFORWARD)
    {
        node->flag.feedforward_type = NO_FEEDFORWARD;
        node->parameters.feedforward.feedforward_Kp = 0.0f;
        node->parameters.feedforward.feedforward_values = NULL;
        return PID_SUCCESS;
    }
    else
    {
        LOG_WARN("PID_Node_SetFeedForward: Invalid parameter - unknown feedforward type");
        return PID_INVALID_PARAMETER;
    }
}



/**
 * @brief 设置PID节点的限制参数。
 * @param node  PID节点指针
 * @param limit 限制参数结构体
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetLimit(PID_Node* node,PID_Limit limit)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetLimit: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (limit.output_max < limit.output_min && fabsf(limit.output_max - limit.output_min) > 1e-6f)
    {
        LOG_WARN("PID_Node_SetLimit: Invalid parameter - output_max should be greater than output_min");
        return PID_INVALID_PARAMETER;
    }
    if (limit.integral_max < 0.0f)
    {
        LOG_WARN("PID_Node_SetLimit: Invalid parameter - integral_max should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    if (limit.derivative_max < 0.0f)
    {
        LOG_WARN("PID_Node_SetLimit: Invalid parameter - derivative_max should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    node->limit = limit;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的最大输出。
 * @param node       PID节点指针
 * @param max_output 最大输出值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetMaxOutput(PID_Node* node, float max_output)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetMaxOutput: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (max_output < node->limit.output_min)
    {
        LOG_WARN("PID_Node_SetMaxOutput: Invalid parameter - max_output should be greater than or equal to current output_min");
        return PID_INVALID_PARAMETER;
    }
    node->limit.output_max = max_output;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的最小输出。
 * @param node       PID节点指针
 * @param min_output 最小输出值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetMinOutput(PID_Node* node, float min_output)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetMinOutput: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (min_output > node->limit.output_max)
    {
        LOG_WARN("PID_Node_SetMinOutput: Invalid parameter - min_output should be less than or equal to current output_max");
        return PID_INVALID_PARAMETER;
    }
    node->limit.output_min = min_output;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的最大积分。
 * @param node         PID节点指针
 * @param max_integral 最大积分值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetMaxIntegral(PID_Node* node, float max_integral)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetMaxIntegral: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (max_integral < 0.0f)
    {
        LOG_WARN("PID_Node_SetMaxIntegral: Invalid parameter - max_integral should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    node->limit.integral_max = max_integral;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的最大微分。
 * @param node           PID节点指针
 * @param max_derivative 最大微分值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetMaxDerivative(PID_Node* node, float max_derivative)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetMaxDerivative: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (max_derivative < 0.0f)
    {
        LOG_WARN("PID_Node_SetMaxDerivative: Invalid parameter - max_derivative should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    node->limit.derivative_max = max_derivative;
    return PID_SUCCESS;
}


/**
 * @brief 设置PID节点的死区。
 * @param node     PID节点指针
 * @param deadband 死区范围
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_SetDeadband(PID_Node* node, float deadband)
{
    if (!node)
    {
        LOG_WARN("PID_Node_SetDeadband: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (deadband < 0.0f)
    {
        LOG_WARN("PID_Node_SetDeadband: Invalid parameter - deadband should be non-negative");
        return PID_INVALID_PARAMETER;
    }
    node->limit.deadband = deadband;
    return PID_SUCCESS;
}


/**
 * @brief 获取链表尾节点的输出值。
 * @param link       PID链表指针
 * @param tail_value 输出参数，保存尾节点的输出值
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Link_Output(PID_Link* link,float* tail_value)
{
    if (!link)
    {
        LOG_WARN("PID_Link_Output: Invalid parameter - link is NULL");
        return PID_INVALID_PARAMETER;
    }
    if (!tail_value)
    {
        LOG_WARN("PID_Link_Output: Invalid parameter - tail_value is NULL");
        return PID_INVALID_PARAMETER;
    }
    if(!link->head || !link->tail || link->size == 0)
    {
        LOG_WARN("PID_Link_Output: No nodes to reset - link is empty");
        return PID_NO_ENOUGH_NODES;
    }
    *tail_value = link->tail->output; // 将尾节点的测量值保存到输出参数
    return PID_SUCCESS;
}


/**
 * @brief 重置PID节点的积分项。
 * @param node PID节点指针
 * @return PID_RETURN_CORE
 */
PID_RETURN_CORE PID_Node_ResetIntegral(PID_Node* node)
{
    if (!node)
    {
        LOG_WARN("PID_Node_ResetIntegral: Invalid parameter - node is NULL");
        return PID_INVALID_PARAMETER;
    }
    node->data.integral_sum = 0.0f;
    return PID_SUCCESS;
}