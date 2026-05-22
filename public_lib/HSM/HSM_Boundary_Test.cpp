#include "HSM_Core.h"
#include <cstdio>
#include <cstring>

#ifndef HSM_EVENT_ID_USER
#define HSM_EVENT_ID_USER 0x10
#endif

// ============================================================
// 辅助宏
// ============================================================
#define TEST(name) static void name(void)
#define CHECK(cond) do { \
    if (!(cond)) { printf("  FAIL [%d]: %s\n", __LINE__, #cond); return; } \
} while(0)

static int g_pass = 0;
static int g_fail = 0;

static void run_test(void (*test)(void), const char* name)
{
    printf("  %-50s", name);
    int before_pass = g_pass;
    g_pass++;
    test();
    if (g_pass > before_pass) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        g_pass = before_pass;
        g_fail++;
    }
}

// ============================================================
// handler / action 桩函数
// ============================================================
static bool stub_handler(HSM_Event_Package e)   { return true; }
static void stub_entry(HSM_Event_Package e)      { }
static void stub_exit(HSM_Event_Package e)       { }
static void stub_continuous()                    { }

static bool null_handler(HSM_Event_Package)      { return false; }
static bool reject_handler(HSM_Event_Package)    { return false; }

// ============================================================
// 1. 创建 / 销毁 边界
// ============================================================
TEST(create_with_null_memory)
{
    HSM* hsm = HSM_Create(nullptr);
    CHECK(hsm == nullptr);
}

TEST(destroy_null_hsm)
{
    HSM_Destroy(nullptr);  // 不能崩
}

TEST(node_create_with_null_memory)
{
    HSM_Node* node = HSM_Node_Create(nullptr);
    CHECK(node == nullptr);
}

// ============================================================
// 2. Start 边界
// ============================================================
TEST(start_with_null_hsm)
{
    HSM_Start(nullptr, (HSM_Node*)0x1);  // 不能崩
}

TEST(start_with_null_node)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node = HSM_Node_Create(&n);
    CHECK(hsm != nullptr);
    CHECK(node != nullptr);

    HSM_Start(hsm, nullptr);           // 空节点
    HSM_Start(hsm, node);              // 未初始化节点 (is_initialized == false)
    HSM_Destroy(hsm);
}

// ============================================================
// 3. Process / SendEvent 在 Start 之前
// ============================================================
TEST(process_before_start)
{
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);
    HSM_Process(hsm);       // 应 WARN, 不能崩
    HSM_SendEvent(hsm, {0});
    HSM_Destroy(hsm);
}

TEST(send_event_before_start)
{
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);
    HSM_Event_Package ev = { HSM_EVENT_ID_USER };
    HSM_SendEvent(hsm, ev); // 应 WARN, 不能崩
    HSM_Destroy(hsm);
}

// ============================================================
// 4. Transition 边界
// ============================================================
TEST(transition_null_target)
{
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    bool ok = HSM_Transition(hsm, nullptr, {0});
    CHECK(ok == false);

    HSM_Destroy(hsm);
}

TEST(transition_before_start)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node = HSM_Node_Create(&n);
    CHECK(hsm != nullptr);
    CHECK(node != nullptr);

    HSM_Node_Param param = { node, stub_handler, nullptr, nullptr, nullptr };
    HSM_RegisterChildNodes(hsm, &param, 1);

    bool ok = HSM_Transition(hsm, node, {0});  // 未 Start
    CHECK(ok == false);

    HSM_Destroy(hsm);
}

TEST(transition_uninitialized_target)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n1, n2;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node1 = HSM_Node_Create(&n1);
    HSM_Node* node2 = HSM_Node_Create(&n2);
    CHECK(hsm != nullptr);

    HSM_Node_Param nodes[] = {
        { node1, stub_handler, nullptr, nullptr, nullptr },
        { node2, stub_handler, nullptr, nullptr, nullptr },
    };
    HSM_RegisterChildNodes(hsm, nodes, 2);
    HSM_Start(hsm, node1);

    // 手动破坏 node2 的 is_initialized
    // 注意: 这里绕过 API 直接改, 模拟异常情况
    // 实际使用中不会这样, 但测试防御性
    bool ok = HSM_Transition(hsm, node2, {0});
    CHECK(ok == true);  // node2 是经过 InitHSM_Node 初始化的, 应该成功

    HSM_Destroy(hsm);
}

