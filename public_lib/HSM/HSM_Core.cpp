#include "HSM_Core.h"
#include "./static_stack.h"
#include <cassert>
#include <array>

DECLARE_STATIC_STACK(DispatchStack,HSM_Node*,HSM_MAX_STACK_DEPTH)

class HSM_Core
{
private:
    bool            is_enabled;                             // 状态机是否启用
    bool            isinitialized;                          // 状态机是否已初始化
    DispatchStack_t dispatchStack_active_state;             // 当前活动状态的栈路径
    DispatchStack_t dispatchStack_next_state;               // 下一状态的栈路径,在状态转换过程中,用于暂存下一状态的栈路径,用于寻找LCA
    HSM_Node*       current_active_state;                   // 当前活动状态
    HSM_Node*       pending_active_state;                   // 待激活状态,在状态转换过程中,用于暂存待激活状态,用于寻找LCA,NULL表示没有待激活状态,作为互斥标志使用
    bool            isPassDefenseCheck();                   // 防御性编程检查函数,检查状态机是否已初始化,是否已启用,是否有当前活动状态等,如果检查失败,则返回false,并打印警告日志
    bool            isPassDefenseCheck(HSM_Node* node);     
    void            clearHSM_Node_Table(HSM_Node_Param node[],std::size_t table_len); //
    HSM_Node*       Dispatch(HSM_Event_Package event);      // 事件分发函数,将事件分发给当前活动状态,并处理状态转换
    HSM_Node*       LCA(HSM_Node* node1, HSM_Node* node2);              // 最近公共祖先函数,用于寻找两个节点的最近公共祖先,在状态转换过程中用于寻找当前活动状态和待激活状态的最近公共祖先
    HSM_Node*       InitHSM_Node(HSM_Node_Param node_param);              // 初始化状态节点函数,用于初始化状态节点,将节点的函数指针和节点关系指针设置为传入的参数,如果初始化成功,则返回节点指针,否则返回NULL
public:
    HSM_Core();
    ~HSM_Core();
    void            Start(HSM_Node* initial_state);              // 启动状态机,将初始状态设置为当前活动状态,并执行初始状态的entry_action
    void            Start(HSM_Node* initial_state, HSM_Event_Package event);              // 启动状态机,将初始状态设置为当前活动状态,并执行初始状态的entry_action,同时处理一个事件,用于启动时需要处理一个事件的场景
    void            Process();                                   // 持续行为函数,用于处理持续状态的持续行为,需要在主循环中持续调用
    void            isEnable(bool is_enable);                                      // 启用状态机,使能状态机后,状态机才能处理事件
    void            SendEvent(HSM_Event_Package event);                             // 发送事件函数,将事件发送给状态机,状态机会将事件分发给当前活动状态,并处理状态转换
    bool            Transition(HSM_Node* target, HSM_Event_Package event);          // 状态转换函数,将当前活动状态切换到目标状态,执行exit/entry链
    bool            RegisterNodeTableForchild(HSM_Node_Param node[],std::size_t table_len);    // 注册子节点(父子链),将节点表按顺序串为parent-child链
    bool            RegisterNodeTableForSibling(HSM_Node_Param node[],std::size_t table_len);  // 注册兄弟节点(兄弟链),将节点表按顺序串为next_sibling链
};

HSM_Core::HSM_Core()
{
    DispatchStack_INIT(&dispatchStack_active_state);
    DispatchStack_INIT(&dispatchStack_next_state);
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
    HSM_Node* node1_ptr = node1;
    HSM_Node* node2_ptr = node2;
    while (node1_ptr)
    {
        DispatchStack_PUSH(&dispatchStack_active_state, node1_ptr);
        node1_ptr = node1_ptr->parent;
    }
    while (node2_ptr)
    {
        DispatchStack_PUSH(&dispatchStack_next_state, node2_ptr);
        node2_ptr = node2_ptr->parent;
    }
    HSM_Node* lca = nullptr;
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
    current_active_state = initial_state;
    if (current_active_state->entry_action)
    {
        current_active_state->entry_action((HSM_Event_Package){0});
    }
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
    current_active_state = initial_state;
    if (current_active_state->entry_action)
    {
        current_active_state->entry_action(event);
    }
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
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::Process called but no active state! Call Start() first.");
        return;
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
        node[i].node->parent = nullptr;
        node[i].node->first_child = nullptr;
        node[i].node->next_sibling = nullptr;
        node[i].node->active_child = nullptr;
    }
}


