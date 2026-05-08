#include "state_machine.h"
#include "control.h"
#include "grayscale.h"
#include "Motor_AT4950.h"
#include "mpu6050_user.h"
#include "Log.h"

extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

#define DISTANCE_TO_C 0.65f   // 从A点到C点的预估距离（单位：米），需要实际测量校准

// 全局状态机实例
SM_Context_t sm;

// ========== 新增：全局变量，用于触发校准 ==========
volatile bool trigger_calibration = false;

// 任务动作序列（根据电赛H题实际路径填写）
const int8_t task_sequence[5][8] = {
    {-1},                                   // TASK_NONE
    {ACT_STRAIGHT, ACT_STOP, -1},          // TASK_1: A->B
    {ACT_STRAIGHT, ACT_TRACKING, ACT_STRAIGHT, ACT_TRACKING, ACT_STOP, -1},  // TASK_2: A->B->C->D->A
    {ACT_STRAIGHT, ACT_TRACKING, ACT_STRAIGHT, ACT_TRACKING, ACT_STOP, -1},  // TASK_3: A->C->B->D->A
    {ACT_STRAIGHT, ACT_TRACKING, ACT_STRAIGHT, ACT_TRACKING, -1}             // TASK_4: 循环4圈
};

// 切换到下一个动作
static void SM_NextAction(void) {
    sm.stage_index++;
    int8_t next = task_sequence[sm.current_task][sm.stage_index];
    if (next == -1) {
        if (sm.current_task == TASK_4) {
            sm.lap_count++;
            if (sm.lap_count < 4) {
                sm.stage_index = 0;
                sm.current_action = (ActionState_t)task_sequence[TASK_3][0];
            } else {
                sm.current_action = ACT_STOP;
            }
        } else {
            sm.current_action = ACT_STOP;
        }
    } else {
        sm.current_action = (ActionState_t)next;
    }
    sm.flag_just_entered = true;
}

static float GetRelativeTurnAngle(TaskType_t task, uint8_t stage, uint8_t lap) {
    if (task == TASK_2) {
        switch (stage) {
            case 0: return 0.0f;
            case 2: return -6.0f;   // 根据实际标定
            default: return 0.0f;
        }
    } else if (task == TASK_3) {
        switch (stage) {
            case 0: return -38.0f;   // A->C
            case 2: return +45.5f;  // B->D
            default: return 0.0f;
        }
    } else if (task == TASK_4) {
        switch (stage) {
            case 0: return -45.5f;   // A->C
            case 2: return +47.3f;  // B->D
            default: return 0.0f;
        }
    }
    return 0.0f;
}
//static float GetRelativeTurnAngle(TaskType_t task, uint8_t stage, uint8_t lap) {
//    if (task == TASK_2) {
//        switch (stage) {
//            case 0: return 0.0f;
//            case 2: return -6.0f;   // 根据实际标定
//            default: return 0.0f;
//        }
//    } else if (task == TASK_3) {
//        switch (stage) {
//            case 0: return -38.5f;   // A->C
//            case 2: return +40.0f;  // B->D
//            default: return 0.0f;
//        }
//    } else if (task == TASK_4) {
//        float base;
//        switch (stage) {
//            case 0: base = -38.5f; break;
//            case 2: base = +40.5f; break;			
//            default: base = 0.0f; break;
//        }
//        
//        float correction = 0.0f;
//        if (!(lap == 0 && stage == 0)) {  // 除了第一圈第一段外，其余都要修正
//            // 根据 stage 决定修正方向
//            if (stage == 0) {
//                correction = -6.0f;   // A->C：向右偏6度（负值）
//            } else if (stage == 2) {
//                correction = +6.0f;   // B->D：向左偏6度（正值）
//            }
//        }
//        return base + correction;
//    }
//    return 0.0f;
//}

// ========== 新增：强制进入校准状态 ==========
void SM_ForceCalibration(void) {
    if (sm.current_action == ACT_STRAIGHT) {
        trigger_calibration = true;
    }
}

void SM_Init(void) {
    sm.current_task = TASK_NONE;
    sm.current_action = ACT_IDLE;
    sm.lap_count = 0;
    sm.stage_index = 0;
    sm.is_task_complete = false;
    sm.need_sound_light = false;
    sm.target_angle = 0.0f;
    sm.flag_just_entered = false;
	sm.original_yaw = 0.0f;
}

