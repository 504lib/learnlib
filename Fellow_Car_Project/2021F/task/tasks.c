#include "tasks.h"
#include "main.h"      // 为了 HAL_GPIO_TogglePin、LED 定义
#include "usart.h"     // 为了 huart1
#include <stdio.h>     // 为了 snprintf
#include <string.h>    // 为了 strlen
#include "main.h" 
#include "oled.h"
#include "mpu6050_user.h"
#include "grayscale.h"
#include "key_control.h"
#include "PID_Node.h"
#include "control.h"   // 新增，包含全局 PID 实例和速度变量
#include "app_protocol.h"
#include "static_stack.h"
#include "Motor_AT4950.h"

// ---------- 动作类型（与命令码对应） ----------
#define ACTION_STRAIGHT  0x01
#define ACTION_LEFT      0x02
#define ACTION_RIGHT     0x03

#define STRAIGHT_DISTANCE 0.5f   // 直行距离(米)
#define TURN_SPEED        0.08f  // 转弯轮速(m/s)
#define TURN_DEADBAND     3.0f   // 转弯角度死区(度)


#define HIGH_4_BITS_MASK 0xF0
#define LOW_4_BITS_MASK  0x0F

typedef enum {
    TASK_IDLE,              // 没有任务
    TASK_GO_TO_CROSS,       // 直行前往十字路口
    TASK_WAITING_AT_CROSS,  // 在十字路口停车等待转弯指令
	TASK_TURN_LEFT,     	// 左转
    TASK_TURN_RIGHT,    	// 右转
    TASK_GO_TO_END,         // 转弯后直行直到终点
    TASK_WAIT_DELIVERY_CONFIRM,  // 等待送药确认
    TASK_TURN_AROUND,            // 原地调头180°
    TASK_RETURN                  // 执行返程栈	
} TaskState_t;

// ---------- 返程子状态 ----------
typedef enum {
    RET_POP_ACTION,
    RET_EXEC_STRAIGHT,
    RET_EXEC_TURN
} ReturnSubState;

static TaskState_t task_state = TASK_IDLE;   // 当前任务状态
DECLARE_STATIC_STACK(ActionStack, uint8_t, 16); 
ActionStack_t action_stack;

Protothread_t task1_pt;
Protothread_t task2_pt;
Protothread_t SerialTask_pt;
Protothread_t oled_pt;
Protothread_t fsm_pt;

extern PID_Node pidMotor1Speed;
extern PID_Node pidMotor2Speed;

bool isFirstExecute = true;

// 初始化所有任务
void Tasks_Init(void)
{
    PT_INIT(&task1_pt);
    PT_INIT(&task2_pt);
    PT_INIT(&SerialTask_pt);
	PT_INIT(&oled_pt); 
	PT_INIT(&fsm_pt);
    ActionStack_INIT(&action_stack);
}

// 任务函数实现
void task1(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while(1)
    {
        HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
        PT_WAIT_TICK(pt, 500);
    }
    PT_END(pt);
}

void task2(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while(1)
    {
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
        PT_WAIT_TICK(pt, 1000);
    }
    PT_END(pt);
}

void SerialTask(Protothread_t* pt)
{
    char buffer[50] = {0};
    mpu = MPU6050_GetHandle();   // 获取全局实例指针
    PT_BEGIN(pt);
    static size_t times = 0;
    while(1)
    {
//		snprintf(buffer, sizeof(buffer), "%.3f,%.3f\n", Actual_Speed_A, Actual_Speed_B);
//        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
        PT_WAIT_TICK(pt, 100);   // 每200ms打印一次
//        snprintf(buffer, sizeof(buffer), "Serial task has execute %d time(s)\n", ++times);
//		snprintf(buffer, sizeof(buffer),"Y:%.1f R:%.1f", mpu->yaw, mpu->roll);
//        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
//	printf("Target=%.2f, Meas=%.2f, Out=%.2f, Speed1=%.5f,Speed=%.5f\n", 
//       pidMotor1Speed.setpoint, 
//       pidMotor1Speed.measured_value,
//       pidMotor1Speed.output,
//	   Actual_Speed_A,
//	   Actual_Speed_B);
//	printf("PID_SPEED1: output:%.2f,measure_value:%.2f,setpoint = %.2f\n",
//			pidMotor1Speed.output,
//			pidMotor1Speed.measured_value,
//			pidMotor1Speed.setpoint);

//	PT_WAIT_TICK(pt, 200);
    }
    PT_END(pt);
}

