/**
 * @file app_fsm_follow.c
 */

#include "app_fsm_follow.h"
#include "app_fsm_lead.h"
#include "../fork_decide/task_config.h"
#include "../Control_Speed/Control_Speed.h"

#define IS_LEAD false

static HSM*     g_hsm      = NULL;
static uint8_t  g_task     = 0;
static bool     g_running  = false;

extern int g_black_count;         // 领头车Fsm里定义的, 跟随车共读
extern volatile int g_fork_timeout_ms;
extern volatile bool g_overtake_checking;
extern volatile bool g_is_overtaking;

static const char* follow_name(uint8_t task)
{
    switch (task) {
        case 1: return "T1_FOLLOW"; case 2: return "T2_FOLLOW";
        case 3: return "T3_FOLLOW"; case 4: return "T4_FOLLOW";
    }
    return "Idle";
}

// ============================================================
// Root
// ============================================================
static bool handler_Root(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_ALL_BLACK) {
        g_black_count++; // 两车共用,递增全黑次数,Task3查表决定走内圈/外圈
        return false;    // 放行给转移表 FOLLOW→FORK
    }
    if (e.HSM_Event_ID == EV_BT_STOP || e.HSM_Event_ID == EV_KEY_SWITCH) {
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Idle"), e);
        return true;
    }
    if (e.HSM_Event_ID == EV_BT_FORK) {
        uint8_t dir = HSM_ReadU8(e.data); // 0=外圈直行 1=内圈左转
        Grayscale_SetMode((dir == 1) ? FORK_MODE_LEFT : FORK_MODE_STRAIGHT);
        return true;
    }
    if (e.HSM_Event_ID == EV_BT_OVERTAKE_DONE) {
        g_is_overtaking = false; // 超车完成
        Control_SetBaseSpeed(TaskConfig_Get(g_task)->base_speed);
        Control_EnableDistance(g_black_count & 1); // 奇数=跟车开距离, 偶数=领头关距离
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
    g_black_count = 0;
    g_is_overtaking = false; // 复位超车标志
    Control_EnableDistance(false);
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
        HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, follow_name(task)), e);
        return true;
    }
    return false;
}

// ============================================================
// FOLLOW / FORK shared entry
// ============================================================
static void entry_Follow(HSM_Event_Package e)
{
    (void)e;
    g_fork_timeout_ms = TaskConfig_Get(g_task)->fork_timeout_ms;
    if (g_black_count == 0) Control_EnableDistance(true); // 首次出发才开
    Control_Start();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    if (!g_is_overtaking) Control_SetBaseSpeed(TaskConfig_Get(g_task)->base_speed); // 超车中不降速
}

static void entry_Fork(HSM_Event_Package e)
{
    (void)e;
    const TaskConfig* cfg = TaskConfig_Get(g_task);
    ForkMode mode = TaskConfig_GetForkMode(g_task, g_black_count, IS_LEAD);
    Grayscale_SetMode(mode);

    // Task3: 走内圈=超车加速+关距离PID, 走外圈=被超等40cm检测
    if (cfg->overtake_speed > 0 && mode == FORK_MODE_LEFT) {
        Control_SetBaseSpeed(cfg->overtake_speed);
        Control_EnableDistance(false);
        g_is_overtaking = true; // 超车中, FORK退出后不降速
    }
    if (cfg->overtake_speed > 0 && mode == FORK_MODE_STRAIGHT && g_black_count != 1) {
        g_overtake_checking = true; // Lap2/3被超车方, 等IR检测40cm
    }
}

// ============================================================
// Finish
// ============================================================
static void entry_Finish(HSM_Event_Package e)
{
    (void)e;
    Control_EnableDistance(false);
    Control_Stop();
    Grayscale_SetMode(FORK_MODE_NORMAL);
    Control_SetBaseSpeed(0.0f);
    g_running = false;
}

// ============================================================
// 状态表
// ============================================================
static const HSM_StateDef follow_states[] = {
    HSM_STATE_DEF("Root",      NULL, handler_Root, NULL,         NULL, NULL),
    HSM_STATE_DEF("Idle",      "Root", handler_Idle, entry_Idle, NULL, NULL),
    HSM_STATE_DEF("T1_FOLLOW", "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T1_FORK",   "Root", NULL, entry_Fork,         NULL, NULL),
    HSM_STATE_DEF("T2_FOLLOW", "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T2_FORK",   "Root", NULL, entry_Fork,         NULL, NULL),
    HSM_STATE_DEF("T3_FOLLOW", "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T3_FORK",   "Root", NULL, entry_Fork,         NULL, NULL),
    HSM_STATE_DEF("T4_FOLLOW", "Root", NULL, entry_Follow,       NULL, NULL),
    HSM_STATE_DEF("T4_FORK",   "Root", NULL, entry_Fork,         NULL, NULL),
    HSM_STATE_DEF("Finish",    "Root", NULL, entry_Finish,       NULL, NULL),
};
#define STATE_COUNT (sizeof(follow_states) / sizeof(follow_states[0]))

// ============================================================
// 转移表
// ============================================================
static HSM_Transition follow_trans[8];
static int follow_trans_count = 0;

static void add_trans(const char* from, uint8_t ev, const char* to)
{
    follow_trans[follow_trans_count++] = (HSM_Transition){
        HSM_FindNode(g_hsm, from), ev, HSM_FindNode(g_hsm, to), NULL
    };
}

static void register_transitions(void)
{
    follow_trans_count = 0;
    add_trans("T1_FOLLOW", EV_ALL_BLACK,   "T1_FORK");
    add_trans("T1_FORK",   EV_FORK_PASSED, "T1_FOLLOW");
    add_trans("T2_FOLLOW", EV_ALL_BLACK,   "T2_FORK");
    add_trans("T2_FORK",   EV_FORK_PASSED, "T2_FOLLOW");
    add_trans("T3_FOLLOW", EV_ALL_BLACK,   "T3_FORK");
    add_trans("T3_FORK",   EV_FORK_PASSED, "T3_FOLLOW");
    add_trans("T4_FOLLOW", EV_ALL_BLACK,   "T4_FORK");
    add_trans("T4_FORK",   EV_FORK_PASSED, "T4_FOLLOW");
    HSM_RegisterTransitions(g_hsm, follow_trans, follow_trans_count);
}

// ============================================================
// API
// ============================================================
void Follow_FSM_Init(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node nodes[STATE_COUNT];
    g_hsm = HSM_Create(&mem, nodes, STATE_COUNT, follow_states, STATE_COUNT);
    register_transitions();
    HSM_Start(g_hsm, HSM_FindNode(g_hsm, "Idle"));
}

void Follow_FSM_Run(void)              { HSM_Process(g_hsm); }
HSM* Follow_FSM_GetHandle(void)        { return g_hsm; }
bool Follow_IsRunning(void)            { return g_running; }
