/**
 * @file app_fsm_lead.c
 *
 * 简化：全黑线 → 直接进 FORK（偏置权值），2秒超时 → 回 LINE
 */

#include "app_fsm_lead.h"
#include "../fork_decide/fork_decide.h"
#include "../Control_Speed/Control_Speed.h"
#include "../app_protocol/app_protocol.h"

#define T1_SPEED  0.30f
#define T2_SPEED  0.50f
#define T3_SPEED  0.30f
#define T4_SPEED  1.00f

static HSM*     g_hsm   = NULL;
static uint8_t  g_task  = 0;
static bool     g_running = false;
static int      g_black_count = 0;  // 全黑线出现次数

static const char* line_name(uint8_t task)
{
    switch (task) {
        case 1: return "T1_LINE"; case 2: return "T2_LINE";
        case 3: return "T3_LINE"; case 4: return "T4_LINE";
    }
    return "Idle";
}

static const char* fork_name(uint8_t task)
{
    switch (task) {
        case 1: return "T1_FORK"; case 2: return "T2_FORK";
        case 3: return "T3_FORK"; case 4: return "T4_FORK";
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
    if (e.HSM_Event_ID == EV_ALL_BLACK) {
        g_black_count++;
        if (g_black_count >= 2) {
            // 第2次全黑 → 终点停车
            Control_Stop();
            Grayscale_SetMode(FORK_MODE_NORMAL);
            Control_SetBaseSpeed(0.0f);
            g_running = false;
            App_Protocol_SendFrame(CMD_BT_STOP, NULL, 0);  // 通知跟随车
            HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Idle"), e);
            return true;
        }
        return false;  // 第1次 → 冒泡给转移表（LINE→FORK）
    }
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
    g_task = 0;
    g_black_count = 0;
}

static bool handler_Idle(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_KEY_CONFIRM) {
        uint8_t task = HSM_ReadU8(e.data);
        if (task < 1 || task > 4) return false;
        g_task = task;
        g_running = true;
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, line_name(task)), e);
        return true;
    }
    return false;
}

// ============================================================
// 共享 entry
// ============================================================
static void entry_LineFollow(HSM_Event_Package e)
{
    (void)e;
    Control_Start();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(task_speed(g_task));
}

static void entry_Fork(HSM_Event_Package e)
{
    (void)e;
    Grayscale_SetMode(ForkDecide_GetMode(g_task));
}

// ============================================================
// 状态表
// ============================================================
static const HSM_StateDef lead_states[] = {
    HSM_STATE_DEF("Root",    NULL, handler_Root, NULL,              NULL, NULL),
    HSM_STATE_DEF("Idle",    "Root", handler_Idle, entry_Idle,      NULL, NULL),

    HSM_STATE_DEF("T1_LINE", "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T1_FORK", "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T2_LINE", "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T2_FORK", "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T3_LINE", "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T3_FORK", "Root", NULL, entry_Fork,        NULL, NULL),

    HSM_STATE_DEF("T4_LINE", "Root", NULL, entry_LineFollow, NULL, NULL),
    HSM_STATE_DEF("T4_FORK", "Root", NULL, entry_Fork,        NULL, NULL),
};
#define STATE_COUNT (sizeof(lead_states) / sizeof(lead_states[0]))

// ============================================================
// 转移表：LINE →(全黑)→ FORK →(2秒超时)→ LINE
// ============================================================
static HSM_Transition lead_trans[8];
static int lead_trans_count = 0;

static void add_trans(const char* from, uint8_t ev, const char* to)
{
    lead_trans[lead_trans_count++] = (HSM_Transition){
        HSM_FindNode(g_hsm, from), ev, HSM_FindNode(g_hsm, to), NULL
    };
}

static void register_transitions(void)
{
    lead_trans_count = 0;
    add_trans("T1_LINE", EV_ALL_BLACK,   "T1_FORK");
    add_trans("T1_FORK", EV_FORK_PASSED, "T1_LINE");
    add_trans("T2_LINE", EV_ALL_BLACK,   "T2_FORK");
    add_trans("T2_FORK", EV_FORK_PASSED, "T2_LINE");
    add_trans("T3_LINE", EV_ALL_BLACK,   "T3_FORK");
    add_trans("T3_FORK", EV_FORK_PASSED, "T3_LINE");
    add_trans("T4_LINE", EV_ALL_BLACK,   "T4_FORK");
    add_trans("T4_FORK", EV_FORK_PASSED, "T4_LINE");
    HSM_RegisterTransitions(g_hsm, lead_trans, lead_trans_count);
}

// ============================================================
// API
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
