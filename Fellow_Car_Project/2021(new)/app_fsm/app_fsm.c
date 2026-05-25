#include "app_fsm.h"
#include "control.h"
#include "mpu6050_user.h"
#include "grayscale.h"
#include "key_control.h"
#include "Log.h"
#include "static_stack.h"
#include "Motor_AT4950.h"
#include "app_protocol.h"
#include <math.h>
#include <string.h>

// ============================================================
// 常量
// ============================================================
#define TURN_SPEED        0.08f
#define TURN_DEADBAND     3.0f
#define CMD_CROSS_ARRIVED 0x30
#define HIGH_4_BITS_MASK  0xF0
#define LOW_4_BITS_MASK   0x0F

// ============================================================
// 动作栈
// ============================================================
DECLARE_STATIC_STACK(ActionStack, uint8_t, 16);

// ============================================================
// 全局变量
// ============================================================
static HSM*       g_app_hsm = NULL;
static ActionStack_t g_action_stack;

// ---- 状态节点指针 ----
static HSM_Node*  g_node_root;
static HSM_Node*  g_node_idle;
static HSM_Node*  g_node_mission;
static HSM_Node*  g_node_outbound;
static HSM_Node*  g_node_wait_key2;
static HSM_Node*  g_node_go_cross;
static HSM_Node*  g_node_at_cross;
static HSM_Node*  g_node_wait_turn_cmd;
static HSM_Node*  g_node_turn_left;
static HSM_Node*  g_node_turn_right;
static HSM_Node*  g_node_go_end;
static HSM_Node*  g_node_wait_confirm;
static HSM_Node*  g_node_return_leg;
static HSM_Node*  g_node_turn_around;
static HSM_Node*  g_node_retrace;
static HSM_Node*  g_node_pop_action;
static HSM_Node*  g_node_exec_straight;
static HSM_Node*  g_node_exec_turn;

// ---- 持久状态变量 ----
static float   turn_target = 0.0f;
static MPU6050_Data_t* turn_mpu = NULL;
static int     turn_phase = 0;       // 0=等角度, 1=等离开路口
static bool    g_isFirstExecute = true;
static bool    g_pushed_end = false;
static uint8_t white_gray_byte_count = 0;
static uint8_t g_ret_action = 0;
static bool    g_exec_left_cross = false;  // ExecStraight 是否已离开路口

// ============================================================
// 辅助函数
// ============================================================

static void turn_initiate(float delta_angle, float speed_a, float speed_b)
{
    turn_mpu = MPU6050_GetHandle();
    turn_target = turn_mpu->yaw + delta_angle;
    if (turn_target > 180.0f)  turn_target -= 360.0f;
    if (turn_target < -180.0f) turn_target += 360.0f;
    turn_phase = 0;
    Control_SetManualSpeeds(speed_a, speed_b);
}

static bool turn_angle_done(void)
{
    return fabsf(AngleDiff(turn_mpu->yaw, turn_target)) <= TURN_DEADBAND;
}

static const char* event_name(uint8_t id)
{
    switch (id) {
        case EV_CAM_START:        return "EV_CAM_START";
        case EV_TURN_LEFT:        return "EV_TURN_LEFT";
        case EV_TURN_RIGHT:       return "EV_TURN_RIGHT";
        case EV_DELIVERY_CONFIRM: return "EV_DELIVERY_CONFIRM";
        case EV_TURN_DONE:        return "EV_TURN_DONE";
        case EV_BT_GO:            return "EV_BT_GO";
        case EV_RESET:            return "EV_RESET";
        default:                  return "UNKNOWN";
    }
}

// ============================================================
// Root - 纯容器，无回调
// ============================================================

// ============================================================
// Idle
// ============================================================
static bool handler_Idle(HSM_Event_Package e)
{
    LOG_INFO("[FSM] Handler Idle received %s(0x%02X)", event_name(e.HSM_Event_ID), e.HSM_Event_ID);

    if (e.HSM_Event_ID == EV_CAM_START) {
        ActionStack_PUSH(&g_action_stack, ACTION_STRAIGHT);
        g_pushed_end = false;
        LOG_INFO("[FSM] Idle → WaitKey2 (CAM start)");
        HSM_RequestTransition(g_app_hsm, g_node_wait_key2, e);
        return true;
    }
    if (e.HSM_Event_ID == EV_BT_GO) {
        ActionStack_PUSH(&g_action_stack, ACTION_STRAIGHT);
        g_pushed_end = false;
        LOG_INFO("[FSM] Idle → GoToCross (BT start)");
        HSM_RequestTransition(g_app_hsm, g_node_go_cross, e);
        return true;
    }
    return false;
}

