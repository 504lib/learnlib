#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>

// 任务类型（1~4）
typedef enum {
    TASK_NONE = 0,
    TASK_1,
    TASK_2,
    TASK_3,
    TASK_4
} TaskType_t;

// 动作状态（与 control 交互）
typedef enum {
    ACT_IDLE = 0,
    ACT_STRAIGHT,      // 直行（使用角度 PID）
    ACT_TRACKING,      // 循迹（使用灰度 PID）
    ACT_STOP
} ActionState_t;

// 状态机控制块
typedef struct {
    TaskType_t current_task;
    ActionState_t current_action;
    uint8_t lap_count;          //仅在 TASK_4 中记录已完成的圈数
    uint8_t stage_index;		//在 task_sequence 数组中的索引，指示当前执行到第几个动作。
    bool is_task_complete;		//任务是否完成（停止后置 true）
    bool need_sound_light;		//声光指示标志
    float target_angle;			//进入直行模式时锁定的航向角
    bool flag_just_entered;		//刚进入新动作时置 true，用于执行一次初始化操作
} SM_Context_t;

// 全局状态机实例（外部可见）
extern SM_Context_t sm;

// 公共接口
void SM_Init(void);
void SM_Update(void);
void SM_StartTask(TaskType_t task);
bool SM_IsRunning(void);
void SM_Reset(void);
float SM_GetTargetAngle(void);
ActionState_t SM_GetCurrentAction(void);

#endif
