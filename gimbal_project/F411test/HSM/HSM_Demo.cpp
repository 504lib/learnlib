#include "HSM_Core.h"
#include <windows.h>
#include <cstdio>

// ============================================================
// 事件
// ============================================================
enum EventID : uint8_t {
    EV_START    = 1,
    EV_STOP     = 2,
    EV_CHARGE   = 3,
    EV_OBSTACLE = 4,
    EV_CLEAR    = 5,
};

// ============================================================
// 全局
// ============================================================
static HSM_Node* g_idle      = nullptr;
static HSM_Node* g_running   = nullptr;
static HSM_Node* g_normal    = nullptr;
static HSM_Node* g_avoiding  = nullptr;
static HSM_Node* g_charging  = nullptr;
static HSM*      g_hsm       = nullptr;

// ============================================================
// entry / exit / continuous
// ============================================================
static void entry_idle(HSM_Event_Package)       { printf("  [entry] 空闲状态\n"); }
static void entry_running(HSM_Event_Package)    { printf("  [entry] 开始运行\n"); }
static void entry_normal(HSM_Event_Package)     { printf("  [entry] 正常行驶\n"); }
static void entry_avoiding(HSM_Event_Package)   { printf("  [entry] 进入避障模式\n"); }
static void exit_avoiding(HSM_Event_Package)    { printf("  [exit]  退出避障模式\n"); }
static void entry_charging(HSM_Event_Package)   { printf("  [entry] 开始充电\n"); }
static void exit_charging(HSM_Event_Package)    { printf("  [exit]  停止充电\n"); }

static int g_heartbeat = 0;
static void continuous_heartbeat()              { printf("  [tick]  心跳 #%d\n", ++g_heartbeat); }

// ============================================================
// handler
// ============================================================
static bool handler_idle(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_START; }
static bool handler_running(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_STOP || e.HSM_Event_ID == EV_OBSTACLE; }
static bool handler_normal(HSM_Event_Package e)   { return e.HSM_Event_ID == EV_OBSTACLE; }
static bool handler_avoiding(HSM_Event_Package e) { return e.HSM_Event_ID == EV_CLEAR || e.HSM_Event_ID == EV_OBSTACLE; }
static bool handler_charging(HSM_Event_Package e) { return e.HSM_Event_ID == EV_START; }
static bool null_handler(HSM_Event_Package)       { return false; }

// ============================================================
// 辅助
// ============================================================
static void section(const char* title) { printf("\n--- %s ---\n\n", title); }

static void sleep_and_process(int ticks)
{
    for (int i = 0; i < ticks; i++) {
        HSM_Process(g_hsm);
        Sleep(100);
    }
}

static void step(const char* desc, HSM_Event_Package ev, HSM_Node* target)
{
    printf("%s\n", desc);
    HSM_SendEvent(g_hsm, ev);                       // 冒泡触发 handler
    if (target) {
        HSM_RequestTransition(g_hsm, target, ev);   // 入队转换
        printf("  -> RequestTransition\n");
    }
    // 转换在下次 Process/SendEvent 中执行, 这里立刻 Process 执行掉
    HSM_Process(g_hsm);
}

// ============================================================
// 构建状态树 (新 API — 无需手动操作字段)
//
//   Root
//   ├── Idle ──(sibling)── Charging
//   └── (Idle.child) Running
//       ├── Normal ──(sibling)── Avoiding
//       └── ...
// ============================================================
static bool build_state_tree()
{
    static FMS_OBJECT_MEMORY obj_mem;
    static FMS_NODE_MEMORY  node_mem[7];

    g_hsm = HSM_Create(&obj_mem);
    if (!g_hsm) return false;

    HSM_Node* root      = HSM_Node_Create(&node_mem[0]);
    g_idle              = HSM_Node_Create(&node_mem[1]);
    g_charging          = HSM_Node_Create(&node_mem[2]);
    g_running           = HSM_Node_Create(&node_mem[3]);
    g_normal            = HSM_Node_Create(&node_mem[4]);
    g_avoiding          = HSM_Node_Create(&node_mem[5]);

    // Root 是结构节点, 在第一次 RegisterChild 时自动初始化
    // Idle 和 Charging 是 Root 的两个子节点 (兄弟)
    HSM_Node_Param root_children[] = {
        { g_idle,     handler_idle,     entry_idle,     nullptr,        continuous_heartbeat },
        { g_charging, handler_charging, entry_charging, exit_charging,  nullptr },
    };
    if (!HSM_RegisterChildNodes(g_hsm, root, root_children, 2)) return false;

    // Running 是 Idle 的子节点
    HSM_Node_Param running_param = { g_running, handler_running, entry_running, nullptr, nullptr };
    if (!HSM_RegisterChild(g_hsm, g_idle, running_param)) return false;

    // Normal 和 Avoiding 是 Running 的两个子节点 (兄弟)
    HSM_Node_Param running_children[] = {
        { g_normal,   handler_normal,   entry_normal,   nullptr,        continuous_heartbeat },
        { g_avoiding, handler_avoiding, entry_avoiding, exit_avoiding,  nullptr },
    };
    if (!HSM_RegisterChildNodes(g_hsm, g_running, running_children, 2)) return false;

    return true;
}

// ============================================================
int main()
{
    SetConsoleOutputCP(65001);
    printf("========================================\n");
    printf("  HSM Demo - 送药小车\n");
    printf("========================================\n");

    if (!build_state_tree()) {
        printf("FATAL: build_state_tree failed!\n");
        return 1;
    }

    // --- 场景 1: 启动 → Idle ---
    section("启动系统进入空闲状态");
    HSM_Start(g_hsm, g_idle);
    sleep_and_process(2);

    // --- 场景 2: Idle → Normal ---
    section("收到 START, 开始运行");
    step(">>> EV_START", { EV_START }, g_normal);
    sleep_and_process(2);

    // --- 场景 3: Normal → Avoiding ---
    section("检测到障碍物, 避障");
    step(">>> EV_OBSTACLE", { EV_OBSTACLE }, g_avoiding);
    sleep_and_process(2);

    // --- 场景 4: Avoiding → Normal ---
    section("障碍清除, 恢复正常");
    step(">>> EV_CLEAR", { EV_CLEAR }, g_normal);
    sleep_and_process(2);

    // --- 场景 5: 直接切 Charging ---
    section("电量低, 直接切充电");
    printf(">>> RequestTransition -> Charging\n");
    HSM_RequestTransition(g_hsm, g_charging, { EV_CHARGE });
    HSM_Process(g_hsm);  // 立即执行
    sleep_and_process(2);

    // --- 场景 6: Charging → Idle ---
    section("充电完成, 回到空闲");
    step(">>> EV_START", { EV_START }, g_idle);
    sleep_and_process(2);

    // --- 结束 ---
    section("销毁");
    HSM_Destroy(g_hsm);
    printf("  状态机已销毁\n");
    printf("\n========================================\n");
    printf("  演示结束\n");
    printf("========================================\n");
    return 0;
}