static void entry_Idle(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter Idle — stop motors, clear stack");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_IDLE;
    ActionStack_CLEAR(&g_action_stack);
    g_isFirstExecute = true;
    g_pushed_end = false;
    white_gray_byte_count = 0;
    g_exec_left_cross = false;
    turn_phase = 0;
}

// ============================================================
// Mission
// ============================================================
static bool handler_Mission(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_RESET) {
        LOG_INFO("[FSM] Handler Mission caught EV_RESET → Idle");
        HSM_RequestTransition(g_app_hsm, g_node_idle, e);
        return true;
    }
    LOG_INFO("[FSM] Handler Mission: %s(0x%02X) not handled, bubble up", event_name(e.HSM_Event_ID), e.HSM_Event_ID);
    return false;
}

static void entry_Mission(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter Mission — init action stack");
    // 栈在 Idle 已清空，这里不需要重复
}

static void exit_Mission(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Exit Mission — stop motors, clear stack");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_IDLE;
    ActionStack_CLEAR(&g_action_stack);
    g_isFirstExecute = true;
    g_pushed_end = false;
    white_gray_byte_count = 0;
    g_exec_left_cross = false;
    turn_phase = 0;
}

// ============================================================
// Outbound - 结构节点
// ============================================================
static bool handler_Outbound(HSM_Event_Package e)
{
    // EV_RESET 不在这里处理，让它冒泡到 Mission
    LOG_INFO("[FSM] Handler Outbound: %s(0x%02X) → bubble up", event_name(e.HSM_Event_ID), e.HSM_Event_ID);
    return false;
}

