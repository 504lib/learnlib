#include "HSM_Core.h"
#include "./static_stack.h"
#include "./static_queue.h"
#include <cassert>
#include <array>
#include <new>



struct TransitQueue_package
{
    HSM_Node* target;
    HSM_Event_Package event;
};

DECLARE_STATIC_STACK(DispatchStack,HSM_Node*,HSM_MAX_STACK_DEPTH)
DECLARE_STATIC_QUEUE(TransitQueue,TransitQueue_package,HSM_MAX_STACK_DEPTH)


class HSM_Core
{
private:
    bool            is_enabled;                             // 状态机是否启用
    bool            isinitialized;                          // 状态机是否已初始化
    DispatchStack_t dispatchStack_active_state;             // 当前活动状态的栈路径
    DispatchStack_t dispatchStack_next_state;               // 下一状态的栈路径,在状态转换过程中,用于暂存下一状态的栈路径,用于寻找LCA
    TransitQueue_t  requestTransitionQueue;                 // 状态转换请求队列,在状态转换过程中,如果有新的状态转换请求,则将请求加入队列,待当前状态转换完成后,再处理请求,用于避免状态转换过程中出现重复转换和状态不稳定的情况
    HSM_Node*       current_active_state;                   // 当前活动状态
    HSM_Node*       pending_active_state;                   // 待激活状态,在状态转换过程中,用于暂存待激活状态,用于寻找LCA,NULL表示没有待激活状态,作为互斥标志使用
    bool            isPassDefenseCheck();                   // 防御性编程检查函数,检查状态机是否已初始化,是否已启用,是否有当前活动状态等,如果检查失败,则返回false,并打印警告日志
    bool            isPassDefenseCheck(HSM_Node* node);     
    void            clearHSM_Node_Table(HSM_Node_Param node[],std::size_t table_len); //
    bool            Transition(HSM_Node* target, HSM_Event_Package event);          // 状态转换函数,将当前活动状态切换到目标状态,执行exit/entry链
    HSM_Node*       Dispatch(HSM_Event_Package event);      // 事件分发函数,将事件分发给当前活动状态,并处理状态转换
    HSM_Node*       LCA(HSM_Node* node1, HSM_Node* node2);              // 最近公共祖先函数,用于寻找两个节点的最近公共祖先,在状态转换过程中用于寻找当前活动状态和待激活状态的最近公共祖先
    HSM_Node*       InitHSM_Node(HSM_Node_Param node_param);              // 初始化状态节点函数,用于初始化状态节点,将节点的函数指针和节点关系指针设置为传入的参数,如果初始化成功,则返回节点指针,否则返回NULL
    void            PushAncestors(DispatchStack_t* stack, HSM_Node* node, HSM_Node* ancestor);  // 将 node 到 ancestor(不含) 的祖先路径压栈
public:
    HSM_Core();
    ~HSM_Core();
    void            Start(HSM_Node* initial_state);              // 启动状态机,将初始状态设置为当前活动状态,并执行初始状态的entry_action
    void            Start(HSM_Node* initial_state, HSM_Event_Package event);              // 启动状态机,将初始状态设置为当前活动状态,并执行初始状态的entry_action,同时处理一个事件,用于启动时需要处理一个事件的场景
    void            Process();                                   // 持续行为函数,用于处理持续状态的持续行为,需要在主循环中持续调用
    void            isEnable(bool is_enable);                                      // 启用状态机,使能状态机后,状态机才能处理事件
    void            SendEvent(HSM_Event_Package event);                             // 发送事件函数,将事件发送给状态机,状态机会将事件分发给当前活动状态,并处理状态转换
    bool            RequestTransition(HSM_Node* target, HSM_Event_Package event);          // 状态转换函数,将当前活动状态切换到目标状态,执行exit/entry链
    bool            RegisterChild(HSM_Node* parent, HSM_Node_Param child_param);              // 注册子节点函数,将子节点注册到父节点下,建立父子关系,如果注册成功,则返回true,否则返回false
    bool            RegisterChildNodes(HSM_Node* parent, HSM_Node_Param child_params[], std::size_t child_count);              // 批量注册子节点函数,将多个子节点注册到父节点下,建立父子关系,如果注册成功,则返回true,否则返回false
};

HSM_Core::HSM_Core()
{
    DispatchStack_INIT(&dispatchStack_active_state);
    DispatchStack_INIT(&dispatchStack_next_state);
    TransitQueue_INIT(&requestTransitionQueue);
    current_active_state = NULL;
    pending_active_state = NULL;
    is_enabled = false;
    isinitialized = true;
}

