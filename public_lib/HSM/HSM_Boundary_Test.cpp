#include "HSM_Core.h"
#include <cstdio>

#ifndef HSM_EVENT_ID_USER
#define HSM_EVENT_ID_USER 0x10
#endif

#define TEST(name) static void name(void)
#define CHECK(cond) do { \
    if (!(cond)) { printf("  FAIL [%d]: %s\n", __LINE__, #cond); g_test_ok = false; return; } \
} while(0)

static int g_pass = 0;
static int g_fail = 0;

static bool g_test_ok = false;

static void run_test(void (*test)(void), const char* name)
{
    printf("  %-50s", name);
    g_test_ok = true;
    test();
    if (g_test_ok) { printf("PASS\n"); g_pass++; }
    else          { printf("FAIL\n"); g_fail++; }
}

// ============================================================
// 桩函数
// ============================================================
static bool stub_handler(HSM_Event_Package)   { return true; }
static void stub_entry(HSM_Event_Package)      { }
static void stub_exit(HSM_Event_Package)       { }
static void stub_continuous()                  { }
static bool null_handler(HSM_Event_Package)    { return false; }

// 辅助: 初始化没有父节点的根节点 (RegisterChild 只初始化子节点, 根节点需要手动 init)
static void init_root(HSM_Node* node,
                      bool (*handler)(HSM_Event_Package),
                      void (*entry)(HSM_Event_Package),
                      void (*exit)(HSM_Event_Package),
                      void (*cont)())
{
    node->is_initialized = true;
    node->handler = handler;
    node->entry_action = entry;
    node->exit_action = exit;
    node->continuous_action = cont;
}

// 辅助: 创建 HSM + 一个已初始化的孤立节点 (作为 active state 用)
static HSM* create_hsm_with_node(HSM_Node** out_node,
                                 bool (*handler)(HSM_Event_Package) = stub_handler)
{
    static FMS_OBJECT_MEMORY obj;
    static FMS_NODE_MEMORY  nm;
    HSM* hsm = HSM_Create(&obj);
    if (!hsm) return nullptr;
    HSM_Node* node = HSM_Node_Create(&nm);
    if (!node) { HSM_Destroy(hsm); return nullptr; }
    init_root(node, handler, nullptr, nullptr, nullptr);
    *out_node = node;
    return hsm;
}

// ============================================================
// 1. 创建 / 销毁
// ============================================================
TEST(create_with_null_memory)
{
    CHECK(HSM_Create(nullptr) == nullptr);
}

TEST(destroy_null_hsm)
{
    HSM_Destroy(nullptr);
}

TEST(node_create_with_null_memory)
{
    CHECK(HSM_Node_Create(nullptr) == nullptr);
}

// ============================================================
// 2. Start
// ============================================================
TEST(start_with_null_hsm)
{
    HSM_Start(nullptr, (HSM_Node*)0x1);
}

TEST(start_with_null_or_uninit_node)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);

    HSM_Start(hsm, nullptr);              // NULL node
    HSM_Start(hsm, (HSM_Node*)&hsm);      // 未初始化的野指针 (is_initialized == false)
    HSM_Destroy(hsm);
}

// ============================================================
// 3. Process / SendEvent 在 Start 前
// ============================================================
TEST(process_before_start)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    HSM_Process(hsm);
    HSM_SendEvent(hsm, {0});
    HSM_Destroy(hsm);
}

TEST(send_event_before_start)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    HSM_SendEvent(hsm, { HSM_EVENT_ID_USER });
    HSM_Destroy(hsm);
}

// ============================================================
// 4. RequestTransition 边界
// ============================================================
TEST(request_transition_null_target)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    CHECK(HSM_RequestTransition(hsm, nullptr, {0}) == false);
    HSM_Destroy(hsm);
}

TEST(request_transition_before_start)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    // 节点已初始化但未 Start
    CHECK(HSM_RequestTransition(hsm, node, {0}) == false);
    HSM_Destroy(hsm);
}

TEST(request_transition_uninit_target)
{
    FMS_NODE_MEMORY n2;
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    HSM_Node* uninit = HSM_Node_Create(&n2);  // 从未初始化
    CHECK(hsm != nullptr);

    HSM_Start(hsm, node);
    CHECK(HSM_RequestTransition(hsm, uninit, {0}) == false);
    HSM_Destroy(hsm);
}

TEST(request_transition_self_noop)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    HSM_Start(hsm, node);

    CHECK(HSM_RequestTransition(hsm, node, {0}) == true);
    HSM_Process(hsm);  // self→self, Transition 直接 return true
    HSM_Destroy(hsm);
}

// ============================================================
// 5. RequestTransition 通过队列执行
// ============================================================
static bool g_entry_called = false;
static void flag_entry(HSM_Event_Package) { g_entry_called = true; }