// ============================================================
// WaitKey2
// ============================================================
static void continuous_WaitKey2(void)
{
    if (KeyControl_IsStartTriggered()) {
        KeyControl_ClearStartTrigger();
        ctrl_mode = CTRL_MODE_TRACKING;
        App_SendCamFrame(CMD_KEY_TRIGGER, NULL, 0);
        LOG_INFO("[FSM] WaitKey2 key2 pressed → GoToCross");
        HSM_RequestTransition(g_app_hsm, g_node_go_cross,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    }
}

static void entry_WaitKey2(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter WaitKey2 — waiting for key2 press");
}

// ============================================================
// GoToCross
// ============================================================
static void entry_GoToCross(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter GoToCross — tracking mode");
    ctrl_mode = CTRL_MODE_TRACKING;
}

static void continuous_GoToCross(void)
{
    // 检测十字路口
    if (Control_IsCrossDetected(4)) {
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
        PID_Node_SetSetpoint(&pidMotor1Speed, 0.0f);
        PID_Node_SetSetpoint(&pidMotor2Speed, 0.0f);
        ctrl_mode = CTRL_MODE_IDLE;
        LOG_INFO("[FSM] GoToCross cross detected → WaitTurnCmd");
        App_SendCamFrame(CMD_CROSS_ARRIVED, NULL, 0);
        g_isFirstExecute = false;
        HSM_RequestTransition(g_app_hsm, g_node_wait_turn_cmd,
            (HSM_Event_Package){.HSM_Event_ID = 0});
        return;
    }

    // 检测终点（全白 且 非首次直行）
    if (gray_byte == 0xFF && !g_isFirstExecute) {
        if (!g_pushed_end) {
            ActionStack_PUSH(&g_action_stack, ACTION_STRAIGHT);
            g_pushed_end = true;
            LOG_INFO("[FSM] GoToCross end detected, pushed ACTION_STRAIGHT, stack size=%d",
                     ActionStack_SIZE(&g_action_stack));
        }
        HAL_GPIO_WritePin(AIN_GPIO_Port, AIN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BIN_GPIO_Port, BIN_Pin, GPIO_PIN_RESET);
        ctrl_mode = CTRL_MODE_IDLE;
        LOG_INFO("[FSM] GoToCross reached end → GoToEnd");
        HSM_RequestTransition(g_app_hsm, g_node_go_end,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    }
}

// ============================================================
// AtCross - 结构节点
// ============================================================

// ============================================================
// WaitTurnCmd
// ============================================================
static bool handler_WaitTurnCmd(HSM_Event_Package e)
{
    LOG_INFO("[FSM] Handler WaitTurnCmd received %s(0x%02X)", event_name(e.HSM_Event_ID), e.HSM_Event_ID);

    if (e.HSM_Event_ID == EV_TURN_LEFT) {
        ActionStack_PUSH(&g_action_stack, ACTION_LEFT);
        LOG_INFO("[FSM] WaitTurnCmd → TurnLeft, stack size=%d", ActionStack_SIZE(&g_action_stack));
        HSM_RequestTransition(g_app_hsm, g_node_turn_left, e);
        return true;
    }
    if (e.HSM_Event_ID == EV_TURN_RIGHT) {
        ActionStack_PUSH(&g_action_stack, ACTION_RIGHT);
        LOG_INFO("[FSM] WaitTurnCmd → TurnRight, stack size=%d", ActionStack_SIZE(&g_action_stack));
        HSM_RequestTransition(g_app_hsm, g_node_turn_right, e);
        return true;
    }
    if (e.HSM_Event_ID == EV_CAM_START) {
        ActionStack_PUSH(&g_action_stack, ACTION_STRAIGHT);
        g_pushed_end = false;
        LOG_INFO("[FSM] WaitTurnCmd CAM restart → WaitKey2");
        HSM_RequestTransition(g_app_hsm, g_node_wait_key2, e);
        return true;
    }
    if (e.HSM_Event_ID == EV_BT_GO) {
        ActionStack_PUSH(&g_action_stack, ACTION_STRAIGHT);
        g_pushed_end = false;
        LOG_INFO("[FSM] WaitTurnCmd BT restart → GoToCross");
        HSM_RequestTransition(g_app_hsm, g_node_go_cross, e);
        return true;
    }
    return false;
}

static void entry_WaitTurnCmd(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter WaitTurnCmd — stopped at cross, waiting for turn command");
    // 停车和通知已在 GoToCross 的 continuous 里做了
}

// ============================================================
// TurnLeft
// ============================================================
static void entry_TurnLeft(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter TurnLeft — initiate +90° turn");
    turn_initiate(90.0f, -TURN_SPEED, TURN_SPEED);
}

static void exit_TurnLeft(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Exit TurnLeft");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_TRACKING;
}

static void continuous_TurnLeft(void)
{
    if (turn_phase == 0) {
        if (turn_angle_done()) {
            LOG_INFO("[FSM] TurnLeft angle reached, waiting to leave cross");
            Control_SetManualSpeeds(0.0f, 0.0f);
            ctrl_mode = CTRL_MODE_TRACKING;
            turn_phase = 1;
        }
    } else {
        if (!Control_IsCrossDetected(4)) {
            LOG_INFO("[FSM] TurnLeft left cross → GoToCross");
            HSM_RequestTransition(g_app_hsm, g_node_go_cross,
                (HSM_Event_Package){.HSM_Event_ID = EV_TURN_DONE});
        }
    }
}

// ============================================================
// TurnRight
// ============================================================
static void entry_TurnRight(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter TurnRight — initiate -90° turn");
    turn_initiate(-90.0f, TURN_SPEED, -TURN_SPEED);
}

static void exit_TurnRight(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Exit TurnRight");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_TRACKING;
}

static void continuous_TurnRight(void)
{
    if (turn_phase == 0) {
        if (turn_angle_done()) {
            LOG_INFO("[FSM] TurnRight angle reached, waiting to leave cross");
            Control_SetManualSpeeds(0.0f, 0.0f);
            ctrl_mode = CTRL_MODE_TRACKING;
            turn_phase = 1;
        }
    } else {
        if (!Control_IsCrossDetected(4)) {
            LOG_INFO("[FSM] TurnRight left cross → GoToCross");
            HSM_RequestTransition(g_app_hsm, g_node_go_cross,
                (HSM_Event_Package){.HSM_Event_ID = EV_TURN_DONE});
        }
    }
}

// ============================================================
// GoToEnd
// ============================================================
static void entry_GoToEnd(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter GoToEnd → auto-transition to WaitConfirm");
    // 直接转到等待送药确认
    HSM_RequestTransition(g_app_hsm, g_node_wait_confirm,
        (HSM_Event_Package){.HSM_Event_ID = 0});
}

// ============================================================
// WaitConfirm
// ============================================================
static bool handler_WaitConfirm(HSM_Event_Package e)
{
    LOG_INFO("[FSM] Handler WaitConfirm received %s(0x%02X)", event_name(e.HSM_Event_ID), e.HSM_Event_ID);

    if (e.HSM_Event_ID == EV_DELIVERY_CONFIRM) {
        LOG_INFO("[FSM] WaitConfirm delivery confirmed → TurnAround");
        HSM_RequestTransition(g_app_hsm, g_node_turn_around, e);
        return true;
    }
    return false;
}

static void entry_WaitConfirm(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter WaitConfirm — waiting for delivery confirm");
}

// ============================================================
// Return - 结构节点
// ============================================================
static bool handler_ReturnLeg(HSM_Event_Package e)
{
    LOG_INFO("[FSM] Handler Return: %s(0x%02X) → bubble up", event_name(e.HSM_Event_ID), e.HSM_Event_ID);
    return false;
}

// ============================================================
// TurnAround
// ============================================================
static void entry_TurnAround(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter TurnAround — initiate 180° turn");
    turn_initiate(180.0f, TURN_SPEED, -TURN_SPEED);
}

static void exit_TurnAround(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Exit TurnAround");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_TRACKING;
}

static void continuous_TurnAround(void)
{
    if (turn_angle_done()) {
        LOG_INFO("[FSM] TurnAround done → PopAction (start retrace)");
        HSM_RequestTransition(g_app_hsm, g_node_pop_action,
            (HSM_Event_Package){.HSM_Event_ID = EV_TURN_DONE});
    }
}

// ============================================================
// Retrace - 结构节点
// ============================================================

// ============================================================
// PopAction
// ============================================================
static void entry_PopAction(HSM_Event_Package e)
{
    (void)e;
    uint8_t action;
    if (!ActionStack_POP(&g_action_stack, &action)) {
        LOG_INFO("[FSM] PopAction stack empty → Idle (mission complete)");
        HSM_RequestTransition(g_app_hsm, g_node_idle,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    } else if (action == ACTION_STRAIGHT) {
        LOG_INFO("[FSM] PopAction popped STRAIGHT → ExecStraight, remaining=%d",
                 ActionStack_SIZE(&g_action_stack));
        HSM_RequestTransition(g_app_hsm, g_node_exec_straight,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    } else {
        g_ret_action = (action == ACTION_LEFT) ? ACTION_RIGHT : ACTION_LEFT;
        LOG_INFO("[FSM] PopAction popped %s, reverse to %s → ExecTurn, remaining=%d",
                 (action == ACTION_LEFT) ? "LEFT" : "RIGHT",
                 (g_ret_action == ACTION_LEFT) ? "LEFT" : "RIGHT",
                 ActionStack_SIZE(&g_action_stack));
        HSM_RequestTransition(g_app_hsm, g_node_exec_turn,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    }
}

// ============================================================
// ExecStraight
// ============================================================
static void entry_ExecStraight(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Enter ExecStraight — tracking mode");
    ctrl_mode = CTRL_MODE_TRACKING;
    white_gray_byte_count = 0;
    g_exec_left_cross = false;
}

static void continuous_ExecStraight(void)
{
    // 阶段1: 先离开当前路口（避免刚转弯完误判）
    if (!g_exec_left_cross) {
        if (!Control_IsCrossDetected(4)) {
            g_exec_left_cross = true;
            LOG_INFO("[FSM] ExecStraight left cross, now tracking to next target");
        }
        return;
    }

    // 阶段2: 灰度滤波，检测下一个路口或终点
    if (gray_byte == 0x00) {
        white_gray_byte_count++;
    } else {
        white_gray_byte_count = 0;
    }

    bool reached = (gray_byte == 0xFF)
                || (white_gray_byte_count >= 3)
                || ((gray_byte & HIGH_4_BITS_MASK) == 0)
                || ((gray_byte & LOW_4_BITS_MASK) == 0);

    if (reached) {
        LOG_INFO("[FSM] ExecStraight reached target (gray=0x%02X) → PopAction", gray_byte);
        Control_SetManualSpeeds(0.0f, 0.0f);
        ctrl_mode = CTRL_MODE_IDLE;
        white_gray_byte_count = 0;
        HSM_RequestTransition(g_app_hsm, g_node_pop_action,
            (HSM_Event_Package){.HSM_Event_ID = 0});
    }
}

// ============================================================
// ExecTurn
// ============================================================
static void entry_ExecTurn(HSM_Event_Package e)
{
    (void)e;
    float delta = (g_ret_action == ACTION_LEFT) ? 90.0f : -90.0f;
    float spd_a = (g_ret_action == ACTION_LEFT) ? -TURN_SPEED : TURN_SPEED;
    float spd_b = (g_ret_action == ACTION_LEFT) ? TURN_SPEED : -TURN_SPEED;

    LOG_INFO("[FSM] Enter ExecTurn — action=%s, delta=%.1f°",
             (g_ret_action == ACTION_LEFT) ? "LEFT" : "RIGHT", delta);
    turn_initiate(delta, spd_a, spd_b);
}

static void exit_ExecTurn(HSM_Event_Package e)
{
    (void)e;
    LOG_INFO("[FSM] Exit ExecTurn");
    Control_SetManualSpeeds(0.0f, 0.0f);
    ctrl_mode = CTRL_MODE_TRACKING;
}

static void continuous_ExecTurn(void)
{
    if (turn_angle_done()) {
        LOG_INFO("[FSM] ExecTurn done → ExecStraight");
        HSM_RequestTransition(g_app_hsm, g_node_exec_straight,
            (HSM_Event_Package){.HSM_Event_ID = EV_TURN_DONE});
    }
}

// ============================================================
// 构建状态树
// ============================================================
static bool build_state_tree(void)
{
    static FMS_OBJECT_MEMORY obj_mem;
    static FMS_NODE_MEMORY  node_mem[18];

    g_app_hsm = HSM_Create(&obj_mem);
    if (!g_app_hsm) {
        LOG_FATAL("[FSM] HSM_Create failed!");
        return false;
    }

    // 分配节点
    g_node_root          = HSM_Node_Create(&node_mem[0]);
    g_node_idle          = HSM_Node_Create(&node_mem[1]);
    g_node_mission       = HSM_Node_Create(&node_mem[2]);
    g_node_outbound      = HSM_Node_Create(&node_mem[3]);
    g_node_wait_key2     = HSM_Node_Create(&node_mem[4]);
    g_node_go_cross      = HSM_Node_Create(&node_mem[5]);
    g_node_at_cross      = HSM_Node_Create(&node_mem[6]);
    g_node_wait_turn_cmd = HSM_Node_Create(&node_mem[7]);
    g_node_turn_left     = HSM_Node_Create(&node_mem[8]);
    g_node_turn_right    = HSM_Node_Create(&node_mem[9]);
    g_node_go_end        = HSM_Node_Create(&node_mem[10]);
    g_node_wait_confirm  = HSM_Node_Create(&node_mem[11]);
    g_node_return_leg    = HSM_Node_Create(&node_mem[12]);
    g_node_turn_around   = HSM_Node_Create(&node_mem[13]);
    g_node_retrace       = HSM_Node_Create(&node_mem[14]);
    g_node_pop_action    = HSM_Node_Create(&node_mem[15]);
    g_node_exec_straight = HSM_Node_Create(&node_mem[16]);
    g_node_exec_turn     = HSM_Node_Create(&node_mem[17]);

    // ---- Root 的子节点: Idle, Mission ----
    {
        HSM_Node_Param children[] = {
            { g_node_idle,    handler_Idle,    entry_Idle,    NULL,        NULL },
            { g_node_mission, handler_Mission, entry_Mission, exit_Mission, NULL },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_root, children, 2)) {
            LOG_FATAL("[FSM] Register Root children failed!");
            return false;
        }
    }

    // ---- Mission 的子节点: Outbound, Return ----
    {
        HSM_Node_Param children[] = {
            { g_node_outbound,   handler_Outbound,  NULL, NULL, NULL },
            { g_node_return_leg, handler_ReturnLeg, NULL, NULL, NULL },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_mission, children, 2)) {
            LOG_FATAL("[FSM] Register Mission children failed!");
            return false;
        }
    }

    // ---- Outbound 的子节点: WaitKey2, GoToCross, AtCross, GoToEnd, WaitConfirm ----
    {
        HSM_Node_Param children[] = {
            { g_node_wait_key2,    NULL,                      entry_WaitKey2,    NULL, continuous_WaitKey2 },
            { g_node_go_cross,     NULL,                      entry_GoToCross,   NULL, continuous_GoToCross },
            { g_node_at_cross,     NULL,                      NULL,              NULL, NULL },
            { g_node_go_end,       NULL,                      entry_GoToEnd,     NULL, NULL },
            { g_node_wait_confirm, handler_WaitConfirm,       entry_WaitConfirm, NULL, NULL },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_outbound, children, 5)) {
            LOG_FATAL("[FSM] Register Outbound children failed!");
            return false;
        }
    }

    // ---- AtCross 的子节点: WaitTurnCmd, TurnLeft, TurnRight ----
    {
        HSM_Node_Param children[] = {
            { g_node_wait_turn_cmd, handler_WaitTurnCmd, entry_WaitTurnCmd, NULL, NULL },
            { g_node_turn_left,     NULL,                entry_TurnLeft,    exit_TurnLeft, continuous_TurnLeft },
            { g_node_turn_right,    NULL,                entry_TurnRight,   exit_TurnRight, continuous_TurnRight },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_at_cross, children, 3)) {
            LOG_FATAL("[FSM] Register AtCross children failed!");
            return false;
        }
    }

    // ---- Return 的子节点: TurnAround, Retrace ----
    {
        HSM_Node_Param children[] = {
            { g_node_turn_around, NULL, entry_TurnAround, exit_TurnAround, continuous_TurnAround },
            { g_node_retrace,     NULL, NULL,             NULL,            NULL },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_return_leg, children, 2)) {
            LOG_FATAL("[FSM] Register Return children failed!");
            return false;
        }
    }

    // ---- Retrace 的子节点: PopAction, ExecStraight, ExecTurn ----
    {
        HSM_Node_Param children[] = {
            { g_node_pop_action,    NULL, entry_PopAction,    NULL,          NULL },
            { g_node_exec_straight, NULL, entry_ExecStraight, NULL,          continuous_ExecStraight },
            { g_node_exec_turn,     NULL, entry_ExecTurn,     exit_ExecTurn, continuous_ExecTurn },
        };
        if (!HSM_RegisterChildNodes(g_app_hsm, g_node_retrace, children, 3)) {
            LOG_FATAL("[FSM] Register Retrace children failed!");
            return false;
        }
    }

    LOG_INFO("[FSM] State tree built successfully");
    return true;
}