HSM_Core::~HSM_Core()
{
    DispatchStack_CLEAR(&dispatchStack_active_state);
    DispatchStack_CLEAR(&dispatchStack_next_state);
    is_enabled = false;
    isinitialized = false;
}

bool HSM_Core::isPassDefenseCheck()
{
    if (!isinitialized)
    {
        LOG_WARN("HSM_Core is not initialized!");
        return false;
    }
    return true;
}


HSM_Node* HSM_Core::Dispatch(HSM_Event_Package event)
{
    if (!isPassDefenseCheck())    return nullptr;
    HSM_Node* current_node = current_active_state;
    while (current_node)
    {
        if (current_node->handler && current_node->handler(event))           // 有节点接这个事件
        {
            break;
        }
        current_node = current_node->parent;                                       // 没有节点接这个事件,继续冒泡        
    }
    return current_node;                                                     // 返回处理事件的节点,如果没有节点处理这个事件,则返回NULL
}

HSM_Node*  HSM_Core::LCA(HSM_Node* node1, HSM_Node* node2)
{
    DispatchStack_CLEAR(&dispatchStack_active_state);
    DispatchStack_CLEAR(&dispatchStack_next_state);

    if (node1 == NULL || node2 == NULL)
    {
        LOG_WARN("LCA function received NULL node! node1: %p, node2: %p", node1, node2);
        return nullptr;
    }
    PushAncestors(&dispatchStack_active_state, node1, nullptr);
    PushAncestors(&dispatchStack_next_state, node2, nullptr);
    HSM_Node* lca = nullptr;
    HSM_Node *node1_ptr, *node2_ptr;
    while (DispatchStack_POP(&dispatchStack_active_state,&node1_ptr) && DispatchStack_POP(&dispatchStack_next_state,&node2_ptr))
    {
        if (node1_ptr == node2_ptr)
        {
            lca = node1_ptr;
        }
        else
        {
            break;
        }
    }
    DispatchStack_CLEAR(&dispatchStack_active_state);
    DispatchStack_CLEAR(&dispatchStack_next_state);
    return lca;
}


void HSM_Core::Start(HSM_Node* initial_state)
{
    DispatchStack_CLEAR(&dispatchStack_active_state);
    if (!isinitialized)
    {
        LOG_WARN("HSM_Core::Start called but HSM is not initialized!");
        return;
    }
    if (initial_state == NULL)
    {
        LOG_WARN("HSM_Core::Start received NULL initial state!");
        return;
    }
    if (!initial_state->is_initialized)
    {
        LOG_WARN("HSM_Core::Start received uninitialized node!");
        return;
    }
    isEnable(true);
    PushAncestors(&dispatchStack_active_state, initial_state, nullptr);
    HSM_Node* node;
    while (DispatchStack_POP(&dispatchStack_active_state, &node))
    {
        if (node->parent)
        {
            node->parent->active_child = node;
        }
        if (node->entry_action)
        {
            node->entry_action((HSM_Event_Package){0});
        }
    }
    current_active_state = initial_state;
    DispatchStack_CLEAR(&dispatchStack_active_state);
}

void HSM_Core::Start(HSM_Node* initial_state, HSM_Event_Package event)
{
    if (!isinitialized)
    {
        LOG_WARN("HSM_Core::Start called but HSM is not initialized!");
        return;
    }
    if (initial_state == NULL)
    {
        LOG_WARN("HSM_Core::Start received NULL initial state!");
        return;
    }
    if (!initial_state->is_initialized)
    {
        LOG_WARN("HSM_Core::Start received uninitialized node!");
        return;
    }
    isEnable(true);
    PushAncestors(&dispatchStack_active_state, initial_state, nullptr);
    HSM_Node* node;
    while (DispatchStack_POP(&dispatchStack_active_state, &node))
    {
        if (node->parent)
        {
            node->parent->active_child = node;
        }
        if (node->entry_action)
        {
            node->entry_action(event);
        }
    }
    current_active_state = initial_state;
    DispatchStack_CLEAR(&dispatchStack_active_state);
}

void HSM_Core::isEnable(bool is_enable)
{
    is_enabled = is_enable;
}


void HSM_Core::Process()
{
    if (!isPassDefenseCheck())
    {
        return;
    }
    if(!is_enabled)
    {
        LOG_WARN("HSM_Core::Process called but HSM is not enabled! Call HSM_SetEnable() first.");
        return;
    }
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::Process called but no active state! Call Start() first.");
        return;
    }
    TransitQueue_package package;
    while (TransitQueue_POP(&requestTransitionQueue, &package))
    {
        Transition(package.target, package.event);
    }
    if (current_active_state->continuous_action)
    {
        current_active_state->continuous_action();
    }
}



