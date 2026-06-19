/**
 * @file app_fsm_lead.c (新 HSM + 转移表)
 *
 * handler 只处理需要根据条件选目标的（Idle, Root），
 * 岔路流转全部用 HSM_RegisterTransitions 转移表。
 */

#include "app_fsm_lead.h"
#include "../fork_decide/fork_decide.h"
#include "../Control_Speed/Control_Speed.h"

#define FORK_SPEED  0.12f
#define T1_SPEED    0.30f
#define T2_SPEED    0.50f
#define T3_SPEED    0.30f
#define T4_SPEED    1.00f

static HSM*     g_hsm   = NULL;
static uint8_t  g_task  = 0;
static bool     g_running = false;

// ---- 辅助 ----
static const char* state_name(uint8_t task, const char* suffix)
{
    switch (task) {
        case 1: if (suffix[0]=='L') return "T1_LINE";
                if (suffix[0]=='W') return "T1_WAIT"; return "T1_FORK";
        case 2: if (suffix[0]=='L') return "T2_LINE";
                if (suffix[0]=='W') return "T2_WAIT"; return "T2_FORK";
        case 3: if (suffix[0]=='L') return "T3_LINE";
                if (suffix[0]=='W') return "T3_WAIT"; return "T3_FORK";
        case 4: if (suffix[0]=='L') return "T4_LINE";
                if (suffix[0]=='W') return "T4_WAIT"; return "T4_FORK";
    }
    return "Idle";
}

static float task_speed(uint8_t task)
{
    switch (task) { case 1: return T1_SPEED; case 2: return T2_SPEED;
                    case 3: return T3_SPEED; case 4: return T4_SPEED; }
    return 0.0f;
}

// ============================================================
// Root: 全局事件
// ============================================================
static bool handler_Root(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_KEY_SWITCH || e.HSM_Event_ID == EV_FINISH) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Idle"), e);
        if (e.HSM_Event_ID == EV_FINISH) {
            Control_Stop();
            Grayscale_SetMode(FORK_MODE_NORMAL);
            Control_SetBaseSpeed(0.0f);
            g_running = false;
        }
        return true;
    }
    return false;  // 不处理 → 冒泡给转移表
}

// ============================================================
// Idle: 只处理按键确认
// ============================================================
static void entry_Idle(HSM_Event_Package e)
{
    (void)e;
    Control_Stop();
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
// 共享 entry：LINE / WAIT / FORK（用 g_task 区分速度 / 权值）
// ============================================================
static void entry_LineFollow(HSM_Event_Package e)
{
    (void)e;
    Control_Start();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(task_speed(g_task));
}

static void entry_WaitFork(HSM_Event_Package e)
{
    (void)e;
    Control_SetBaseSpeed(FORK_SPEED);
}

static void entry_Fork(HSM_Event_Package e)
{
    (void)e;
    Grayscale_SetMode(ForkDecide_GetMode(g_task));
}

// ============================================================
// 状态表（全部平级在 Root 下）
// ============================================================
static const HSM_StateDef lead_states[] = {
    HSM_STATE_DEF("Root",     NULL, handler_Root, NULL,              NULL, NULL),
    HSM_STATE_DEF("Idle",     "Root", handler_Idle, entry_Idle,      NULL, NULL),

    HSM_STATE_DEF("T1_LINE",  "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T1_WAIT",  "Root", NULL, entry_WaitFork,    NULL, NULL),
    HSM_STATE_DEF("T1_FORK",  "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T2_LINE",  "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T2_WAIT",  "Root", NULL, entry_WaitFork,    NULL, NULL),
    HSM_STATE_DEF("T2_FORK",  "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T3_LINE",  "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T3_WAIT",  "Root", NULL, entry_WaitFork,    NULL, NULL),
    HSM_STATE_DEF("T3_FORK",  "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T4_LINE",  "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T4_WAIT",  "Root", NULL, entry_WaitFork,    NULL, NULL),
    HSM_STATE_DEF("T4_FORK",  "Root", NULL, entry_Fork,        NULL, NULL),
};
#define STATE_COUNT (sizeof(lead_states) / sizeof(lead_states[0]))

// ============================================================
// 转移表：岔路流转全在这里，无需 handler
// ============================================================
#define T(_from, _ev, _to)  { HSM_FindNode(g_hsm, _from), _ev, HSM_FindNode(g_hsm, _to), NULL }

static void register_transitions(void)
{
    const HSM_Transition trans[] = {
        // T1: LINE →(全黑)→ WAIT →(岔路)→ FORK →(通过)→ LINE
        T("T1_LINE", EV_ALL_BLACK,   "T1_WAIT"),
        T("T1_WAIT", EV_FORK_APPEAR, "T1_FORK"),
        T("T1_FORK", EV_FORK_PASSED, "T1_LINE"),
        // T2
        T("T2_LINE", EV_ALL_BLACK,   "T2_WAIT"),
        T("T2_WAIT", EV_FORK_APPEAR, "T2_FORK"),
        T("T2_FORK", EV_FORK_PASSED, "T2_LINE"),
        // T3
        T("T3_LINE", EV_ALL_BLACK,   "T3_WAIT"),
        T("T3_WAIT", EV_FORK_APPEAR, "T3_FORK"),
        T("T3_FORK", EV_FORK_PASSED, "T3_LINE"),
        // T4
        T("T4_LINE", EV_ALL_BLACK,   "T4_WAIT"),
        T("T4_WAIT", EV_FORK_APPEAR, "T4_FORK"),
        T("T4_FORK", EV_FORK_PASSED, "T4_LINE"),
    };
    HSM_RegisterTransitions(g_hsm, trans, sizeof(trans)/sizeof(trans[0]));
}
#undef T

// ============================================================
// 创建 + 启动
// ============================================================
void Lead_FSM_Init(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node nodes[STATE_COUNT];
    g_hsm = HSM_Create(&mem, nodes, STATE_COUNT, lead_states, STATE_COUNT);
    register_transitions();
    HSM_Start(g_hsm, HSM_FindNode(g_hsm, "Idle"));
}

void Lead_FSM_Run(void)           { HSM_Process(g_hsm); }
HSM* Lead_FSM_GetHandle(void)     { return g_hsm; }
uint8_t Lead_GetCurrentTask(void) { return g_task; }
bool   Lead_IsRunning(void)       { return g_running; }
