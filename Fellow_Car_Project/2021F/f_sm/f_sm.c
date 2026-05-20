#include "f_sm.h"
#include "control.h"            // 需要用到 pidGrayscale, pidAngle, mpu 等
#include "mpu6050_user.h"
#include "grayscale.h"
#include "Log.h"
#include "tim.h"                // 如果需要 HAL_GetTick

// ---------- 全局变量定义 ----------
float          g_target_yaw = 0.0f;
float          g_distance_traveled = 0.0f;
int            g_target_ward = 0;
CtrlMode_t     g_ctrl_mode = CTRL_MODE_IDLE;

// ---------- 静态变量 ----------
static FSM             fsm_app;                  // 状态机实例
static FSM_Node        fsm_nodes[FSM_STATE_MAX]; // 节点数组

// 任务参数（可根据实际情况通过串口修改）
#define STRAIGHT_DISTANCE      0.65f    // 直行到达路口/见线前的预估值 (m)
#define STOP_DURATION_MS       3000     // 在病房前停车时间 (ms)

static uint32_t stop_start_tick = 0;     // 停车开始时刻
static bool     ward_detected = false;   // 模拟：是否检测到目标病房

// ---------- 辅助函数声明 ----------
static void Enter_State(FSM_StateID_t state);
static bool Check_Ward_Detected(void);   // 实际应用需要根据数字识别或其他传感器
static void Reset_Distance(void);

// ============================================================
// 状态进入动作（设置控制模式、保存航向、清零计数器等）
// ============================================================
static void Enter_Idle(void)
{
    g_ctrl_mode = CTRL_MODE_IDLE;
    LOG_INFO("Enter IDLE");
}

static void Enter_Straight(void)
{
    MPU6050_Data_t* mpu = MPU6050_GetHandle();
    g_target_yaw = mpu->yaw;               // 锁定当前航向
    g_ctrl_mode = CTRL_MODE_STRAIGHT;
    Reset_Distance();                      // 清零路程累积
    LOG_INFO("Enter STRAIGHT, target yaw=%.1f", g_target_yaw);
}

static void Enter_Tracking(void)
{
    g_ctrl_mode = CTRL_MODE_TRACKING;
    // 复位灰度 PID 积分，避免上一阶段累积影响
    PID_Node_ResetIntegral(&pidGrayscale);
    LOG_INFO("Enter TRACKING");
}

static void Enter_StopAtWard(void)
{
    g_ctrl_mode = CTRL_MODE_STOP;
    stop_start_tick = HAL_GetTick();
    ward_detected = false;
    LOG_INFO("Enter STOP_AT_WARD (ward %d)", g_target_ward);
}

static void Enter_ReturnStraight(void)
{
    MPU6050_Data_t* mpu = MPU6050_GetHandle();
    g_target_yaw = mpu->yaw;
    g_ctrl_mode = CTRL_MODE_STRAIGHT;
    Reset_Distance();
    LOG_INFO("Enter RETURN_STRAIGHT");
}

static void Enter_ReturnTracking(void)
{
    g_ctrl_mode = CTRL_MODE_TRACKING;
    PID_Node_ResetIntegral(&pidGrayscale);
    LOG_INFO("Enter RETURN_TRACKING");
}

static void Enter_Finish(void)
{
    g_ctrl_mode = CTRL_MODE_STOP;
    LOG_INFO("Enter FINISH (task complete)");
}

// ============================================================
// 状态转移函数
// ============================================================
static void Transition_To(FSM_StateID_t new_state)
{
    if (new_state >= FSM_STATE_MAX) return;
    fsm_app.current_state = new_state;
    Enter_State(new_state);
}

static void Enter_State(FSM_StateID_t state)
{
    switch (state) {
        case FSM_STATE_IDLE:              Enter_Idle(); break;
        case FSM_STATE_STRAIGHT:          Enter_Straight(); break;
        case FSM_STATE_TRACKING:          Enter_Tracking(); break;
        case FSM_STATE_STOP_AT_WARD:      Enter_StopAtWard(); break;
        case FSM_STATE_RETURN_STRAIGHT:   Enter_ReturnStraight(); break;
        case FSM_STATE_RETURN_TRACKING:   Enter_ReturnTracking(); break;
        case FSM_STATE_FINISH:            Enter_Finish(); break;
        default: break;
    }
}