void OLED_Task(Protothread_t* pt)
{
    char buffer[20] = {0};
    uint8_t mode = 0;
    PT_BEGIN(pt);
    while (1)
    {
        mode = KeyControl_GetDisplayMode();
//		if (mode == 0) {
//			// 临时调试：显示 mpu 地址和 yaw/roll 原始值
//			snprintf(buffer, sizeof(buffer), "mpu:0x%p", mpu);
//			OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
//			if (mpu != NULL) {
//				snprintf(buffer, sizeof(buffer), "y=%.1f r=%.1f", mpu->yaw, mpu->roll);
//				OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
//			} else {
//				OLED_ShowString(0, 16, "mpu is NULL", 16, 1);
//			}
//		}
        if (mode == 0)   // 姿态模式（简化，只显示当前角度）
        {
            snprintf(buffer, sizeof(buffer), "Y:%.1f", mpu->yaw);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "R:%.1f", mpu->roll);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
			OLED_ShowString(0, 48, (uint8_t*)"TAT TAT TAT TAT", 16, 1);
            // 不再显示 Y_TAR 和 A_target
        }
        else if (mode == 1)   // 灰度模式
        {
            snprintf(buffer, sizeof(buffer), "gray:");
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            for (size_t i = 0; i < 8; i++)
            {
                if (gray_byte & (1 << i))
                    OLED_ShowString(48 + i * 8, 0, (uint8_t*)"1", 16, 1);
                else
                    OLED_ShowString(48 + i * 8, 0, (uint8_t*)"0", 16, 1);
            }
            snprintf(buffer, sizeof(buffer), "gray_err:%.2f", gray_error);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
        }
        else if (mode == 2)   // 速度模式
        {
            snprintf(buffer, sizeof(buffer), "val_A:%.2f", Actual_Speed_A);
            OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_B:%.2f", Actual_Speed_B);
            OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_A_T:%.2f", pidMotor1Speed.setpoint);
            OLED_ShowString(0, 32, (uint8_t*)buffer, 16, 1);
            snprintf(buffer, sizeof(buffer), "val_B_T:%.2f", pidMotor2Speed.setpoint);
            OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
        }
//        else if (mode == 3)   // 任务显示模式
//        {
//			OLED_ShowString(0, 0, (uint8_t*)"TASK1", 16, 1);
//        }
//        else if (mode == 4)   // 任务显示模式
//        {
//			OLED_ShowString(0, 0, (uint8_t*)"TASK2", 16, 1);
//        }
//        else if (mode == 5)   // 任务显示模式
//        {
//			OLED_ShowString(0, 0, (uint8_t*)"TASK3", 16, 1);
//        }
//        else if (mode == 6)   // 任务显示模式
//        {
//			OLED_ShowString(0, 0, (uint8_t*)"TASK4", 16, 1);
//        }		
        OLED_Refresh();
        PT_WAIT_TICK(pt, 20);
    }
    PT_END(pt);
}


void Task_APP_ReceiveCmd(Protothread_t* pt)
{
    uint8_t cmd;
    static bool pushed_end = false;   // 防止 TASK_GO_TO_END 重复压栈
    PT_BEGIN(pt);
    while (1)
    {
        if (App_CmdDequeue(&cmd)) {
            if (cmd == 0x01) {
                if (task_state != TASK_IDLE && task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Task already running, ignore 0x01");
                } else {
                    ActionStack_CLEAR(&action_stack);   // 新任务清空栈
                    ActionStack_PUSH(&action_stack, cmd);					
                    LOG_INFO("Start go to cross");
                    task_state = TASK_GO_TO_CROSS;
                    ctrl_mode = CTRL_MODE_TRACKING;
					pushed_end = false;
                }
            } else if (cmd == 0x02) {
                if (task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Not at cross, ignore left turn");
                } else {
					ActionStack_PUSH(&action_stack, cmd);
                    LOG_INFO("Left turn then go to cross");
                    task_state = TASK_TURN_LEFT;
                }
            } else if (cmd == 0x03) {
                if (task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Not at cross, ignore right turn");
                } else {
					ActionStack_PUSH(&action_stack, cmd);
                    LOG_INFO("Right turn then go to end");
                    task_state = TASK_TURN_RIGHT;
                }
            } else if (cmd == 0x04) {   // 送药完成确认
                if (task_state != TASK_WAIT_DELIVERY_CONFIRM) {
                    LOG_WARN("Not at delivery point, ignore confirm");
                } else {
                    LOG_INFO("Delivery confirmed, preparing to return");
                    task_state = TASK_TURN_AROUND;
                }
            }
			
        }

        PT_WAIT_TICK(pt, 100);
    }
    PT_END(pt);
}


