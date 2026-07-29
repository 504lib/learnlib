#include "HSM_Core.h"
#include "HSM_Bus.h"
#include <cstdio>

// ============================================================
// 事件
// ============================================================
enum {
    EV_TICK      = 1,
    EV_START     = 2,
    EV_STOP      = 3,
    EV_LOW_POWER = 4,
    EV_CHARGED   = 5,
    EV_TX_DONE   = 6,
};

// ============================================================
// 1. Motion HSM — 4 状态
// ============================================================
static bool h_m_idle(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_START; }
static bool h_m_running(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_STOP || e.HSM_Event_ID == EV_LOW_POWER; }
static bool h_m_normal(HSM_Event_Package e)   { return e.HSM_Event_ID == EV_LOW_POWER; }
static bool h_m_safe(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_CHARGED; }

static void e_m_running(HSM_Event_Package) { printf("  [motion] 开始运行\n"); }
static void e_m_idle(HSM_Event_Package)    { printf("  [motion] 回到空闲\n"); }
static void e_m_safe(HSM_Event_Package)    { printf("  [motion] 安全停车\n"); }

static int  g_m_tick = 0;
static void tick_motion() { if (++g_m_tick % 3 == 0) printf("  [motion] 运行中 #%d\n", g_m_tick); }

static HSM* create_motion(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node        nodes[8];
    static HSM_StateDef    defs[] = {
        HSM_STATE_DEF("m_root",   NULL,        NULL,         NULL,         NULL,        NULL),
        HSM_STATE_DEF("m_idle",   "m_root",    h_m_idle,     e_m_idle,     NULL,        NULL),
        HSM_STATE_DEF("m_running","m_idle",    h_m_running,  e_m_running,  NULL,        NULL),
        HSM_STATE_DEF("m_normal", "m_running", h_m_normal,   NULL,         NULL,        tick_motion),
        HSM_STATE_DEF("m_safe",   "m_running", h_m_safe,     e_m_safe,     NULL,        NULL),
    };
    return HSM_Create(&mem, nodes, 8, defs, 5);
}

// ============================================================
// 2. Radio HSM — 3 状态
// ============================================================
static bool h_r_off(HSM_Event_Package e) { return e.HSM_Event_ID == EV_START; }
static bool h_r_tx(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_TX_DONE || e.HSM_Event_ID == EV_STOP; }
static bool h_r_rx(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_STOP; }

static void e_r_tx(HSM_Event_Package) { printf("  [radio]  开始发射\n"); }
static void e_r_rx(HSM_Event_Package) { printf("  [radio]  开始接收\n"); }
static void e_r_off(HSM_Event_Package){ printf("  [radio]  关闭\n"); }

static HSM* create_radio(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node        nodes[8];
    static HSM_StateDef    defs[] = {
        HSM_STATE_DEF("r_root", NULL,      NULL,     NULL,      NULL, NULL),
        HSM_STATE_DEF("r_off",  "r_root",  h_r_off,  e_r_off,   NULL, NULL),
        HSM_STATE_DEF("r_tx",   "r_off",   h_r_tx,   e_r_tx,    NULL, NULL),
        HSM_STATE_DEF("r_rx",   "r_off",   h_r_rx,   e_r_rx,    NULL, NULL),
    };
    return HSM_Create(&mem, nodes, 8, defs, 4);
}

// ============================================================
// 3. Power HSM — 3 状态
// ============================================================
static bool h_p_normal(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_LOW_POWER; }
static bool h_p_low(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_CHARGED || e.HSM_Event_ID == EV_START; }
static bool h_p_charging(HSM_Event_Package e){ return e.HSM_Event_ID == EV_CHARGED; }

static void e_p_low(HSM_Event_Package)     { printf("  [power]  电量低！\n"); }
static void e_p_charging(HSM_Event_Package){ printf("  [power]  开始充电\n"); }
static void e_p_normal(HSM_Event_Package)  { printf("  [power]  电量正常\n"); }

static HSM* create_power(void)
{
    static HSM_Core_Memory mem;
    static HSM_Node        nodes[8];
    static HSM_StateDef    defs[] = {
        HSM_STATE_DEF("p_root",    NULL,       NULL,        NULL,         NULL, NULL),
        HSM_STATE_DEF("p_normal",  "p_root",   h_p_normal,  e_p_normal,   NULL, NULL),
        HSM_STATE_DEF("p_low",     "p_root",   h_p_low,     e_p_low,      NULL, NULL),
        HSM_STATE_DEF("p_charging","p_low",    h_p_charging, e_p_charging, NULL, NULL),
    };
    return HSM_Create(&mem, nodes, 8, defs, 4);
}