void SM_StartTask(TaskType_t task) {
    SM_Reset();
    sm.current_task = task;
    sm.current_action = ACT_STRAIGHT;
    sm.stage_index = 0;
    sm.is_task_complete = false;
    sm.flag_just_entered = true;
}

void SM_Reset(void) {
    sm.lap_count = 0;
    sm.stage_index = 0;
    sm.is_task_complete = false;
    sm.need_sound_light = false;
    sm.flag_just_entered = false;
}

void SM_Update(void) {
    if (sm.is_task_complete) return;
    
    extern MPU6050_Data_t* mpu;
    extern PID_Node pidGrayscale;
    extern PID_Node pidAngle;
    extern float total_distance_traveled; 
	
    switch (sm.current_action) {
        case ACT_STRAIGHT:
            if (sm.flag_just_entered) {
                float current_yaw = mpu->yaw;
                float relative_turn = 0.0f;

                if (sm.current_task == TASK_2) {
                    relative_turn = GetRelativeTurnAngle(sm.current_task, sm.stage_index, sm.lap_count);
                }
                else if (sm.current_task == TASK_3 || sm.current_task == TASK_4) {
                    relative_turn = GetRelativeTurnAngle(sm.current_task, sm.stage_index, sm.lap_count);
                }
                else if (sm.stage_index == 0) {
                    relative_turn = 0.0f;
                }
				sm.original_yaw = current_yaw;
                sm.target_angle = current_yaw + relative_turn;
                if (sm.target_angle > 180.0f) sm.target_angle -= 360.0f;
                if (sm.target_angle < -180.0f) sm.target_angle += 360.0f;

                LOG_DEBUG("STRAIGHT: stage=%d, cur_yaw=%.1f, rel_turn=%.1f, target=%.1f",
                          sm.stage_index, current_yaw, relative_turn, sm.target_angle);

                PID_Node_ResetIntegral(&pidAngle);
                sm.flag_just_entered = false;
                total_distance_traveled = 0.0f;
            }
            // 触发校准的条件：到达预设距离 或 强制标志置位 或 检测到黑线
            if (total_distance_traveled >= DISTANCE_TO_C || trigger_calibration || gray_byte != 0xFF) {
                sm.current_action = ACT_CALIBRATION;
                sm.flag_just_entered = true;
                trigger_calibration = false;
            }
            break;
            
        case ACT_CALIBRATION:
            if (sm.flag_just_entered) {
                LOG_DEBUG("Entering CALIBRATION mode");
                sm.flag_just_entered = false;
            }
            // 等待稳定检测到黑线（gray_byte != 0xFF）
            if (gray_byte != 0xFF) {
                static uint8_t stable_cnt = 0;
                stable_cnt++;
                if (stable_cnt >= 10) {
                    SM_NextAction(); // 切换到 ACT_TRACKING
                    stable_cnt = 0;
                }
            } else {
                static uint8_t stable_cnt = 0;
                stable_cnt = 0;       // 变回全白则重置计数
            }
            break;
            
        case ACT_TRACKING:
        {
            static uint8_t white_cnt = 0;
            if (sm.flag_just_entered) {
                LOG_DEBUG("action=TRACKING, stage_index=%d", sm.stage_index);
                PID_Node_ResetIntegral(&pidGrayscale);
                sm.flag_just_entered = false;
                white_cnt = 0;
            }
            if (gray_byte == 0xFF) {
                white_cnt++;
                if (white_cnt >= 3) {
                    SM_NextAction();
                    sm.need_sound_light = true;
                    white_cnt = 0;
                }
            } else {
                white_cnt = 0;
            }
            break;
        }
            
        case ACT_STOP:
            if (sm.flag_just_entered) {
                Motor_setSpeed(&motor1, 0);
                Motor_setSpeed(&motor2, 0);
                sm.need_sound_light = true;
                sm.flag_just_entered = false;
            }
            sm.is_task_complete = true;
            break;
            
        default:
            break;
    }
}

bool SM_IsRunning(void) {
    return (!sm.is_task_complete && sm.current_task != TASK_NONE);
}

float SM_GetTargetAngle(void) {
    return sm.target_angle;
}

ActionState_t SM_GetCurrentAction(void) {
    return sm.current_action;
}
