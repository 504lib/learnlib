#include "HSM_Core.h"
#include "static_stack.h"
#include "static_queue.h"
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
    bool                is_enabled;
    bool                isinitialized;
    DispatchStack_t     dispatchStack_active_state;
    DispatchStack_t     dispatchStack_next_state;
    TransitQueue_t      requestTransitionQueue;
    HSM_Node*           current_active_state;
    HSM_Node*           pending_active_state;
    HSM_Node*           node_pool;                  // 用户提供的节点池
    size_t              node_pool_size;
    const HSM_StateDef* state_defs;
    size_t              state_count;
    const HSM_Transition* trans_table;
    size_t                trans_count;

    bool            isPassDefenseCheck();
    bool            isPassDefenseCheck(HSM_Node* node);
    bool            Transition(HSM_Node* target, HSM_Event_Package event);
    HSM_Node*       Dispatch(HSM_Event_Package event);
    HSM_Node*       LCA(HSM_Node* node1, HSM_Node* node2);
    void            PushAncestors(DispatchStack_t* stack, HSM_Node* node, HSM_Node* ancestor);
    void            LinkChild(HSM_Node* parent, HSM_Node* child);
    void            DumpTree(HSM_Node* node, int depth);
    const char*     NodeName(HSM_Node* node);
public:
    HSM_Core();
    ~HSM_Core();
    void            Start(HSM_Node* initial_state);
    void            Start(HSM_Node* initial_state, HSM_Event_Package event);
    void            Process();
    void            isEnable(bool is_enable);
    void            SendEvent(HSM_Event_Package event);
    bool            RequestTransition(HSM_Node* target, HSM_Event_Package event);
    HSM_Node*       BuildFromDefs(HSM_Node nodes[], size_t max_nodes, const HSM_StateDef states[], size_t count);
    HSM_Node*       FindNodeByName(const char* name);
    void            RegisterTransitions(const HSM_Transition trans[], size_t count);
};

HSM_Core::HSM_Core()
{
    DispatchStack_INIT(&dispatchStack_active_state);
    DispatchStack_INIT(&dispatchStack_next_state);
    TransitQueue_INIT(&requestTransitionQueue);
    current_active_state = NULL;
    pending_active_state = NULL;
    node_pool = NULL;
    node_pool_size = 0;
    state_defs = NULL;
    state_count = 0;
    trans_table = NULL;
    trans_count = 0;
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
            node->entry_action((HSM_Event_Package){.HSM_Event_ID = HSM_EVENT_NULL});
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

HSM_Node* HSM_Core::BuildFromDefs(HSM_Node nodes[], size_t max_nodes, const HSM_StateDef states[], size_t count)
{
    if (count > max_nodes) {
        LOG_WARN("HSM_Core::BuildFromDefs state count %zu exceeds node pool size %zu!", count, max_nodes);
        return nullptr;
    }
    // Pass 1: init each node from StateDef
    for (size_t i = 0; i < count; i++) {
        HSM_Node* node = &nodes[i];
        node->handler          = states[i].handler;
        node->entry_action     = states[i].entry_action;
        node->exit_action      = states[i].exit_action;
        node->continuous_action = states[i].continuous_action;
        node->parent           = nullptr;
        node->first_child      = nullptr;
        node->next_sibling     = nullptr;
        node->active_child     = nullptr;
        node->is_initialized   = true;
    }
    // store pointer for FindNodeByName before Pass 2
    node_pool      = nodes;
    node_pool_size = max_nodes;
    state_defs     = states;
    state_count    = count;

    // Pass 2: link parent-child by name
    for (size_t i = 0; i < count; i++) {
        if (!states[i].parent_name) continue;  // root
        HSM_Node* parent = FindNodeByName(states[i].parent_name);
        if (!parent) {
            LOG_WARN("HSM_Core::BuildFromDefs parent '%s' not found for '%s'!",
                     states[i].parent_name, states[i].name);
            return nullptr;
        }
        LinkChild(parent, &nodes[i]);
    }
    // return root (the first node with no parent)
    HSM_Node* root = &nodes[0];
    for (size_t i = 0; i < count; i++) {
        if (!states[i].parent_name) { root = &nodes[i]; break; }
    }
    LOG_INFO("[HSM] state tree (%zu nodes):", count);
    DumpTree(root, 0);
    return root;
}

void HSM_Core::LinkChild(HSM_Node* parent, HSM_Node* child)
{
    child->parent = parent;
    if (!parent->first_child) {
        parent->first_child  = child;
        parent->active_child = child;
    } else {
        HSM_Node* sibling = parent->first_child;
        while (sibling->next_sibling) sibling = sibling->next_sibling;
        sibling->next_sibling = child;
    }
}

const char* HSM_Core::NodeName(HSM_Node* node)
{
    if (!node || !node_pool) return "?";
    ptrdiff_t idx = node - node_pool;
    if (idx < 0 || (size_t)idx >= state_count) return "?";
    return state_defs[idx].name ? state_defs[idx].name : "?";
}

void HSM_Core::DumpTree(HSM_Node* root, int _depth)
{
    DispatchStack_t node_stack;
    int             depth_stack[HSM_MAX_STACK_DEPTH];
    int             top = 0;
    DispatchStack_INIT(&node_stack);

    DispatchStack_PUSH(&node_stack, root);
    depth_stack[top++] = 0;

    char line[96];
    while (top > 0) {
        top--;
        HSM_Node* node;
        DispatchStack_POP(&node_stack, &node);
        int depth = depth_stack[top];
        if (!node) continue;

        int pos = 0;
        for (int i = 0; i < depth && pos < 92; i++) {
            line[pos++] = ' ';
            line[pos++] = ' ';
        }
        const char* name = NodeName(node);
        while (*name && pos < 94) line[pos++] = *name++;
        line[pos] = '\0';
        LOG_INFO("  %s", line);

        // 逆序压子节点
        HSM_Node* siblings[HSM_MAX_STACK_DEPTH];
        int s_count = 0;
        HSM_Node* child = node->first_child;
        while (child && s_count < HSM_MAX_STACK_DEPTH) {
            siblings[s_count++] = child;
            child = child->next_sibling;
        }
        for (int i = s_count - 1; i >= 0; i--) {
            DispatchStack_PUSH(&node_stack, siblings[i]);
            depth_stack[top++] = depth + 1;
        }
    }
    DispatchStack_CLEAR(&node_stack);
}

void HSM_Core::RegisterTransitions(const HSM_Transition trans[], size_t count)
{
    trans_table = trans;
    trans_count = count;
}

HSM_Node* HSM_Core::FindNodeByName(const char* name)
{
    for (size_t i = 0; i < state_count; i++) {
        if (state_defs[i].name && name &&
            /* simple strcmp — no <string.h> dependency */
            state_defs[i].name[0] == name[0]) {
            const char* a = state_defs[i].name;
            const char* b = name;
            while (*a && *b && *a == *b) { a++; b++; }
            if (*a == '\0' && *b == '\0') return &node_pool[i];
        }
    }
    return nullptr;
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

    // 自动查 transition 表
    for (size_t i = 0; i < trans_count; i++) {
        const HSM_Transition* t = &trans_table[i];
        if (t->event != event.HSM_Event_ID) continue;
        if (t->from && t->from != current_active_state) continue;
        if (t->guard && !t->guard()) continue;
        RequestTransition(t->to, event);
        break;
    }

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
        LOG_DEBUG("HSM_Core::Transition entering node at \"%s\"", entry_node->name);
        if (entry_node->entry_action) entry_node->entry_action(event);
    }

    current_active_state = target;
    return true;
}