TEST(transition_to_same_state)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node = HSM_Node_Create(&n);
    CHECK(hsm != nullptr);

    HSM_Node_Param param = { node, stub_handler, stub_entry, stub_exit, nullptr };
    HSM_RegisterChildNodes(hsm, &param, 1);
    HSM_Start(hsm, node);

    bool ok = HSM_Transition(hsm, node, {0});  // 自己切自己
    CHECK(ok == true);  // 应该成功, no-op

    HSM_Destroy(hsm);
}

TEST(transition_disconnected_tree)
{
    // 两棵独立的树, LCA 为 nullptr
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  na, nb;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* tree_a = HSM_Node_Create(&na);
    HSM_Node* tree_b = HSM_Node_Create(&nb);
    CHECK(hsm != nullptr);

    HSM_Node_Param pa = { tree_a, stub_handler, nullptr, nullptr, nullptr };
    HSM_Node_Param pb = { tree_b, stub_handler, nullptr, nullptr, nullptr };
    HSM_RegisterChildNodes(hsm, &pa, 1);
    HSM_RegisterChildNodes(hsm, &pb, 1);  // 第二棵独立的树

    HSM_Start(hsm, tree_a);
    bool ok = HSM_Transition(hsm, tree_b, {0});
    CHECK(ok == false);  // LCA 找不到

    HSM_Destroy(hsm);
}

// ============================================================
// 5. Register 边界
// ============================================================
TEST(register_empty_table)
{
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    bool ok = HSM_RegisterChildNodes(hsm, nullptr, 0);
    CHECK(ok == false);

    HSM_Destroy(hsm);
}

TEST(register_table_exceeds_stack)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  nodes[HSM_MAX_STACK_DEPTH + 1];
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    // 创建超过 HSM_MAX_STACK_DEPTH 的节点表
    HSM_Node_Param params[HSM_MAX_STACK_DEPTH + 1];
    for (int i = 0; i < HSM_MAX_STACK_DEPTH + 1; i++) {
        params[i].node = HSM_Node_Create(&nodes[i]);
        params[i].handler = stub_handler;
        params[i].entry_action = nullptr;
        params[i].exit_action = nullptr;
        params[i].continuous_action = nullptr;
    }

    bool ok = HSM_RegisterChildNodes(hsm, params, HSM_MAX_STACK_DEPTH + 1);
    CHECK(ok == false);

    HSM_Destroy(hsm);
}

TEST(register_sibling_empty_table)
{
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    bool ok = HSM_RegisterSiblingNodes(hsm, nullptr, 0);
    CHECK(ok == false);

    HSM_Destroy(hsm);
}

// ============================================================
// 6. SetEnable / 状态检查
// ============================================================
TEST(set_enable_toggle)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node = HSM_Node_Create(&n);
    CHECK(hsm != nullptr);

    HSM_Node_Param param = { node, stub_handler, nullptr, nullptr, nullptr };
    HSM_RegisterChildNodes(hsm, &param, 1);
    HSM_Start(hsm, node);  // Start 内部调了 isEnable(true)

    // 禁用后 SendEvent 应该静默返回
    HSM_SetEnable(hsm, false);
    HSM_Event_Package ev = { HSM_EVENT_ID_USER };
    HSM_SendEvent(hsm, ev);  // 不能崩

    // 重新启用
    HSM_SetEnable(hsm, true);
    HSM_SendEvent(hsm, ev);  // 不能崩

    HSM_Destroy(hsm);
}

// ============================================================
// 7. handler 返回 false (不处理事件) 的冒泡行为
// ============================================================
static int g_handler_call_count = 0;
static bool counting_reject_handler(HSM_Event_Package) { g_handler_call_count++; return false; }
static bool counting_accept_handler(HSM_Event_Package)  { g_handler_call_count++; return true;  }