bool HSM_Core::isPassDefenseCheck(HSM_Node* node)
{
    if (!node)
    {
        LOG_WARN("HSM_Core received NULL node! Please provide a valid node.");
        return false;
    }
    if (!node->is_initialized)
    {
        LOG_WARN("HSM_Core received uninitialized node! Please initialize the node before using it. Node pointer: %p", node);
        return false;
    }
    return isPassDefenseCheck();
}

void HSM_Core::PushAncestors(DispatchStack_t* stack, HSM_Node* node, HSM_Node* ancestor)
{
    while (node && node != ancestor)
    {
        DispatchStack_PUSH(stack, node);
        node = node->parent;
    }
}

HSM_Node* HSM_Core::InitHSM_Node(HSM_Node_Param node_param)
{
    if (!node_param.node)
    {
        LOG_WARN("HSM_Core::InitHSM_Node received NULL node pointer!");
        return nullptr;
    }

    HSM_Node* node = node_param.node;
    if (node->is_initialized)
    {
        LOG_WARN("HSM_Core::InitHSM_Node node at %p already initialized, skipping!", (void*)node);
        return node;
    }
    node->is_initialized = true;
    node->handler = node_param.handler;
    node->entry_action = node_param.entry_action;
    node->exit_action = node_param.exit_action;
    node->continuous_action = node_param.continuous_action;
    node->parent = nullptr;
    node->first_child = nullptr;
    node->next_sibling = nullptr;
    node->active_child = nullptr;
    return node;
}             


void HSM_Core::clearHSM_Node_Table(HSM_Node_Param node[],std::size_t table_len)
{
    for (std::size_t i = 0; i < table_len; i++) // 清理流程
    {
        if (!node[i].node)// 不存在,往下都清除
        {
            continue;
        }
        node[i].node->is_initialized = false;
        node[i].node->parent = nullptr;
        node[i].node->first_child = nullptr;
        node[i].node->next_sibling = nullptr;
        node[i].node->active_child = nullptr;
    }
}




void HSM_Core::SendEvent(HSM_Event_Package event)
{
    if (!isPassDefenseCheck()) return;
    if (!is_enabled)
    {
        LOG_WARN("HSM_Core::SendEvent called but HSM is not enabled! Call HSM_SetEnable() first.");
        return; 
    }
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::SendEvent called but no active state! Call Start() first.");
        return;
    }
    Dispatch(event);
    TransitQueue_package package;
    while (TransitQueue_POP(&requestTransitionQueue, &package))
    {
        Transition(package.target, package.event);
    }
}

bool HSM_Core::RequestTransition(HSM_Node* target, HSM_Event_Package event)
{
    if (!isPassDefenseCheck()) return false;
    if (!is_enabled)
    {
        LOG_WARN("HSM_Core::RequestTransition called but HSM is not enabled! Call HSM_SetEnable() first!");
        return false;
    }
    if (!target)
    {
        LOG_WARN("HSM_Core::RequestTransition received NULL target!");
        return false;
    }
    if (!target->is_initialized)
    {
        LOG_WARN("HSM_Core::RequestTransition received uninitialized target node!");
        return false;
    }
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::RequestTransition called but no active state! Call Start() first.");
        return false;
    }
    TransitQueue_package transition_request = {target, event};
    if (!TransitQueue_PUSH(&requestTransitionQueue, transition_request))
    {
        LOG_WARN("HSM_Core::RequestTransition failed to push transition request to queue! Queue might be full.");
        return false;
    }
    return true;
}

bool HSM_Core::Transition(HSM_Node* target, HSM_Event_Package event)
{
    if (!isPassDefenseCheck()) return false;
    if (!is_enabled)
    {
        LOG_WARN("HSM_Core::Transition called but HSM is not enabled! Call HSM_SetEnable() first!");
        return false;
    }
    if (!target)
    {
        LOG_WARN("HSM_Core::Transition received NULL target!");
        return false;
    }
    if (!target->is_initialized)
    {
        LOG_WARN("HSM_Core::Transition received uninitialized target node!");
        return false;
    }
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::Transition called but no active state! Call Start() first.");
        return false;
    }
    if (target == current_active_state) return true;

    HSM_Node* lca = LCA(current_active_state, target);
    if (!lca)
    {
        LOG_WARN("HSM_Core::Transition failed to find LCA between current and target!");
        return false;
    }
    if (target->parent)
    {
        target->parent->active_child = target; // 更新父节点的活跃子节点指针
    }
    
    HSM_Node* node = current_active_state;
    while (node != lca)
    {
        if (node->exit_action) node->exit_action(event);
        node = node->parent;
    }

    PushAncestors(&dispatchStack_active_state, target, lca);

    HSM_Node* entry_node;
    while (DispatchStack_POP(&dispatchStack_active_state, &entry_node))
    {
        LOG_DEBUG("HSM_Core::Transition entering node at %p", (void*)entry_node);
        if (entry_node->entry_action) entry_node->entry_action(event);
    }

    current_active_state = target;
    return true;
}