extern "C"
{
    HSM* HSM_Create(HSM_Core_Memory* mem, HSM_Node nodes[], size_t max_nodes,
                     const HSM_StateDef states[], size_t count)
    {
        static_assert(sizeof(HSM_Core) <= HSM_CORE_STORAGE_SIZE,
            "HSM_CORE_STORAGE_SIZE too small. Increase it or pass -DHSM_CORE_STORAGE_SIZE=XXX");
        if (!mem || !nodes || !states) {
            LOG_WARN("HSM_Create received NULL parameter!");
            return nullptr;
        }
        HSM_Core* hsm = new (mem->storage) HSM_Core();
        if (!hsm->BuildFromDefs(nodes, max_nodes, states, count)) {
            hsm->~HSM_Core();
            return nullptr;
        }
        return reinterpret_cast<HSM*>(hsm);
    }
    void HSM_Destroy(HSM* hsm)
    {
        if (!hsm) {
            LOG_WARN("HSM_Destroy received NULL pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->~HSM_Core();
    }
    void HSM_Start(HSM* hsm, HSM_Node* initial_state)
    {
        if (!hsm) {
            LOG_WARN("HSM_Start received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Start(initial_state);
    }
    void HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event)
    {
        if (!hsm) {
            LOG_WARN("HSM_StartWithEvent received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Start(initial_state, event);
    }
    void HSM_Process(HSM* hsm)
    {
        if (!hsm) {
            LOG_WARN("HSM_Process received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->Process();
    }
    void HSM_SetEnable(HSM* hsm, bool is_enable)
    {
        if (!hsm) {
            LOG_WARN("HSM_SetEnable received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->isEnable(is_enable);
    }
    void HSM_SendEvent(HSM* hsm, HSM_Event_Package event)
    {
        if (!hsm) {
            LOG_WARN("HSM_SendEvent received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->SendEvent(event);
    }
    bool HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event)
    {
        if (!hsm) {
            LOG_WARN("HSM_RequestTransition received NULL HSM pointer!");
            return false;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->RequestTransition(target, event);
    }
    HSM_Node* HSM_FindNode(HSM* hsm, const char* name)
    {
        if (!hsm) {
            LOG_WARN("HSM_FindNode received NULL HSM pointer!");
            return nullptr;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        return hsm_core->FindNodeByName(name);
    }
    void HSM_RegisterTransitions(HSM* hsm, const HSM_Transition trans[], size_t count)
    {
        if (!hsm) {
            LOG_WARN("HSM_RegisterTransitions received NULL HSM pointer!");
            return;
        }
        HSM_Core* hsm_core = reinterpret_cast<HSM_Core*>(hsm);
        hsm_core->RegisterTransitions(trans, count);
    }
}

