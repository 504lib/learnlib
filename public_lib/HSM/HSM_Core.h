#ifndef __HSM_CORE_H__
#define __HSM_CORE_H__ 


// #ifndef __cplusplus
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#ifndef HSM_EVENT_NULL  
#define HSM_EVENT_NULL 0xFF
#endif // !HSM_EVENT_NULL

#ifndef HSM_EVENT_PAYLOAD_SIZE
#define HSM_EVENT_PAYLOAD_SIZE 16
#endif // !HSM_EVENT_PAYLOAD_SIZE


// 单个 HSM 实例的最大状态节点数
#ifndef HSM_MAX_NODES
#define HSM_MAX_NODES 16
#endif // !HSM_MAX_NODES

#ifndef HSM_MAX_STACK_DEPTH
#define HSM_MAX_STACK_DEPTH 16
#endif // !HSM_MAX_STACK_DEPTH

#ifndef HSM_CORE_STORAGE_SIZE
#define HSM_CORE_STORAGE_SIZE 1024
#endif // !HSM_CORE_STORAGE_SIZE



typedef struct HSM_Event_Package
{
    uint8_t HSM_Event_ID; // 事件ID
    uint8_t data[HSM_EVENT_PAYLOAD_SIZE]; // 事件数据载荷
} HSM_Event_Package;

// ---- 小端序字节解析 (static inline, 无依赖) ----

static inline uint8_t  HSM_ReadU8 (const uint8_t* buf) { return buf[0]; }
static inline int8_t   HSM_ReadI8 (const uint8_t* buf) { return (int8_t)buf[0]; }

static inline uint16_t HSM_ReadU16LE(const uint8_t* buf) {
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}
static inline int16_t  HSM_ReadI16LE(const uint8_t* buf) {
    return (int16_t)HSM_ReadU16LE(buf);
}
static inline uint32_t HSM_ReadU32LE(const uint8_t* buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}
static inline int32_t  HSM_ReadI32LE(const uint8_t* buf) {
    return (int32_t)HSM_ReadU32LE(buf);
}
static inline float    HSM_ReadF32LE(const uint8_t* buf) {
    union { uint32_t u; float f; } u;
    u.u = HSM_ReadU32LE(buf);
    return u.f;
}

static inline void HSM_WriteU8 (uint8_t* buf, uint8_t  v) { buf[0] = v; }
static inline void HSM_WriteI8 (uint8_t* buf, int8_t   v) { buf[0] = (uint8_t)v; }

static inline void HSM_WriteU16LE(uint8_t* buf, uint16_t v) {
    buf[0] = (uint8_t)v;       buf[1] = (uint8_t)(v >> 8);
}
static inline void HSM_WriteI16LE(uint8_t* buf, int16_t v) {
    HSM_WriteU16LE(buf, (uint16_t)v);
}
static inline void HSM_WriteU32LE(uint8_t* buf, uint32_t v) {
    buf[0] = (uint8_t)v;        buf[1] = (uint8_t)(v >> 8);
    buf[2] = (uint8_t)(v >> 16); buf[3] = (uint8_t)(v >> 24);
}
static inline void HSM_WriteI32LE(uint8_t* buf, int32_t v) {
    HSM_WriteU32LE(buf, (uint32_t)v);
}
static inline void HSM_WriteF32LE(uint8_t* buf, float v) {
    union { float f; uint32_t u; } u;
    u.f = v;
    HSM_WriteU32LE(buf, u.u);
}

typedef struct HSM_StateDef
{
    const char* name;
    const char* parent_name;
    bool (*handler)(HSM_Event_Package event);
    void (*entry_action)(HSM_Event_Package event);
    void (*exit_action)(HSM_Event_Package event);
    void (*continuous_action)(void);
} HSM_StateDef;

#define HSM_STATE_DEF(name, parent_name, handler, entry_action, exit_action, continuous_action) \
    (HSM_StateDef){ name, parent_name, handler, entry_action, exit_action, continuous_action }

typedef struct {
    max_align_t storage[HSM_CORE_STORAGE_SIZE / sizeof(max_align_t)];
} HSM_Core_Memory;

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


#ifdef __cplusplus
extern "C" {
#endif

HSM*      HSM_Create(HSM_Core_Memory* mem, HSM_Node nodes[], size_t max_nodes,
                       const HSM_StateDef states[], size_t count);
void      HSM_Destroy(HSM* hsm);
void      HSM_Start(HSM* hsm, HSM_Node* initial_state);
void      HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event);
void      HSM_Process(HSM* hsm);
void      HSM_SetEnable(HSM* hsm, bool is_enable);
void      HSM_SendEvent(HSM* hsm, HSM_Event_Package event);
bool      HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event);
HSM_Node* HSM_FindNode(HSM* hsm, const char* name);

#ifdef __cplusplus
}
#endif

#endif // !__HSM_CORE_H__