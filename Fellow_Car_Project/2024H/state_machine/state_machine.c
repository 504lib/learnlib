#include "state_machine.h"
#include "control.h"
#include "grayscale.h"
#include "Motor_AT4950.h"
#include "mpu6050_user.h"
#include "Log.h"

extern MotorAT4950 motor1;
extern MotorAT4950 motor2;

// 全局状态机实例（定义在此）
SM_Context_t sm;

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

void SM_Init(void) {
    sm.current_task = TASK_NONE;
    sm.current_action = ACT_IDLE;
    sm.lap_count = 0;
    sm.stage_index = 0;
    sm.is_task_complete = false;
    sm.need_sound_light = false;
    sm.target_angle = 0.0f;
    sm.flag_just_entered = false;
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
    
    switch (sm.current_action) {
	case ACT_STRAIGHT:
		if (sm.flag_just_entered) {
			// 判断是否是第三题的第一个直行（A→C）
            LOG_DEBUG("action=STRAIGHT, stage_index=%d", sm.stage_index);
			if (sm.current_task == TASK_3 && sm.stage_index == 0) {
				sm.target_angle = 45.0f;
			} else if (sm.stage_index == 0) {
				// 其他任务锁定当前航向
				sm.target_angle = mpu->yaw;
			} else {
				// 加固定修正
				sm.target_angle = mpu->yaw - 15.0f;
				if (sm.target_angle > 180.0f) sm.target_angle -= 360.0f;
				if (sm.target_angle < -180.0f) sm.target_angle += 360.0f;
                LOG_DEBUG("Set target_angle=%.1f", sm.target_angle);
			}
			PID_Node_ResetIntegral(&pidAngle);
			sm.flag_just_entered = false;
		}
		// 检测到黑线（非全白）时切换到循迹
		if (gray_byte != 0xFF) {
			SM_NextAction();
			sm.need_sound_light = true;
		}
		break;		
//	case ACT_STRAIGHT:
//		if (sm.flag_just_entered) {
//			if (sm.stage_index == 0) {
//				sm.target_angle = mpu->yaw;           // 第一次直行：不加修正
//			} else {
//				sm.target_angle = mpu->yaw - 15.0f;    // 后续直行：加固定修正
//            if (sm.target_angle > 180.0f) sm.target_angle -= 360.0f;
//            if (sm.target_angle < -180.0f) sm.target_angle += 360.0f;
//			}
//			PID_Node_ResetIntegral(&pidAngle);
//			sm.flag_just_entered = false;
//		}
//		// 检测到黑线 -> 切换到循迹
//		if (gray_byte != 0xFF) {
//			SM_NextAction();
//			sm.need_sound_light = true;
//		}
//		break;		
            
        case ACT_TRACKING:
            if (sm.flag_just_entered) {
                LOG_DEBUG("action=TRACKING, stage_index=%d", sm.stage_index);
                PID_Node_ResetIntegral(&pidGrayscale);
                sm.flag_just_entered = false;
            }
            // 检测到全白（弧线结束）-> 切换至直行
            if (gray_byte == 0xFF) {
                static uint8_t white_cnt = 0;
                white_cnt++;
                if (white_cnt >= 3) {
                    SM_NextAction();
                    sm.need_sound_light = true;
                    white_cnt = 0;
                }
            } else {
                static uint8_t white_cnt = 0;
                white_cnt = 0;
            }
            break;
            
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