bool HSM_Core::RegisterChild(HSM_Node* parent, HSM_Node_Param child_param)
{
    if (!isPassDefenseCheck()) return false;
    if (is_enabled)
    {
        LOG_WARN("HSM_Core::RegisterChild called while HSM is enabled!");
        return false;
    }
    if (!parent)
    {
        LOG_WARN("HSM_Core::RegisterChild received NULL parent node!");
        return false;
    }
    if (!parent->is_initialized)
    {
        parent->is_initialized = true;
    }
    HSM_Node* child = InitHSM_Node(child_param);
    if (!child)
    {
        LOG_WARN("HSM_Core::RegisterChild failed to initialize child node!");
        return false;
    }
    child->parent = parent;
    if (!parent->first_child)
    {
        parent->first_child = child;
        parent->active_child = child; // 如果父节点没有子节点,则将新注册的子节点设置为活跃子节点
    }
    else
    {
        HSM_Node* sibling = parent->first_child;
        while (sibling->next_sibling)
        {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }
    return true;
}

bool HSM_Core::RegisterChildNodes(HSM_Node* parent, HSM_Node_Param child_params[], std::size_t child_count)
{
    if (!isPassDefenseCheck()) return false;
    if (is_enabled)
    {
        LOG_WARN("HSM_Core::RegisterChildNodes called while HSM is enabled! Please disable HSM before registering child nodes.");
        return false;
    }
    if (!parent)
    {
        LOG_WARN("HSM_Core::RegisterChildNodes received NULL parent node!");
        return false;
    }
    for (std::size_t i = 0; i < child_count; i++)
    {
        if (!RegisterChild(parent, child_params[i]))
        {
            LOG_WARN("HSM_Core::RegisterChildNodes failed to register child at index %zu!", i);
            clearHSM_Node_Table(child_params, i); // 清理已注册的子节点
            return false;
        }
    }
    return true;
}

extern "C"
{
    HSM* HSM_Create(FMS_OBJECT_MEMORY* memory)
    {
        static_assert(sizeof(HSM_Core) <= HSM_OBJECT_STORAGE_SIZE, "HSM_Core size exceeds HSM_OBJECT_STORAGE_SIZE! Please increase HSM_OBJECT_STORAGE_SIZE or reduce HSM_Core size.");
        if (!memory)
        {
            LOG_WARN("HSM_Create received NULL memory pointer!");
            return nullptr;
        }
        HSM_Core* hsm = new (memory->storage) HSM_Core();
        return reinterpret_cast<HSM*>(hsm);
    }
    HSM_Node* HSM_Node_Create(FMS_NODE_MEMORY* memory)
    {
        static_assert(sizeof(HSM_Node) <= HSM_NODE_STORAGE_SIZE, "HSM_Node size exceeds HSM_NODE_STORAGE_SIZE! Please increase HSM_NODE_STORAGE_SIZE or reduce HSM_Node size.");
        if (!memory)
        {
            LOG_WARN("HSM_Node_Create received NULL memory pointer!");
            return nullptr;
        }
        HSM_Node* node = new (memory->storage) HSM_Node();
        return node;
    }
    void HSM_Destroy(HSM* hsm)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_Destroy received NULL pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->~HSM_Core();
    }
    void HSM_Start(HSM* hsm, HSM_Node* initial_state)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_Start received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Start(initial_state);
    }
    void HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_StartWithEvent received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Start(initial_state, event);
    }
    void HSM_Process(HSM* hsm)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_Process received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Process();
    }
    void HSM_SetEnable(HSM* hsm, bool is_enable)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_SetEnable received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->isEnable(is_enable);
    }
    void HSM_SendEvent(HSM* hsm, HSM_Event_Package event)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_SendEvent received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->SendEvent(event);
    }
    bool HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_RequestTransition received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RequestTransition(target, event);
    }
    bool HSM_RegisterChild(HSM* hsm, HSM_Node* parent, HSM_Node_Param child_param)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_RegisterChild received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RegisterChild(parent, child_param);
    }
    bool HSM_RegisterChildNodes(HSM* hsm, HSM_Node* parent, HSM_Node_Param child_params[], std::size_t child_count)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_RegisterChildNodes received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RegisterChildNodes(parent, child_params, child_count);
    }
}

