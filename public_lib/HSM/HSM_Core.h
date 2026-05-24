#ifndef __HSM_CORE_H__
#define __HSM_CORE_H__ 


// #ifndef __cplusplus
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
// #endif // !__cplusplus
#if defined(__clang__) || defined(__GNUC__)
#define HSM_ALIGN(N) __attribute__((aligned(N)))
#elif defined(__CC_ARM)
#define HSM_ALIGN(N) __align(N)
#else
#define HSM_ALIGN(N)
#endif

#ifndef HSM_EVENT_PAYLOAD_SIZE
#define HSM_EVENT_PAYLOAD_SIZE 16
#endif // !HSM_EVENT_PAYLOAD_SIZE

#ifndef HSM_OBJECT_STORAGE_SIZE
#define HSM_OBJECT_STORAGE_SIZE 1024
#endif // !HSM_OBJECT_STORAGE_SIZE

#ifndef HSM_NODE_STORAGE_SIZE
#define HSM_NODE_STORAGE_SIZE 80
#endif // !HSM_NODE_STORAGE_SIZE

#ifndef HSM_MAX_STACK_DEPTH
#define HSM_MAX_STACK_DEPTH 16
#endif // !HSM_MAX_STACK_DEPTH


typedef struct HSM_ALIGN(8)
{
    unsigned char storage[HSM_OBJECT_STORAGE_SIZE];
} FMS_OBJECT_MEMORY;

typedef struct HSM_ALIGN(8)
{
    unsigned char storage[HSM_NODE_STORAGE_SIZE];
} FMS_NODE_MEMORY;

typedef struct HSM_Event_Package
{
    uint8_t HSM_Event_ID; // 事件ID
    union
    {
        uint8_t data[HSM_EVENT_PAYLOAD_SIZE]; // 事件数据载荷
        uint32_t datau32;
        int32_t  datai32;
        uint16_t datau16;
        int16_t  datai16;
        uint8_t  datau8;
        int8_t   datai8;
        float    dataf32;
        double   dataf64;
    } HSM_DataPayload;
} HSM_Event_Package;


typedef struct HSM HSM;

typedef struct HSM_Node
{
    bool is_initialized;
    bool (*handler)(HSM_Event_Package event);
    void (*entry_action)(HSM_Event_Package event);
    void (*exit_action)(HSM_Event_Package event);
    void (*continuous_action)(void);
    struct HSM_Node* parent;
    struct HSM_Node* first_child;
    struct HSM_Node* next_sibling;
    struct HSM_Node* active_child;
} HSM_Node;

typedef struct HSM_Node_ParamForFunction
{
    HSM_Node* node;
    bool (*handler)(HSM_Event_Package event);   // 事件处理函数指针,必须填写,此回调作为冒泡的重要依据,如果不填写,则此节点将无法冒泡事件
    void (*entry_action)(HSM_Event_Package event);         // 进入状态时的动作函数指针,可选填写
    void (*exit_action)(HSM_Event_Package event);          // 退出状态时的动作函数指针,可选填写
    void (*continuous_action)();    // 持续状态时的动作函数指针,可选填写
}HSM_Node_Param;



#ifdef __cplusplus
extern "C" {
#endif

HSM*   HSM_Create(FMS_OBJECT_MEMORY* memory);
HSM_Node* HSM_Node_Create(FMS_NODE_MEMORY* memory);
void   HSM_Destroy(HSM* hsm);
void   HSM_Start(HSM* hsm, HSM_Node* initial_state);
void   HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event);
void   HSM_Process(HSM* hsm);
void   HSM_SetEnable(HSM* hsm, bool is_enable);
void   HSM_SendEvent(HSM* hsm, HSM_Event_Package event);
bool   HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event);
bool   HSM_RegisterChild(HSM* hsm, HSM_Node* parent, HSM_Node_Param child_param);
bool   HSM_RegisterChildNodes(HSM* hsm, HSM_Node* parent, HSM_Node_Param child_params[],size_t child_count);

#ifdef __cplusplus
}
#endif

#endif // !__HSM_CORE_H__