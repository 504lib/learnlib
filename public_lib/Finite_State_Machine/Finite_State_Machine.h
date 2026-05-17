/**
 * @file Finite_State_Machine.h
 * @author whyP762 (3046961251@qq.com)
 * @brief    有限状态机的C版本,内容是CPP实现的,但是接口是C的,此文件C和Cpp均可用,但推荐引用hpp版本的文件,因为hpp版本的文件是C++的,而且接口更友好,如果需要使用C的接口,请引用此文件,如果需要使用C++的接口,请引用hpp版本的文件,因为C++的接口更友好,如果需要使用C的接口,请引用此文件,如果需要使用C++的接口,请引用hpp版本的文件,因为C++的接口更友好,如果需要使用C的接口,请引用此文件,如果需要使用C++的接口,请引用hpp版本的文件,因为C++的接口更友好,如果需要使用C的接口,请引用此文件,如果需要使用C++的接口,请引用hpp版本的文件,因为C++的接口更友好
 * @version 0.1
 * @date 2026-05-17
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef __FINITE_STATE_MACHINE_H__
#define __FINITE_STATE_MACHINE_H__
#include <stdint.h>
#include <stddef.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif


#ifndef MAX_STATE_NUM
#define MAX_STATE_NUM 16
#endif // !MAX_STATE_NUM

#ifdef __cplusplus
extern "C" {
#endif // DEBUG

#define FSM_STORAGE_SIZE 1024

#if defined(__clang__) || defined(__GNUC__)
#define FSM_ALIGN(N) __attribute__((aligned(N)))
#elif defined(__CC_ARM)
#define FSM_ALIGN(N) __align(N)
#else
#define FSM_ALIGN(N)
#endif


typedef struct FSM_ALIGN(8)
{
    unsigned char storage[FSM_STORAGE_SIZE];
} FMS_MEMORY;

typedef struct FSM_Structure FSM_Structure;
typedef struct FSM_Node FSM_Node;

typedef struct FSM_Function
{
    void (*action)(void);                         // 状态转换时的动作函数指针
    void (*entry)(void);                          // 进入状态时的动作函数指针
    void (*exit)(void);                           // 退出状态时的动作函数指针
}FSM_Function;
typedef struct DataPackage
{
    uint8_t    target_ID;                      // 目标状态ID
    union
    {
        uint32_t    data_u32;                  // 32位无符号整数数据
        int32_t     data_i32;                  // 32位有符号整数数据
        float       data_f32;                  // 32位浮点数数据
        double      data_f64;                  // 64位浮点数数据
        uint8_t     data_u8[4];                // 4字节无符号整数数据
        int8_t      data_i8[4];                // 4字节有符号整数数据
        uint16_t    data_u16[2];               // 2字节无符号整数数据
        int16_t     data_i16[2];               // 2字节有符号整数数据
    } data;
    size_t     data_size;                      // 数据大小
}DataPackage;



FSM_Structure* FSM_Create(FMS_MEMORY* memory);
void FSM_Destroy(FSM_Structure* fsm);
bool FSM_Add_State(FSM_Structure* fsm, uint8_t state_id, FSM_Function function);
bool FSM_Push_Data_Package(FSM_Structure* fsm, DataPackage data_package);
bool FSM_Get_Data_Package(FSM_Structure* fsm, DataPackage* data_package);
bool FSM_Start(FSM_Structure* fsm, uint8_t first_state_id);
void FSM_Set_Enable(FSM_Structure* fsm, bool enable);
void FSM_Process(FSM_Structure* fsm);
bool FSM_State_Transition(FSM_Structure* fsm, uint8_t state_id);

#ifdef __cplusplus
}
#endif

#endif // !__FINITE_STATE_MACHINE_H__