// ============================================================
// 公共 API
// ============================================================
void App_FSM_Init(void)
{
    LOG_INFO("========== App FSM Init ==========");

    if (!build_state_tree()) {
        LOG_FATAL("[FSM] build_state_tree failed!");
        return;
    }

    ActionStack_INIT(&g_action_stack);

    // 启动状态机，初始状态 Idle
    HSM_Start(g_app_hsm, g_node_idle);
    LOG_INFO("[FSM] HSM started, initial state: Idle");
}

void App_FSM_Run(void)
{
    uint8_t cmd;

    // 1. 检查系统复位
    if (KeyControl_IsSystemReset()) {
        KeyControl_ClearSystemReset();
        KeyControl_ClearStartTrigger();
        LOG_INFO("[FSM] Key3 system reset triggered");
        HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_RESET});
    }

    // 2. 摄像头命令 → HSM 事件
    if (App_CmdDequeue_Cam(&cmd)) {
        LOG_INFO("[FSM] CAM command: 0x%02X", cmd);
        switch (cmd) {
            case 0x01:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_CAM_START});
                break;
            case 0x02:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_TURN_LEFT});
                break;
            case 0x03:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_TURN_RIGHT});
                break;
            case 0x04:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_DELIVERY_CONFIRM});
                break;
            default:
                LOG_WARN("[FSM] Unknown CAM command: 0x%02X", cmd);
                break;
        }
    }

    // 3. 蓝牙命令 → HSM 事件
    if (App_CmdDequeue(&cmd)) {
        LOG_INFO("[FSM] BT command: 0x%02X", cmd);
        switch (cmd) {
            case 0x01:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_BT_GO});
                break;
            case 0x02:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_TURN_LEFT});
                break;
            case 0x03:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_TURN_RIGHT});
                break;
            case 0x04:
                HSM_SendEvent(g_app_hsm, (HSM_Event_Package){.HSM_Event_ID = EV_DELIVERY_CONFIRM});
                break;
            default:
                LOG_WARN("[FSM] Unknown BT command: 0x%02X", cmd);
                break;
        }
    }

    // 4. 驱动 HSM（处理转换队列 + 执行 continuous_action）
    HSM_Process(g_app_hsm);
}

bool App_FSM_IsRunning(void)
{
    // 简单判断：不在 Idle 就是在执行任务
    // 注意：这里无法直接获取 HSM 当前状态，通过 control mode 间接判断
    return (ctrl_mode != CTRL_MODE_IDLE);
}
