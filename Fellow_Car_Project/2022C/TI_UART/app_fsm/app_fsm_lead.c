/**
 * @file app_fsm_lead.c  (新 HSM API)
 *
 * 状态树 (全部平级在 Root 下，直接跳叶子节点):
 *   Root → Idle | T1_LINE | T1_WAIT | T1_FORK | ... | T4_FORK | Finish
 */

#include "app_fsm_lead.h"
#include "../fork_decide/fork_decide.h"
#include "../Control_Speed/Control_Speed.h"
#include "../Log/Log.h"

#define FORK_SPEED  0.12f
#define T1_SPEED    0.30f
#define T2_SPEED    0.50f
#define T3_SPEED    0.30f
#define T4_SPEED    1.00f

// ---- 全局 ----
static HSM*     g_hsm   = NULL;
static uint8_t  g_task  = 0;
static bool     g_running = false;

// ---- 辅助: 根据 task 找对应的 LINE/WAIT/FORK 节点名 ----
static const char* state_name(uint8_t task, const char* suffix)
{
    static char buf[16];
    // task=1~4, suffix="LINE"/"WAIT"/"FORK"
    // 简单做法：直接用 switch
    switch (task) {
        case 1:
            if (suffix[0]=='L') return "T1_LINE";
            if (suffix[0]=='W') return "T1_WAIT";
            return "T1_FORK";
        case 2:
            if (suffix[0]=='L') return "T2_LINE";
            if (suffix[0]=='W') return "T2_WAIT";
            return "T2_FORK";
        case 3:
            if (suffix[0]=='L') return "T3_LINE";
            if (suffix[0]=='W') return "T3_WAIT";
            return "T3_FORK";
        case 4:
            if (suffix[0]=='L') return "T4_LINE";
            if (suffix[0]=='W') return "T4_WAIT";
            return "T4_FORK";
    }
    return "Idle";
}

static float task_speed(uint8_t task)
{
    switch (task) { case 1: return T1_SPEED; case 2: return T2_SPEED;
                    case 3: return T3_SPEED; case 4: return T4_SPEED; }
    return 0.0f;
}

static void stop_motors(void)
{
    Control_Stop();
}

// ============================================================
// Root handler: 全局事件冒泡到这里
// ============================================================
static bool handler_Root(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_KEY_SWITCH || e.HSM_Event_ID == EV_FINISH) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Idle"), e);
        if (e.HSM_Event_ID == EV_FINISH) {
            stop_motors();
            Grayscale_SetMode(FORK_MODE_NORMAL);
            Control_SetBaseSpeed(0.0f);
            g_running = false;
        }
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
    stop_motors();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(0.0f);
    g_running = false;
    g_task = 0;
}

static bool handler_Idle(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_KEY_CONFIRM) {
        uint8_t task = HSM_ReadU8(e.data);
        if (task < 1 || task > 4) return false;
        g_task = task;
        g_running = true;
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, state_name(task, "LINE")), e);
        return true;
    }
    return false;
}

// ============================================================
// LINE_FOLLOW handlers (entry: 设速度+权值, handler: 全黑→WAIT)
// ============================================================
static void entry_LineFollow(HSM_Event_Package e)
{
    (void)e;
    Control_Start();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(task_speed(g_task));
}

static bool handler_LineFollow(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_ALL_BLACK) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, state_name(g_task, "WAIT")), e);
        return true;
    }
    return false;
}

// ============================================================
// WAIT_FORK handlers (entry: 减速, handler: 岔路出现→FORK)
// ============================================================
static void entry_WaitFork(HSM_Event_Package e)
{
    (void)e;
    Control_SetBaseSpeed(FORK_SPEED);
}

static bool handler_WaitFork(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_FORK_APPEAR) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, state_name(g_task, "FORK")), e);
        return true;
    }
    return false;
}

// ============================================================
// FORK handlers (entry: 偏置权值, handler: 通过→LINE)
// ============================================================
static void entry_Fork(HSM_Event_Package e)
{
    (void)e;
    Grayscale_SetMode(ForkDecide_GetMode(g_task));
}

static bool handler_Fork(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_FORK_PASSED) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, state_name(g_task, "LINE")), e);
        return true;
    }
    return false;
}

// ============================================================
// 状态表 (新 HSM API: 所有状态平级在 Root 下)
// ============================================================
static const HSM_StateDef lead_states[] = {
    HSM_STATE_DEF("Root",     NULL, handler_Root,      NULL,              NULL, NULL),
    HSM_STATE_DEF("Idle",     "Root", handler_Idle,    entry_Idle,        NULL, NULL),

    HSM_STATE_DEF("T1_LINE",  "Root", handler_LineFollow, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T1_WAIT",  "Root", handler_WaitFork,   entry_WaitFork,   NULL, NULL),
    HSM_STATE_DEF("T1_FORK",  "Root", handler_Fork,       entry_Fork,       NULL, NULL),

    HSM_STATE_DEF("T2_LINE",  "Root", handler_LineFollow, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T2_WAIT",  "Root", handler_WaitFork,   entry_WaitFork,   NULL, NULL),
    HSM_STATE_DEF("T2_FORK",  "Root", handler_Fork,       entry_Fork,       NULL, NULL),

    HSM_STATE_DEF("T3_LINE",  "Root", handler_LineFollow, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T3_WAIT",  "Root", handler_WaitFork,   entry_WaitFork,   NULL, NULL),
    HSM_STATE_DEF("T3_FORK",  "Root", handler_Fork,       entry_Fork,       NULL, NULL),

    HSM_STATE_DEF("T4_LINE",  "Root", handler_LineFollow, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T4_WAIT",  "Root", handler_WaitFork,   entry_WaitFork,   NULL, NULL),
    HSM_STATE_DEF("T4_FORK",  "Root", handler_Fork,       entry_Fork,       NULL, NULL),

    HSM_STATE_DEF("Finish",   "Root", NULL,              NULL,              NULL, NULL),
};
#define STATE_COUNT (sizeof(lead_states) / sizeof(lead_states[0]))

// ============================================================
// 创建 + 启动
// ============================================================
void Lead_FSM_Init(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node nodes[STATE_COUNT];

    g_hsm = HSM_Create(&mem, nodes, STATE_COUNT, lead_states, STATE_COUNT);
    HSM_Start(g_hsm, HSM_FindNode(g_hsm, "Idle"));
}

void Lead_FSM_Run(void)   { HSM_Process(g_hsm); }
HSM* Lead_FSM_GetHandle(void) { return g_hsm; }
uint8_t Lead_GetCurrentTask(void) { return g_task; }
bool   Lead_IsRunning(void)      { return g_running; }
