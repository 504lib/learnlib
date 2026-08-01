#include "Finite_State_Machine.h"


FSM_ReturnCode FSM_Init(FSM* fsm)
{
    if (!fsm)
    {
        return FSM_ERROR_NULL_POINTER;
    }
    fsm->current_state = (State_ID_t)0x00;
    fsm->first_node = NULL;
    fsm->is_initialized = true;
    return FSM_SUCCESS;
}

FSM_ReturnCode FMS_Node_Init(FSM_Node* node,const char* state_name,void (*action)(void))
{
    if (!node || !state_name || !action)
    {
        return FSM_ERROR_NULL_POINTER;
    }
    strncpy(node->state_name, state_name, sizeof(node->state_name) - 1);
    node->action = action;
    node->isInitial_state = true;
    return FSM_SUCCESS;
}



FSM_ReturnCode FSM_Add_Node(FSM* fsm, FSM_Node* node,State_ID_t state_id)
{
    if (!fsm || !node)
    {
        return FSM_ERROR_NULL_POINTER;
    }
    if (!node->isInitial_state || !fsm->is_initialized)
    {
        return FSM_ERROR_NO_INITIAL_STATE;
    }
    if (!fsm->first_node)
    {
        fsm->first_node = node;
    }
    FSM_Node* cur = fsm->first_node;
    while (cur->next_states)
    {
        if (state_id == cur->state.state_id)
        {
            return FSM_ERROR_INVALID_STATE;
        }
        cur = cur->next_states;
    }
    cur->next_states = node;
    node->state.state_id = state_id;
    return FSM_SUCCESS;
}


FSM_ReturnCode FSM_Handle_Event(FSM* fsm, FSM_EventType event)
{
    if (!fsm || !fsm->is_initialized)
    {
        return FSM_ERROR_NULL_POINTER;
    }
    if (!fsm->current_state == 0x00)
    {
        return FSM_ERROR_NO_INITIAL_STATE;
    }
    switch (event)
    {
    case FSM_EVENT_IDLE:
        
        break;
    case FSM_EVENT_START:
        break;
    case FSM_EVENT_STOP:
        break;
    case FSM_EVENT_PAUSE:
        break;
    default:
        break;
    }
    return FSM_ERROR_INVALID_EVENT;
}