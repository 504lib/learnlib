#ifndef __FINITE_STATE_MACHINE_H__
#define __FINITE_STATE_MACHINE_H__

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// #define SYSTEM_TICK_GET_FUNCTION_FOR_MS() (void*)0
#define Packet_Buffer_Size 256
#define State_ID_t  uint32_t

typedef struct FSM_Data_Packet
{
    uint8_t         data[256];         // data buffer
}FSM_Data_Packet;

typedef enum {
    FSM_EVENT_IDLE = 0,
    FSM_EVENT_START = 1 << 0,
    FSM_EVENT_STOP = 1 << 1,
    FSM_EVENT_PAUSE = 1 << 2,
}FSM_EventType;

typedef enum FSM_ReturnCode {
    FSM_SUCCESS = 0,
    FSM_ERROR_INVALID_STATE = -1,
    FSM_ERROR_INVALID_EVENT = -2,
    FSM_ERROR_NULL_POINTER = -3,
    FSM_ERROR_NO_INITIAL_STATE = -4,
} FSM_ReturnCode;

typedef struct FSM_Event{
    FSM_EventType    last_event_id;     // bit mask for last event
    FSM_EventType    event_id;          // bit mask for event
}FSM_Event;

typedef struct FSM_State {
    void            (*on_enter)(void);
    void            (*on_exit)(void);
    size_t          enter_times;
    size_t          exit_times;
    uint32_t        state_id;            // unique identifier for the state
    uint32_t        last_enter_tick;     // timestamp of the last time the state was entered
    uint32_t        last_exit_tick;      // timestamp of the last time the state was exited
    uint32_t        total_time_in_state; // total time spent in this state
    uint32_t        last_state_id;       // unique identifier for the last state
} FSM_State;



typedef struct FSM_Node
{
    bool           isInitial_state;
    char           state_name[32];
    void            (*action)(void);
    FSM_State       state;
    FSM_Event       event;
    FSM_State       last_event;
    FSM_State*      next_states;
}FSM_Node;


typedef struct FSM
{
    bool            is_initialized;
    State_ID_t      current_state;
    FSM_Node*       first_node;
}FSM;

FSM_ReturnCode FSM_Init(FSM* fsm);
FSM_ReturnCode FMS_Node_Init(FSM_Node* node,
                         const char* state_name,
                         void (*action)(void));
FSM_ReturnCode FSM_Add_Node(FSM* fsm, FSM_Node* node,State_ID_t state_id);
FSM_ReturnCode FSM_Handle_Event(FSM* fsm, FSM_EventType event);
FSM_ReturnCode FMS_Update(FSM* fsm);
FSM_ReturnCode FSM_Transition(FSM* fsm, State_ID_t next_state_id);

#endif