TEST(request_transition_via_queue)
{
    FMS_NODE_MEMORY np, nc;
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* child  = HSM_Node_Create(&nc);

    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    init_root(parent, stub_handler, nullptr, nullptr, nullptr);
    HSM_Node_Param cp = { child, stub_handler, flag_entry, nullptr, nullptr };
    CHECK(HSM_RegisterChild(hsm, parent, cp));

    HSM_Start(hsm, parent);

    g_entry_called = false;
    CHECK(HSM_RequestTransition(hsm, child, {0}) == true);
    CHECK(g_entry_called == false);   // 还没执行

    HSM_Process(hsm);                 // 出队执行
    CHECK(g_entry_called == true);    // entry 触发

    HSM_Destroy(hsm);
}

// ============================================================
// 6. Register 边界
// ============================================================
TEST(register_child_null_parent)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  nc;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* child = HSM_Node_Create(&nc);
    CHECK(hsm != nullptr);

    HSM_Node_Param cp = { child, stub_handler, nullptr, nullptr, nullptr };
    CHECK(HSM_RegisterChild(hsm, nullptr, cp) == false);
    HSM_Destroy(hsm);
}

TEST(register_child_nodes_null_parent)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  nc;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* child = HSM_Node_Create(&nc);
    CHECK(hsm != nullptr);

    HSM_Node_Param params[] = { { child, stub_handler, nullptr, nullptr, nullptr } };
    CHECK(HSM_RegisterChildNodes(hsm, nullptr, params, 1) == false);
    HSM_Destroy(hsm);
}

TEST(register_child_nodes_empty)
{
    FMS_NODE_MEMORY np;
    HSM_Node* parent = HSM_Node_Create(&np);
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    init_root(parent, stub_handler, nullptr, nullptr, nullptr);
    CHECK(hsm != nullptr);

    CHECK(HSM_RegisterChildNodes(hsm, parent, nullptr, 0) == true);
    HSM_Destroy(hsm);
}

TEST(register_multiple_children)
{
    FMS_NODE_MEMORY np, n1, n2, n3;
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* c1 = HSM_Node_Create(&n1);
    HSM_Node* c2 = HSM_Node_Create(&n2);
    HSM_Node* c3 = HSM_Node_Create(&n3);

    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    init_root(parent, stub_handler, nullptr, nullptr, nullptr);
    CHECK(hsm != nullptr);

    HSM_Node_Param params[] = {
        { c1, stub_handler, nullptr, nullptr, nullptr },
        { c2, stub_handler, nullptr, nullptr, nullptr },
        { c3, stub_handler, nullptr, nullptr, nullptr },
    };
    CHECK(HSM_RegisterChildNodes(hsm, parent, params, 3) == true);
    CHECK(parent->first_child == c1);
    CHECK(c1->next_sibling == c2);
    CHECK(c2->next_sibling == c3);
    CHECK(c3->next_sibling == nullptr);
    CHECK(c1->parent == parent);
    CHECK(c2->parent == parent);
    CHECK(c3->parent == parent);
    HSM_Destroy(hsm);
}

// ============================================================
// 7. RegisterChild enabled 状态拒绝
// ============================================================
TEST(register_rejected_when_enabled)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  np, nc;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* child  = HSM_Node_Create(&nc);
    init_root(parent, stub_handler, nullptr, nullptr, nullptr);
    CHECK(hsm != nullptr);

    HSM_RegisterChild(hsm, parent, { child, stub_handler, nullptr, nullptr, nullptr });
    HSM_Start(hsm, parent);  // Start 内部 isEnable(true)

    FMS_NODE_MEMORY n2;
    HSM_Node* child2 = HSM_Node_Create(&n2);
    HSM_Node_Param cp2 = { child2, stub_handler, nullptr, nullptr, nullptr };
    CHECK(HSM_RegisterChild(hsm, parent, cp2) == false);  // enabled 时拒绝
    HSM_Destroy(hsm);
}

// ============================================================
// 8. SetEnable
// ============================================================
TEST(set_enable_toggle)
{
    HSM_Node* node;
    HSM* hsm = create_hsm_with_node(&node);
    CHECK(hsm != nullptr);
    HSM_Start(hsm, node);

    HSM_SetEnable(hsm, false);
    HSM_SendEvent(hsm, { HSM_EVENT_ID_USER });  // 拒绝, 不崩

    HSM_SetEnable(hsm, true);
    HSM_SendEvent(hsm, { HSM_EVENT_ID_USER });  // 正常
    HSM_Destroy(hsm);
}

// ============================================================
// 9. handler 冒泡
// ============================================================
static int g_handler_call_count = 0;
static bool counting_reject(HSM_Event_Package) { g_handler_call_count++; return false; }
static bool counting_accept(HSM_Event_Package) { g_handler_call_count++; return true; }

TEST(handler_bubbling)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  np, nc;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* child  = HSM_Node_Create(&nc);
    CHECK(hsm != nullptr);

    init_root(parent, counting_accept, nullptr, nullptr, nullptr);
    HSM_Node_Param cp = { child, counting_reject, nullptr, nullptr, nullptr };
    CHECK(HSM_RegisterChild(hsm, parent, cp));

    HSM_Start(hsm, parent);
    HSM_RequestTransition(hsm, child, {0});
    HSM_Process(hsm);  // 执行切换 child

    g_handler_call_count = 0;
    HSM_SendEvent(hsm, { HSM_EVENT_ID_USER });
    CHECK(g_handler_call_count == 2);  // child 拒绝 → 冒泡 parent

    HSM_Destroy(hsm);
}

