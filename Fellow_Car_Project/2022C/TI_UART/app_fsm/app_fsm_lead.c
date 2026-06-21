/**
 * @file app_fsm_lead.c
 */

#include "app_fsm_lead.h"
#include "../fork_decide/task_config.h"
#include "../Control_Speed/Control_Speed.h"
#include "../app_protocol/app_protocol.h"

#define IS_LEAD true

static HSM*     g_hsm   = NULL;
static uint8_t  g_task  = 0;
static bool     g_running = false;
int g_black_count = 0;               // 全黑出现次数(跟随车也要读)

extern volatile int g_fork_timeout_ms;
extern volatile bool g_overtake_checking;
extern volatile bool g_is_overtaking;

static const char* line_name(uint8_t task)
{
    switch (task) {
        case 1: return "T1_LINE"; case 2: return "T2_LINE";
        case 3: return "T3_LINE"; case 4: return "T4_LINE";
    }
    return "Idle";
}

// ============================================================
// Root
// ============================================================
static bool handler_Root(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == 0x24) { // EV_BT_OVERTAKE_DONE
        g_is_overtaking = false; // 超车完成,允许恢复正常速度
        Control_SetBaseSpeed(TaskConfig_Get(g_task)->base_speed);
        Control_EnableDistance(!(g_black_count & 1)); // 偶数→开, 奇数→关
        return true;
    }
    if (e.HSM_Event_ID == EV_ALL_BLACK) {
        g_black_count++; // 两车共用全局,递增全黑次数,Task3查表决定走内圈/外圈
        if (g_black_count >= TaskConfig_Get(g_task)->finish_count) { // finish_count=2→1圈, =4→2圈, =6→3圈
            Control_Stop();
            Grayscale_SetMode(FORK_MODE_NORMAL);
            Control_SetBaseSpeed(0.0f);
            g_running = false;
            App_Protocol_SendFrame(CMD_BT_STOP, NULL, 0);
            HSM_RequestTransition(g_hsm, HSM_FindNode(g_hsm, "Idle"), e);
            return true;
        }
        return false;  // 不到终点→放行, 转移表 LINE→FORK 接管
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
    g_is_overtaking = false; // 复位超车标志
}

static bool handler_Idle(HSM_Event_Package e)
{
    if (e.HSM_Event_ID == EV_KEY_CONFIRM) {
        uint8_t task = HSM_ReadU8(e.data); // payload第1字节→task号
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
    g_fork_timeout_ms = TaskConfig_Get(g_task)->fork_timeout_ms;
    // g_overtake_checking不在此清零, 等ISR测到40cm才清
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

    // Task3: 走内圈=超车加速+关距离PID, 走外圈且非Lap1=被超等检测
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
// 转移表
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
