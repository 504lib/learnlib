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

static float GetRelativeTurnAngle(TaskType_t task, uint8_t stage, uint8_t lap) {
    if (task == TASK_2) {
        switch (stage) {
            case 0: return 0.0f;
            case 2: return -6.0f;   // 根据实际标定
            default: return 0.0f;
        }
    } else if (task == TASK_3) {
        switch (stage) {
            case 0: return -38.5f;   // A->C
            case 2: return +40.0f;  // B->D
            default: return 0.0f;
        }
    } else if (task == TASK_4) {
        float base;
        switch (stage) {
            case 0: base = -38.5f; break;
            case 2: base = +48.5f; break;
            default: base = 0.0f; break;
        }
        float correction = (lap == 0 && stage == 0) ? 0.0f : -6.0f;
        return base + correction;
    }
    return 0.0f;
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
			// 记录进入直行时的当前航向
			float current_yaw = mpu->yaw;
			float relative_turn = 0.0f;

			if (sm.current_task == TASK_2) {
				// TASK_2 使用相对转角表（可复用统一函数）
				relative_turn = GetRelativeTurnAngle(sm.current_task, sm.stage_index, sm.lap_count);
			}
			else if (sm.current_task == TASK_3 || sm.current_task == TASK_4) {
				relative_turn = GetRelativeTurnAngle(sm.current_task, sm.stage_index, sm.lap_count);
			}
			else if (sm.stage_index == 0) {
				// 其他任务首次直行：保持当前航向
				relative_turn = 0.0f;
			}
			// 计算目标角度
			sm.target_angle = current_yaw + relative_turn;
			// 归一化到 [-180, 180]
			if (sm.target_angle > 180.0f) sm.target_angle -= 360.0f;
			if (sm.target_angle < -180.0f) sm.target_angle += 360.0f;

			// 可选：打印调试信息
			LOG_DEBUG("STRAIGHT: stage=%d, cur_yaw=%.1f, rel_turn=%.1f, target=%.1f",
					  sm.stage_index, current_yaw, relative_turn, sm.target_angle);

			PID_Node_ResetIntegral(&pidAngle);
			sm.flag_just_entered = false;
		}
		// 检测黑线 -> 切换到循迹
		if (gray_byte != 0xFF) {
			SM_NextAction();
			sm.need_sound_light = true;
		}
		break;		
//	case ACT_STRAIGHT:
//		if (sm.flag_just_entered) { 
//			if(sm.current_task == TASK_2)
//			{
//				switch(sm.stage_index)
//				{
//					case 0: sm.target_angle = 0.0f;	break;
//					case 2: sm.target_angle = mpu->yaw - 6.0f; 				
//					LOG_DEBUG("Subsequent straight (stage_index=%d), set target_angle=%.1f (yaw-22)",
//				    sm.stage_index, sm.target_angle);
//					break;
//					default: break;
//				}
//			}
//			if (sm.current_task == TASK_3 || sm.current_task == TASK_4) {
//				float base_angle = 0.0f;
//				switch (sm.stage_index) {
//					case 0: base_angle = -39.0f; break;   // A -> C
//					case 2: base_angle = -146.0f; break;  // B -> D
//					default: base_angle = mpu->yaw; break;
//				}
//				if (sm.current_task == TASK_4) {
//					float correction = 0.0f;
//					if (!(sm.lap_count == 0 && sm.stage_index == 0)) {
//						correction = -22.0f;
//					}
//					sm.target_angle = base_angle + correction;
//					LOG_DEBUG("TASK_4 lap=%d stage=%d base=%.1f correction=%.1f target=%.1f",
//							  sm.lap_count, sm.stage_index, base_angle, correction, sm.target_angle);
//				} else {
//					sm.target_angle = base_angle;
//					LOG_DEBUG("TASK_3 straight stage_index=%d, set target_angle=%.1f",
//							  sm.stage_index, sm.target_angle);
//				}
//			}
//			else if (sm.stage_index == 0) {
//				sm.target_angle = mpu->yaw;
//				LOG_DEBUG("First straight (stage_index=0), lock current yaw=%.1f", sm.target_angle);
//			}
////			else {
////				sm.target_angle = mpu->yaw - 22.0f;
////				LOG_DEBUG("Subsequent straight (stage_index=%d), set target_angle=%.1f (yaw-22)",
////						  sm.stage_index, sm.target_angle);
////			}
//			// 角度归一化到 [-180, 180]
//			if (sm.target_angle > 180.0f) sm.target_angle -= 360.0f;
//			if (sm.target_angle < -180.0f) sm.target_angle += 360.0f;
//			
//			PID_Node_ResetIntegral(&pidAngle);
//			sm.flag_just_entered = false;
//		}

//		// 检测到黑线（非全白）时切换到循迹
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