void Task_Go_To_Gross(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while (1)
    {
        PT_WAIT_UNTIL(pt, task_state == TASK_GO_TO_CROSS);
        if (Control_IsCrossDetected(4)) {			
                HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
                PID_Node_SetSetpoint(&pidMotor1Speed, 0.0f);
                PID_Node_SetSetpoint(&pidMotor2Speed, 0.0f);
                ctrl_mode = CTRL_MODE_IDLE;
                task_state = TASK_WAITING_AT_CROSS;
                LOG_INFO("Arrived at cross, waiting for turn cmd");
				isFirstExecute = false;
        }
    }
    
    PT_END(pt);
}


void Task_FSM(Protothread_t *pt) {
    MPU6050_Data_t *mpu = NULL;
    uint8_t cmd;
    float target_angle;
    // ===== 转弯必须用 static，否则恢复时变量值丢失 =====
    static float turn_target_angle = 0.0f;
    static MPU6050_Data_t *turn_mpu = NULL;
    static ReturnSubState ret_state = RET_POP_ACTION; //返程
    static uint8_t ret_action = 0;
    static bool pushed_end = false;   // 防止 TASK_GO_TO_END 重复压栈
	
    PT_BEGIN(pt);
    while (1) {
        // ========== 处理蓝牙命令 ==========
        if (App_CmdDequeue(&cmd)) {
            if (cmd == 0x01) {
                if (task_state != TASK_IDLE && task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Task already running, ignore 0x01");
                } else {
                    ActionStack_CLEAR(&action_stack);   // 新任务清空栈
                    ActionStack_PUSH(&action_stack, cmd);					
                    LOG_INFO("Start go to cross");
                    task_state = TASK_GO_TO_CROSS;
                    ctrl_mode = CTRL_MODE_TRACKING;
					pushed_end = false;
                }
            } else if (cmd == 0x02) {
                if (task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Not at cross, ignore left turn");
                } else {
					ActionStack_PUSH(&action_stack, cmd);
                    LOG_INFO("Left turn then go to cross");
                    task_state = TASK_TURN_LEFT;
                }
            } else if (cmd == 0x03) {
                if (task_state != TASK_WAITING_AT_CROSS) {
                    LOG_WARN("Not at cross, ignore right turn");
                } else {
					ActionStack_PUSH(&action_stack, cmd);
                    LOG_INFO("Right turn then go to end");
                    task_state = TASK_TURN_RIGHT;
                }
            } else if (cmd == 0x04) {   // 送药完成确认
                if (task_state != TASK_WAIT_DELIVERY_CONFIRM) {
                    LOG_WARN("Not at delivery point, ignore confirm");
                } else {
                    LOG_INFO("Delivery confirmed, preparing to return");
                    task_state = TASK_TURN_AROUND;
                }
            }
			
        }
		
        // ========== 执行当前任务状态==========
		
        if (task_state == TASK_GO_TO_CROSS) {
//            if (gray_byte == 0x00) {
              if (Control_IsCrossDetected(4)) {			
                HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
                PID_Node_SetSetpoint(&pidMotor1Speed, 0.0f);
                PID_Node_SetSetpoint(&pidMotor2Speed, 0.0f);
                ctrl_mode = CTRL_MODE_IDLE;
                task_state = TASK_WAITING_AT_CROSS;
                LOG_INFO("Arrived at cross, waiting for turn cmd");
				isFirstExecute = false;
//				LOG_INFO("current First? %s",(isFirstExecute) ? "true" : "false");
            }
			// 全白且不是第一次直行 -> 去程终点			
			else if(gray_byte == 0xFF && !isFirstExecute)
			{
                if (!pushed_end) {
                    ActionStack_PUSH(&action_stack, ACTION_STRAIGHT); // 最后一段直行压栈
                    pushed_end = true;
                }
				HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, GPIO_PIN_RESET);
//                Control_SetManualSpeeds(0.0f, 0.0f);
                ctrl_mode = CTRL_MODE_IDLE;
                task_state = TASK_GO_TO_END;
                LOG_INFO("Reached end (white mark)");
			}
            PT_WAIT_TICK(pt, 10);
        }
        else if (task_state == TASK_TURN_LEFT) {
            // 第一次进入时初始化目标角度
            turn_mpu = MPU6050_GetHandle();
            turn_target_angle = turn_mpu->yaw + 90.0f;
            if (turn_target_angle > 180.0f) turn_target_angle -= 360.0f;

            Control_SetManualSpeeds(-TURN_SPEED, TURN_SPEED);

            // 等待，使用静态变量
            PT_WAIT_UNTIL(pt,
                fabsf(AngleDiff(turn_mpu->yaw, turn_target_angle)) <= TURN_DEADBAND
            );

            Control_SetManualSpeeds(0.0f, 0.0f);
            ctrl_mode = CTRL_MODE_TRACKING;
            // 先离开当前路口，防止立刻重新检测到同一个路口
            PT_WAIT_UNTIL(pt, !Control_IsCrossDetected(4));
            task_state = TASK_GO_TO_CROSS;
        }
        else if (task_state == TASK_TURN_RIGHT) {
            // 第一次进入时初始化目标角度
            turn_mpu = MPU6050_GetHandle();
            turn_target_angle = turn_mpu->yaw - 90.0f;
            if (turn_target_angle < -180.0f) turn_target_angle += 360.0f;

            Control_SetManualSpeeds(TURN_SPEED, -TURN_SPEED);

            // 等待，使用静态变量
            PT_WAIT_UNTIL(pt,
                fabsf(AngleDiff(turn_mpu->yaw, turn_target_angle)) <= TURN_DEADBAND
            );

            Control_SetManualSpeeds(0.0f, 0.0f);
            ctrl_mode = CTRL_MODE_TRACKING;
            // 先离开当前路口，防止立刻重新检测到同一个路口
            PT_WAIT_UNTIL(pt, !Control_IsCrossDetected(4));
            task_state = TASK_GO_TO_CROSS;
        }
        else if (task_state == TASK_GO_TO_END) {
			// 直接进入等待送药确认，不再重复等待全白
			task_state = TASK_WAIT_DELIVERY_CONFIRM;
			LOG_INFO("Reached end, waiting for delivery confirm");
			PT_WAIT_TICK(pt, 10);   // 可选的短暂让出			
//            PT_WAIT_UNTIL(pt, Control_IsAtEnd());
//            Control_SetManualSpeeds(0.0f, 0.0f);
//            ctrl_mode = CTRL_MODE_IDLE;
//            task_state = TASK_IDLE;
//            LOG_INFO("Reached end, task finished");
//            uint8_t completed_cmd;
//            while (!ActionStack_IS_EMPTY(&action_stack))
//            {
//                ActionStack_POP(&action_stack, &completed_cmd);
//                LOG_INFO("Completed command: 0x%02X", completed_cmd);
//            }
//            isFirstExecute = true;  // 重置标志，准备下一轮任务
        }
        else if (task_state == TASK_TURN_AROUND) {
            // 原地调头 180°
            turn_mpu = MPU6050_GetHandle();
            turn_target_angle = turn_mpu->yaw + 180.0f;
            if (turn_target_angle > 180.0f) turn_target_angle -= 360.0f;
            if (turn_target_angle < -180.0f) turn_target_angle += 360.0f;
            Control_SetManualSpeeds(TURN_SPEED, -TURN_SPEED);
            PT_WAIT_UNTIL(pt,
                fabsf(AngleDiff(turn_mpu->yaw, turn_target_angle)) <= TURN_DEADBAND);
            Control_SetManualSpeeds(0.0f, 0.0f);
            ctrl_mode = CTRL_MODE_TRACKING;
            task_state = TASK_RETURN;
            ret_state = RET_POP_ACTION;
            LOG_INFO("Turned around, starting return");
        }
        else if (task_state == TASK_RETURN) {
            // ========== 返程子状态机（if-else 实现）==========
            if (ret_state == RET_POP_ACTION) {
                if (!ActionStack_POP(&action_stack, &ret_action)) {
                    Control_SetManualSpeeds(0.0f, 0.0f);
                    ctrl_mode = CTRL_MODE_IDLE;
                    task_state = TASK_IDLE;
                    ret_state = RET_POP_ACTION;
                    isFirstExecute = true;
                    LOG_INFO("Return to start completed");
                } else {
                    if (ret_action == ACTION_STRAIGHT) {
						LOG_INFO("current ret_cation:ACTION_STRAIGHT");
                        ctrl_mode = CTRL_MODE_TRACKING;
                        ret_state = RET_EXEC_STRAIGHT;
                    } else {
                        ret_action = (ret_action == ACTION_LEFT) ? ACTION_RIGHT : ACTION_LEFT;
						LOG_INFO("current ret_cation:%s",(ret_action == ACTION_LEFT) ? "ACTION_LEFT" : "ACTION_RIGHT");
                        ret_state = RET_EXEC_TURN;
                    }
                }
            }
			else if (ret_state == RET_EXEC_STRAIGHT) {
				// 1. 先离开当前路口（避免刚转弯完误判）
				PT_WAIT_UNTIL(pt, !Control_IsCrossDetected(4));

                PT_WAIT_UNTIL(pt, gray_byte == 0xFF || gray_byte == 0x00 || gray_byte & HIGH_4_BITS_MASK == 0 || gray_byte & LOW_4_BITS_MASK == 0); // 等待离开路口，进入全白或全黑或任一侧全黑状态
				// 2. 再等待下一个目标
				// if (ActionStack_SIZE(&action_stack) > 0) {
				// 	PT_WAIT_UNTIL(pt, Control_IsCrossDetected(4));   // 中间路口
                //     LOG_INFO("current Stack no empty");
				// } else {
				// 	PT_WAIT_UNTIL(pt, gray_byte == 0xFF);            // 终点起点
				// }

				Control_SetManualSpeeds(0.0f, 0.0f);
				ctrl_mode = CTRL_MODE_IDLE;
				ret_state = RET_POP_ACTION;
				PT_WAIT_TICK(pt, 10);
			}			
            else if (ret_state == RET_EXEC_TURN) {
                turn_mpu = MPU6050_GetHandle();
                if (ret_action == ACTION_LEFT) {
                    turn_target_angle = turn_mpu->yaw + 90.0f;
                    if (turn_target_angle > 180.0f) turn_target_angle -= 360.0f;
                    Control_SetManualSpeeds(-TURN_SPEED, TURN_SPEED);
                } else { // ACTION_RIGHT
                    turn_target_angle = turn_mpu->yaw - 90.0f;
                    if (turn_target_angle < -180.0f) turn_target_angle += 360.0f;
                    Control_SetManualSpeeds(TURN_SPEED, -TURN_SPEED);
                }
                PT_WAIT_UNTIL(pt,
                    fabsf(AngleDiff(turn_mpu->yaw, turn_target_angle)) <= TURN_DEADBAND);
				LOG_WARN("current pass");
                Control_SetManualSpeeds(0.0f, 0.0f);
                ctrl_mode = CTRL_MODE_TRACKING;
                ret_state = RET_EXEC_STRAIGHT;
				/** todo 排查exec_straight状态问题·
				*/
            }
        }			
        else if (task_state == TASK_WAITING_AT_CROSS) {
            PT_WAIT_TICK(pt, 50);
        }
        else {  // TASK_IDLE 等
            PT_WAIT_TICK(pt, 50);
        }
        // 注意：每个分支内已有 PT_WAIT_*，所以尾部不再需要额外等待
    }
    PT_END(pt);
}

//void task4(Protothread_t* pt)
//{
//	PT_BEGIN(pt);
//	while(1)
//	{
//		uint8_t mode = 0;
//		PT_WAIT_UNTIL(pt,mode == 1);
//		PT_WAIT_TICK(pt,20);
//	}
//	
//	PT_END(pt);
//}