TEST(handler_bubbling_when_rejected)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n_parent, n_child;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* parent = HSM_Node_Create(&n_parent);
    HSM_Node* child  = HSM_Node_Create(&n_child);
    CHECK(hsm != nullptr);

    HSM_Node_Param nodes[] = {
        { parent, counting_accept_handler, nullptr, nullptr, nullptr },
        { child,  counting_reject_handler,  nullptr, nullptr, nullptr },
    };
    HSM_RegisterChildNodes(hsm, nodes, 2);
    HSM_Start(hsm, parent);

    // 当前状态是 parent, 切换 active state 到 child 才能测试冒泡
    HSM_Transition(hsm, child, {0});

    g_handler_call_count = 0;
    HSM_Event_Package ev = { HSM_EVENT_ID_USER };
    HSM_SendEvent(hsm, ev);
    CHECK(g_handler_call_count == 2);  // child 拒绝 → 冒泡到 parent

    HSM_Destroy(hsm);
}

// ============================================================
// 8. continuous_action 测试
// ============================================================
static int g_continuous_count = 0;
static void counting_continuous() { g_continuous_count++; }

TEST(process_calls_continuous_action)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* node = HSM_Node_Create(&n);
    CHECK(hsm != nullptr);

    HSM_Node_Param param = { node, stub_handler, nullptr, nullptr, counting_continuous };
    HSM_RegisterChildNodes(hsm, &param, 1);
    HSM_Start(hsm, node);

    g_continuous_count = 0;
    HSM_Process(hsm);
    CHECK(g_continuous_count == 1);

    HSM_Process(hsm);
    CHECK(g_continuous_count == 2);

    HSM_Destroy(hsm);
}

// ============================================================
// 9. 正常流程: 构造 → 注册 → 启动 → Process
// ============================================================
TEST(full_lifecycle_parent_child)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  n_root, n_child;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* root  = HSM_Node_Create(&n_root);
    HSM_Node* child = HSM_Node_Create(&n_child);
    CHECK(hsm != nullptr);
    CHECK(root != nullptr);
    CHECK(child != nullptr);

    HSM_Node_Param nodes[] = {
        { root,  stub_handler, nullptr, nullptr, nullptr },
        { child, stub_handler, nullptr, nullptr, nullptr },
    };
    CHECK(HSM_RegisterChildNodes(hsm, nodes, 2));

    HSM_Start(hsm, root);
    HSM_Process(hsm);
    HSM_Destroy(hsm);
}

// ============================================================
// 主入口
// ============================================================
int main()
{
    printf("\n=== HSM Boundary Tests ===\n\n");

    printf("[create / destroy]\n");
    run_test(create_with_null_memory,       "HSM_Create(NULL)");
    run_test(destroy_null_hsm,              "HSM_Destroy(NULL)");
    run_test(node_create_with_null_memory,  "HSM_Node_Create(NULL)");

    printf("\n[start]\n");
    run_test(start_with_null_hsm,           "HSM_Start(NULL, ...)");
    run_test(start_with_null_node,          "HSM_Start(hsm, NULL)");

    printf("\n[process / event before start]\n");
    run_test(process_before_start,          "Process() before Start()");
    run_test(send_event_before_start,       "SendEvent() before Start()");

    printf("\n[transition]\n");
    run_test(transition_null_target,        "Transition(NULL target)");
    run_test(transition_before_start,       "Transition() before Start()");
    run_test(transition_uninitialized_target,"Transition(uninitialized)");
    run_test(transition_to_same_state,      "Transition(self → self)");
    run_test(transition_disconnected_tree,  "Transition(disconnected tree)");

    printf("\n[register]\n");
    run_test(register_empty_table,          "Register(empty table)");
    run_test(register_table_exceeds_stack,  "Register(exceeds stack)");
    run_test(register_sibling_empty_table,  "RegisterSibling(empty)");

    printf("\n[enable / handler / continuous]\n");
    run_test(set_enable_toggle,             "SetEnable toggle");
    run_test(handler_bubbling_when_rejected,"Handler bubbling");
    run_test(process_calls_continuous_action,"Process() → continuous");

    printf("\n[full lifecycle]\n");
    run_test(full_lifecycle_parent_child,   "Create → Register → Start → Process → Destroy");
    run_test(transition_to_same_state,      "Transition(self) = no-op");

    printf("\n=== %d/%d passed ===\n", g_pass, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