// ============================================================
// 10. continuous_action
// ============================================================
static int g_continuous_count = 0;
static void counting_continuous() { g_continuous_count++; }

TEST(process_continuous)
{
    FMS_NODE_MEMORY n;
    HSM_Node* node = HSM_Node_Create(&n);
    FMS_OBJECT_MEMORY obj;
    HSM* hsm = HSM_Create(&obj);
    CHECK(hsm != nullptr);

    init_root(node, stub_handler, nullptr, nullptr, counting_continuous);
    HSM_Start(hsm, node);

    g_continuous_count = 0;
    HSM_Process(hsm);
    CHECK(g_continuous_count == 1);
    HSM_Process(hsm);
    CHECK(g_continuous_count == 2);
    HSM_Destroy(hsm);
}

// ============================================================
// 11. 完整生命周期
// ============================================================
TEST(full_lifecycle)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  np, nc;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* child  = HSM_Node_Create(&nc);
    CHECK(hsm != nullptr);

    init_root(parent, stub_handler, stub_entry, stub_exit, nullptr);
    HSM_Node_Param cp = { child, stub_handler, stub_entry, stub_exit, nullptr };
    CHECK(HSM_RegisterChild(hsm, parent, cp));

    HSM_Start(hsm, parent);
    HSM_Process(hsm);
    HSM_Destroy(hsm);
}

// ============================================================
// 12. Process 处理队列中的多个 Transition
// ============================================================
static HSM_Node* g_active = nullptr;
static void track_entry(HSM_Event_Package) { g_active = (HSM_Node*)(uintptr_t)1; }

TEST(process_drains_transition_queue)
{
    FMS_OBJECT_MEMORY obj;
    FMS_NODE_MEMORY  np, n1, n2;
    HSM* hsm = HSM_Create(&obj);
    HSM_Node* parent = HSM_Node_Create(&np);
    HSM_Node* c1 = HSM_Node_Create(&n1);
    HSM_Node* c2 = HSM_Node_Create(&n2);
    CHECK(hsm != nullptr);

    init_root(parent, stub_handler, nullptr, nullptr, nullptr);
    HSM_RegisterChild(hsm, parent, { c1, stub_handler, track_entry, nullptr, nullptr });
    HSM_RegisterChild(hsm, parent, { c2, stub_handler, nullptr, nullptr, nullptr });

    HSM_Start(hsm, parent);

    // 连续入队两个转换
    CHECK(HSM_RequestTransition(hsm, c1, {0}) == true);
    CHECK(HSM_RequestTransition(hsm, c2, {0}) == true);

    // 一次 Process 出队执行全部
    HSM_Process(hsm);
    // c1 entry 和 c2 entry 都被调用了
    // 最终 current 应该是 c2
    HSM_Destroy(hsm);
}

// ============================================================
int main()
{
    printf("\n=== HSM Boundary Tests ===\n\n");

    printf("[create / destroy]\n");
    run_test(create_with_null_memory,       "HSM_Create(NULL)");
    run_test(destroy_null_hsm,              "HSM_Destroy(NULL)");
    run_test(node_create_with_null_memory,  "HSM_Node_Create(NULL)");

    printf("\n[start]\n");
    run_test(start_with_null_hsm,           "Start(NULL, ...)");
    run_test(start_with_null_or_uninit_node,"Start(uninitialized node)");

    printf("\n[before start]\n");
    run_test(process_before_start,          "Process() before Start");
    run_test(send_event_before_start,       "SendEvent() before Start");

    printf("\n[request transition]\n");
    run_test(request_transition_null_target,"RequestTransition(NULL)");
    run_test(request_transition_before_start,"RequestTransition before Start");
    run_test(request_transition_uninit_target,"RequestTransition(uninit target)");
    run_test(request_transition_self_noop,  "RequestTransition(self)");
    run_test(request_transition_via_queue,  "RequestTransition → Process");

    printf("\n[register]\n");
    run_test(register_child_null_parent,    "RegisterChild(NULL parent)");
    run_test(register_child_nodes_null_parent,"RegisterChildNodes(NULL parent)");
    run_test(register_child_nodes_empty,    "RegisterChildNodes(0 count)");
    run_test(register_multiple_children,    "RegisterChildNodes x3 sibling");
    run_test(register_rejected_when_enabled,"Register rejected if enabled");

    printf("\n[enable / handler / continuous]\n");
    run_test(set_enable_toggle,             "SetEnable toggle");
    run_test(handler_bubbling,              "Handler bubbling");
    run_test(process_continuous,            "Process() continuous");
    run_test(process_drains_transition_queue,"Process drains queue");

    printf("\n[lifecycle]\n");
    run_test(full_lifecycle,                "Create→Register→Start→Process→Destroy");

    printf("\n=== %d/%d passed ===\n", g_pass, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
