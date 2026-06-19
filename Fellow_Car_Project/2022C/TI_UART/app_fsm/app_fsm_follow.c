/**
 * @file app_fsm_follow.c (新 HSM + 转移表)
 *
 * 跟随车：蓝牙命令驱动。Root 处理 BT_STOP / BT_FORK，Idle 处理 BT_GO。
 */

#include "app_fsm_follow.h"
#include "../fork_decide/fork_decide.h"
#include "../Control_Speed/Control_Speed.h"

#define T1_SPEED  0.30f
#define T2_SPEED  0.50f
#define T3_SPEED  0.30f
#define T4_SPEED  1.00f

static HSM*     g_hsm      = NULL;
static uint8_t  g_task     = 0;
static bool     g_running  = false;

static float task_speed(uint8_t task)
{
    switch (task) { case 1: return T1_SPEED; case 2: return T2_SPEED;
                    case 3: return T3_SPEED; case 4: return T4_SPEED; }
    return 0.0f;
}

// ============================================================
// Root: BT_STOP → Finish,  BT_FORK → 只切权值不换状态
// ============================================================
static bool handler_Root(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_BT_STOP) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Finish"), e);
        return true;
    }
    if (e.HSM_Event_ID == EV_BT_FORK) {
        uint8_t dir = HSM_ReadU8(e.data);
        Grayscale_SetMode((dir == 1) ? FORK_MODE_LEFT : FORK_MODE_STRAIGHT);
        return true;
    }
    return false;
}

// ============================================================
// Idle
// ============================================================
static void entry_Idle(HSM_Event_Package e)
{
    (void)e;
    Control_Stop();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(0.0f);
    g_running = false;
}

static bool handler_Idle(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_BT_GO) {
        uint8_t task = HSM_ReadU8(e.data);
        if (task < 1 || task > 4) return false;
        g_task = task;
        g_running = true;
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm,
            task==1?"T1_FOLLOW":task==2?"T2_FOLLOW":task==3?"T3_FOLLOW":"T4_FOLLOW"), e);
        return true;
    }
    return false;
}

// ============================================================
// FOLLOW 共享 entry
// ============================================================
static void entry_Follow(HSM_Event_Package e)
{
    (void)e;
    Control_Start();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(task_speed(g_task));
}

// ============================================================
// Finish
// ============================================================
static void entry_Finish(HSM_Event_Package e)
{
    (void)e;
    Control_Stop();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(0.0f);
    g_running = false;
}

// ============================================================
// 状态表
// ============================================================
static const HSM_StateDef follow_states[] = {
    HSM_STATE_DEF("Root",       NULL, handler_Root, NULL,         NULL, NULL),
    HSM_STATE_DEF("Idle",       "Root", handler_Idle, entry_Idle, NULL, NULL),
    HSM_STATE_DEF("T1_FOLLOW",  "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T2_FOLLOW",  "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T3_FOLLOW",  "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T4_FOLLOW",  "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("Finish",     "Root", NULL, entry_Finish,       NULL, NULL),
};
#define STATE_COUNT (sizeof(follow_states) / sizeof(follow_states[0]))

// ============================================================
// API
// ============================================================
void Follow_FSM_Init(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node nodes[STATE_COUNT];
    g_hsm = HSM_Create(&mem, nodes, STATE_COUNT, follow_states, STATE_COUNT);
    HSM_Start(g_hsm, HSM_FindNode(g_hsm, "Idle"));
}

void Follow_FSM_Run(void)              { HSM_Process(g_hsm); }
HSM* Follow_FSM_GetHandle(void)        { return g_hsm; }
bool Follow_IsRunning(void)            { return g_running; }