// ============================================================
// 外部指令接收（从蓝牙/按键/调试器调用）
// ============================================================
void FSM_App_ReceiveCommand(uint8_t cmd)
{
    switch (cmd) {
        case 0x01:   // 去病房1
            g_target_ward = 1;
            Transition_To(FSM_STATE_STRAIGHT);
            break;

        case 0x02:   // 去病房2
            g_target_ward = 2;
            Transition_To(FSM_STATE_STRAIGHT);
            break;

        case 0x03:   // 从任意位置直接返回药房（可设计为直行返回）
            Transition_To(FSM_STATE_RETURN_STRAIGHT);
            break;

        case 0x04:   // 紧急停止
            Transition_To(FSM_STATE_IDLE);
            break;

        default:
            LOG_WARN("Unknown command: 0x%02X", cmd);
            break;
    }
}

// ============================================================
// 每个控制周期调用的主循环
// ============================================================
void FSM_App_Run(void)
{
    // 获取传感器数据引用
    MPU6050_Data_t* mpu = MPU6050_GetHandle();

    switch (fsm_app.current_state) {
        case FSM_STATE_IDLE:
            // 等待外部命令，不做自动转移
            break;

        case FSM_STATE_STRAIGHT:
            // 条件1：到达预设距离（防止一直直行）
            // 条件2：灰度传感器看到黑线（说明接近路口或目的地）
            if (g_distance_traveled >= STRAIGHT_DISTANCE ||
                gray_byte != 0xFF) 
            {
                Transition_To(FSM_STATE_TRACKING);
            }
            break;

        case FSM_STATE_TRACKING:
            // 到达目标病房的条件（需根据实际数字识别或特殊标记判断）
            if (Check_Ward_Detected()) {
                Transition_To(FSM_STATE_STOP_AT_WARD);
            }
            // 注意：从跟踪状态返回主路的条件应在返回循迹中处理，
            // 这里只处理去程的病房停靠。
            break;

        case FSM_STATE_STOP_AT_WARD:
            // 停留指定时间后自动返回
            if (HAL_GetTick() - stop_start_tick >= STOP_DURATION_MS) {
                Transition_To(FSM_STATE_RETURN_TRACKING);
            }
            break;

        case FSM_STATE_RETURN_STRAIGHT:
            // 返回直行结束条件：到达药房附近（可根据距离或特定标记）
            if (g_distance_traveled >= STRAIGHT_DISTANCE ||
                gray_byte != 0xFF)
            {
                Transition_To(FSM_STATE_FINISH);
            }
            break;

        case FSM_STATE_RETURN_TRACKING:
            // 返回循迹结束条件：到达主路后可能转直行
            // 为简化，这里直接转 Finish
            if (Check_Ward_Detected()) {   // 示例：检测到药房标记
                Transition_To(FSM_STATE_FINISH);
            }
            break;

        case FSM_STATE_FINISH:
            // 任务完成后等待新命令，可自动转 IDLE
            Transition_To(FSM_STATE_IDLE);
            break;

        default:
            break;
    }
}

// ============================================================
// 初始化应用状态机
// ============================================================
void FSM_App_Init(void)
{
    // 1. 初始化底层 FSM 库实例
    FSM_Init(&fsm_app);

    // 2. 初始化所有状态节点（action 留空，本模块不依赖库的 action）
    FMS_Node_Init(&fsm_nodes[FSM_STATE_IDLE],             "Idle",              NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_STRAIGHT],         "Straight",          NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_TRACKING],         "Tracking",          NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_STOP_AT_WARD],     "StopAtWard",        NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_RETURN_STRAIGHT],  "ReturnStraight",    NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_RETURN_TRACKING],  "ReturnTracking",    NULL);
    FMS_Node_Init(&fsm_nodes[FSM_STATE_FINISH],           "Finish",            NULL);

    for (int i = 0; i < FSM_STATE_MAX; i++) {
        FSM_Add_Node(&fsm_app, &fsm_nodes[i], (State_ID_t)i);
    }

    // 3. 设置初始状态
    fsm_app.current_state = FSM_STATE_IDLE;
    Enter_State(FSM_STATE_IDLE);        // 执行 Idle 进入动作
}

// ============================================================
// 工具函数
// ============================================================
bool FSM_App_IsRunning(void)
{
    return (fsm_app.current_state != FSM_STATE_IDLE &&
            fsm_app.current_state != FSM_STATE_FINISH);
}

static void Reset_Distance(void)
{
    // 清空 control.c 中累计的路程变量（需在 control.c 中提供接口）
    extern void Control_ResetDistance(void);
    Control_ResetDistance();
    g_distance_traveled = 0.0f;
}

static bool Check_Ward_Detected(void)
{
    // ！！！这里需要根据实际传感器实现 ！！！
    // 例如：数字识别结果、特定灰度图案、RFID 等
    // 目前用按键模拟（可保留用于调试）
    return ward_detected;
}

// 提供一个调试接口：手动设置检测到病房
void FSM_SetWardDetected(bool detected) {
    ward_detected = detected;
}