bool HSM_Core::RegisterNodeTableForchild(HSM_Node_Param node[],const std::size_t table_len)
{
    if (!isPassDefenseCheck())
    {
        return false;
    }
    if (table_len == 0)
    {
        LOG_WARN("HSM_Core::RegisterNodeTableForchild received empty node table!");
        return false;
    }
    if (table_len > HSM_MAX_STACK_DEPTH)
    {
        LOG_WARN("HSM_Core::RegisterNodeTableForchild received node table with length %zu, which exceeds the maximum stack depth of %d!", table_len, HSM_MAX_STACK_DEPTH);
        return false;
    }
    HSM_Node* parent_node = InitHSM_Node(node[0]);
    if (!parent_node)
    {
        clearHSM_Node_Table(node,1);
        LOG_WARN("HSM_Core::RegisterNodeTableForchild failed to initialize the first node!");
        return false;
    }
    for (std::size_t i = 1; i < table_len; i++)
    {
        HSM_Node* current_node = InitHSM_Node(node[i]);
        if (!current_node)
        {
            clearHSM_Node_Table(node,i);
            LOG_WARN("HSM_Core::RegisterNodeTableForchild failed to initialize node at index %zu!", i);
            return false;
        }
        current_node->parent = parent_node;
        if (parent_node->first_child && parent_node->first_child != current_node)
        {
            LOG_WARN("HSM_Core::RegisterNodeTableForchild overwriting existing first_child of node at %p!", (void*)parent_node);
        }
        parent_node->first_child = current_node;
        parent_node = current_node;
    }
    return true;
}


bool HSM_Core::RegisterNodeTableForSibling(HSM_Node_Param node[],const std::size_t table_len)
{
    if (!isPassDefenseCheck()) return false;
    if (table_len == 0)
    {
        LOG_WARN("HSM_Core::RegisterNodeTableForSibling received empty node table!");
        return false;
    }
    if (table_len > HSM_MAX_STACK_DEPTH)
    {
        LOG_WARN("HSM_Core::RegisterNodeTableForSibling received node table with length %zu, which exceeds the maximum stack depth of %d!", table_len, HSM_MAX_STACK_DEPTH);
        return false;
    }
    HSM_Node* prev_sibling = InitHSM_Node(node[0]);
    if (!prev_sibling)
    {
        clearHSM_Node_Table(node, 1);
        LOG_WARN("HSM_Core::RegisterNodeTableForSibling failed to initialize the first node!");
        return false;
    }
    for (std::size_t i = 1; i < table_len; i++)
    {
        HSM_Node* current_node = InitHSM_Node(node[i]);
        if (!current_node)
        {
            clearHSM_Node_Table(node, i);
            LOG_WARN("HSM_Core::RegisterNodeTableForSibling failed to initialize node at index %zu!", i);
            return false;
        }
        if (prev_sibling->next_sibling && prev_sibling->next_sibling != current_node)
        {
            LOG_WARN("HSM_Core::RegisterNodeTableForSibling overwriting existing next_sibling of node at %p!", (void*)prev_sibling);
        }
        prev_sibling->next_sibling = current_node;
        prev_sibling = current_node;
    }
    return true;
}


void HSM_Core::SendEvent(HSM_Event_Package event)
{
    if (!isPassDefenseCheck()) return;
    if (!current_active_state)
    {
        LOG_WARN("HSM_Core::SendEvent called but no active state! Call Start() first.");
        return;
    }
    Dispatch(event);
}

bool HSM_Core::Transition(HSM_Node* target, HSM_Event_Package event)
{
    if (!isPassDefenseCheck()) return false;
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

    HSM_Node* node = current_active_state;
    while (node != lca)
    {
        if (node->exit_action) node->exit_action(event);
        node = node->parent;
    }

    HSM_Node* n = target;
    while (n != lca)
    {
        DispatchStack_PUSH(&dispatchStack_active_state, n);
        n = n->parent;
    }

    HSM_Node* entry_node;
    while (DispatchStack_POP(&dispatchStack_active_state, &entry_node))
    {
        if (entry_node->entry_action) entry_node->entry_action(event);
    }

    current_active_state = target;
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
    bool HSM_Transition(HSM* hsm, HSM_Node* target, HSM_Event_Package event)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_Transition received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->Transition(target, event);
    }
    bool HSM_RegisterChildNodes(HSM* hsm, HSM_Node_Param node[], std::size_t table_len)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_RegisterChildNodes received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RegisterNodeTableForchild(node, table_len);
    }
    bool HSM_RegisterSiblingNodes(HSM* hsm, HSM_Node_Param node[], std::size_t table_len)
    {
        if (!hsm)
        {
            LOG_WARN("HSM_RegisterSiblingNodes received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RegisterNodeTableForSibling(node, table_len);
    }
}

