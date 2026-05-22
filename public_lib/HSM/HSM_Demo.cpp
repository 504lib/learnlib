#include "HSM_Core.h"
#include <windows.h>
#include <cstdio>

// ============================================================
// 事件定义
// ============================================================
enum EventID : uint8_t {
    EV_START    = 1,
    EV_STOP     = 2,
    EV_CHARGE   = 3,
    EV_OBSTACLE = 4,
    EV_CLEAR    = 5,
};

// ============================================================
// 全局节点指针
// ============================================================
static HSM_Node* g_idle      = nullptr;
static HSM_Node* g_running   = nullptr;
static HSM_Node* g_normal    = nullptr;
static HSM_Node* g_avoiding  = nullptr;
static HSM_Node* g_charging  = nullptr;
static HSM*      g_hsm       = nullptr;

// ============================================================
// entry / exit / continuous 动作
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

// ============================================================
// 构建状态树
//
//   Root (结构节点, 不作为 active state)
//   ├── Idle ──(sibling)── Charging
//   └── Running (Idle 的 first_child)
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
    g_running           = HSM_Node_Create(&node_mem[2]);
    g_normal            = HSM_Node_Create(&node_mem[3]);
    g_avoiding          = HSM_Node_Create(&node_mem[4]);
    g_charging          = HSM_Node_Create(&node_mem[5]);

    // 1) 初始化 Root (纯结构节点)
    HSM_Node_Param rp = { root, null_handler, nullptr, nullptr, nullptr };
    if (!HSM_RegisterChildNodes(g_hsm, &rp, 1)) return false;

    // 2) 父子链: Idle → Running → Normal
    HSM_Node_Param chain[] = {
        { g_idle,    handler_idle,    entry_idle,     nullptr,        continuous_heartbeat },
        { g_running, handler_running, entry_running,  nullptr,        nullptr },
        { g_normal,  handler_normal,  entry_normal,   nullptr,        continuous_heartbeat },
    };
    if (!HSM_RegisterChildNodes(g_hsm, chain, 3)) return false;

    // 3) 初始化 Charging 和 Avoiding
    HSM_Node_Param cp = { g_charging, handler_charging, entry_charging, exit_charging, nullptr };
    HSM_Node_Param ap = { g_avoiding, handler_avoiding, entry_avoiding, exit_avoiding, nullptr };
    if (!HSM_RegisterChildNodes(g_hsm, &cp, 1)) return false;
    if (!HSM_RegisterChildNodes(g_hsm, &ap, 1)) return false;

    // 4) 手动建立树结构 (API 目前只支持链式注册, 复杂树需要手动补关系)
    root->first_child = g_idle;
    g_idle->parent = root;
    g_idle->next_sibling = g_charging;
    g_charging->parent = root;

    g_normal->next_sibling = g_avoiding;
    g_avoiding->parent = g_running;

    return true;
}

// ============================================================
// 场景步骤
// ============================================================
static void step(const char* desc, HSM_Event_Package ev, HSM_Node* target)
{
    printf("%s\n", desc);
    HSM_SendEvent(g_hsm, ev);
    if (target) {
        printf("  -> Transition\n");
        HSM_Transition(g_hsm, target, ev);
    }
}

// ============================================================
int main()
{
    SetConsoleOutputCP(65001);  // UTF-8
    printf("========================================\n");
    printf("  HSM Demo - 送药小车\n");
    printf("========================================\n");

    if (!build_state_tree()) {
        printf("FATAL: build_state_tree failed!\n");
        return 1;
    }

    // --- 场景 1: 启动, 进入 Idle ---
    section("启动系统进入空闲状态");
    HSM_Start(g_hsm, g_idle);
    for (int i = 0; i < 2; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 场景 2: Idle → Running/Normal ---
    section("收到 START, 开始运行");
    step(">>> EV_START", { EV_START }, g_normal);
    for (int i = 0; i < 3; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 场景 3: Normal → Avoiding ---
    section("检测到障碍物, 避障");
    step(">>> EV_OBSTACLE", { EV_OBSTACLE }, g_avoiding);
    for (int i = 0; i < 2; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 场景 4: Avoiding → Normal ---
    section("障碍清除, 恢复正常");
    step(">>> EV_CLEAR", { EV_CLEAR }, g_normal);
    for (int i = 0; i < 2; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 场景 5: 直接 Transition 到 Charging ---
    section("电量低, 直接切充电 (不依赖 handler)");
    printf(">>> Transition -> Charging\n");
    HSM_Transition(g_hsm, g_charging, { EV_CHARGE });
    for (int i = 0; i < 2; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 场景 6: Charging → Idle ---
    section("充电完成, 回到空闲");
    step(">>> EV_START", { EV_START }, g_idle);
    for (int i = 0; i < 2; i++) { HSM_Process(g_hsm); Sleep(100); }

    // --- 结束 ---
    section("销毁");
    HSM_Destroy(g_hsm);
    printf("  状态机已销毁\n");
    printf("\n========================================\n");
    printf("  演示结束\n");
    printf("========================================\n");
    return 0;
}