// ============================================================
// 辅助
// ============================================================
static void section(const char* title) { printf("\n--- %s ---\n\n", title); }

// ============================================================
int main()
{
    printf("========================================\n");
    printf("  HSM Bus Demo — 运动 × 通讯 × 电量\n");
    printf("  3 个独立 HSM, 共 10 状态 (不是 36)\n");
    printf("========================================\n");

    HSM* motion = create_motion();
    HSM* radio  = create_radio();
    HSM* power  = create_power();

    HSM_Bus bus = {0};
    HSM_Bus_Add(&bus, motion);
    HSM_Bus_Add(&bus, radio);
    HSM_Bus_Add(&bus, power);

    // 各自启动
    HSM_Start(motion, HSM_FindNode(motion, "m_idle"));
    HSM_Start(radio,  HSM_FindNode(radio,  "r_off"));
    HSM_Start(power,  HSM_FindNode(power,  "p_normal"));

    // --- 场景 1: 开始运行 ---
    section("启动: 所有收到 START → radio 发射, motion 运行");
    HSM_Bus_SendEvent(&bus, (HSM_Event_Package){ .HSM_Event_ID = EV_START });
    HSM_RequestTransition(motion, HSM_FindNode(motion, "m_normal"), (HSM_Event_Package){ EV_START });
    HSM_RequestTransition(radio,  HSM_FindNode(radio,  "r_tx"),    (HSM_Event_Package){ EV_START });
    HSM_Bus_Process(&bus);

    // --- 场景 2: 几个 tick ---
    section("持续运行 3 ticks — 只有 motion.normal 有 continuous");
    for (int i = 0; i < 3; i++) {
        HSM_Bus_SendEvent(&bus, (HSM_Event_Package){ .HSM_Event_ID = EV_TICK });
        HSM_Bus_Process(&bus);
    }

    // --- 场景 3: 电量低 → 跨维度协调 ---
    section("电池低电量 → motion 触发避障, power 切低功耗");
    HSM_Bus_SendEvent(&bus, (HSM_Event_Package){ .HSM_Event_ID = EV_LOW_POWER });
    HSM_RequestTransition(motion, HSM_FindNode(motion, "m_safe"), (HSM_Event_Package){ EV_LOW_POWER });
    HSM_RequestTransition(power,  HSM_FindNode(power,  "p_low"),  (HSM_Event_Package){ EV_LOW_POWER });
    HSM_Bus_Process(&bus);

    // --- 场景 4: 开始充电 ---
    section("进入充电 — power.low → power.charging");
    HSM_SendEvent(power, (HSM_Event_Package){ .HSM_Event_ID = EV_START });
    HSM_RequestTransition(power, HSM_FindNode(power, "p_charging"), (HSM_Event_Package){ EV_START });
    HSM_Bus_Process(&bus);

    // --- 场景 5: 电池充满, 恢复正常 ---
    section("电池充满 → power 正常, motion 恢复正常");
    HSM_SendEvent(power, (HSM_Event_Package){ .HSM_Event_ID = EV_CHARGED });
    HSM_RequestTransition(power,  HSM_FindNode(power,  "p_normal"), (HSM_Event_Package){ EV_CHARGED });
    HSM_RequestTransition(motion, HSM_FindNode(motion, "m_idle"),   (HSM_Event_Package){ EV_CHARGED });
    HSM_Bus_Process(&bus);

    // --- 场景 6: 停止 ---
    section("关闭: STOP → 全部回初始态");
    HSM_Bus_SendEvent(&bus, (HSM_Event_Package){ .HSM_Event_ID = EV_STOP });
    HSM_RequestTransition(motion, HSM_FindNode(motion, "m_idle"), (HSM_Event_Package){ EV_STOP });
    HSM_RequestTransition(radio,  HSM_FindNode(radio,  "r_off"),  (HSM_Event_Package){ EV_STOP });
    HSM_Bus_Process(&bus);

    // --- 结束 ---
    section("销毁");
    HSM_Destroy(motion);
    HSM_Destroy(radio);
    HSM_Destroy(power);
    printf("  全部销毁\n\n========================================\n");
    printf("  Bus Demo 结束\n");
    printf("========================================\n");
    return 0;
}
