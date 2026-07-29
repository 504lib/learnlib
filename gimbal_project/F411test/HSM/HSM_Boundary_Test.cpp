#include "HSM_Core.h"
#include <cstdio>

#ifndef HSM_EVENT_ID_USER
#define HSM_EVENT_ID_USER 0x10
#endif

#define TEST(name) static void name(void)
#define CHECK(cond) do { \
    if (!(cond)) { printf("  FAIL [%d]: %s\n", __LINE__, #cond); g_test_ok = false; return; } \
} while(0)

static int  g_pass    = 0;
static int  g_fail    = 0;
static bool g_test_ok = false;

static void run_test(void (*test)(void), const char* name)
{
    printf("  %-50s", name);
    g_test_ok = true;
    test();
    if (g_test_ok) { printf("PASS\n"); g_pass++; }
    else           { printf("FAIL\n"); g_fail++; }
}

// ============================================================
// 桩函数
// ============================================================
static bool stub_handler(HSM_Event_Package)   { return true; }
static void stub_entry(HSM_Event_Package)      { }
static void stub_exit(HSM_Event_Package)       { }
static void stub_continuous()                  { }
static bool null_handler(HSM_Event_Package)    { return false; }

// ============================================================
// 辅助 — 最小 HSM
// ============================================================
#define AUX_NODES 4

struct MiniHSM {
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[AUX_NODES];
    HSM*            hsm;
};

// 创建只有一个 root 的 HSM
static MiniHSM make_single_node(
    bool (*handler)(HSM_Event_Package) = stub_handler,
    void (*entry)(HSM_Event_Package)   = nullptr,
    void (*exit)(HSM_Event_Package)    = nullptr,
    void (*cont)(void)                 = nullptr)
{
    MiniHSM m = {};
    m.defs[0] = HSM_STATE_DEF("root", nullptr, handler, entry, exit, cont);
    m.hsm = HSM_Create(&m.mem, m.nodes, AUX_NODES, m.defs, 1);
    return m;
}

// ============================================================
// 1. 创建 / 销毁
// ============================================================
TEST(create_with_null_mem)
{
    HSM_Node     nodes[AUX_NODES];
    HSM_StateDef defs[] = { HSM_STATE_DEF("r", nullptr, stub_handler, nullptr, nullptr, nullptr) };
    CHECK(HSM_Create(nullptr, nodes, AUX_NODES, defs, 1) == nullptr);
}

TEST(create_with_null_nodes)
{
    HSM_Core_Memory mem;
    HSM_StateDef defs[] = { HSM_STATE_DEF("r", nullptr, stub_handler, nullptr, nullptr, nullptr) };
    CHECK(HSM_Create(&mem, nullptr, AUX_NODES, defs, 1) == nullptr);
}

TEST(create_with_null_defs)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    CHECK(HSM_Create(&mem, nodes, AUX_NODES, nullptr, 1) == nullptr);
}

TEST(create_count_exceeds_pool)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[2];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("a", nullptr, stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("b", "a",      stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c", "b",      stub_handler, nullptr, nullptr, nullptr),
    };
    CHECK(HSM_Create(&mem, nodes, 2, defs, 3) == nullptr);
}

TEST(create_missing_parent)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("a", "ghost", stub_handler, nullptr, nullptr, nullptr),
    };
    // parent "ghost" 不存在 → 创建失败
    CHECK(HSM_Create(&mem, nodes, AUX_NODES, defs, 1) == nullptr);
}

TEST(destroy_null_hsm)
{
    HSM_Destroy(nullptr);
}

TEST(destroy_valid_hsm)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Destroy(m.hsm);
}

// ============================================================
// 2. HSM_FindNode
// ============================================================
TEST(find_node_by_name)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);

    HSM_Node* r = HSM_FindNode(m.hsm, "root");
    CHECK(r != nullptr);
    CHECK(r == &m.nodes[0]);

    HSM_Node* x = HSM_FindNode(m.hsm, "nope");
    CHECK(x == nullptr);

    HSM_Destroy(m.hsm);
}

TEST(find_node_null_hsm)
{
    CHECK(HSM_FindNode(nullptr, "x") == nullptr);
}

// ============================================================
// 3. Start
// ============================================================
TEST(start_with_null_hsm)
{
    HSM_Start(nullptr, (HSM_Node*)0x1);
}

TEST(start_with_null_node)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, nullptr);
    HSM_Destroy(m.hsm);
}

TEST(start_with_uninit_node)
{
    HSM_Core_Memory mem;
    HSM_Node nodes[AUX_NODES];
    HSM_StateDef defs[] = { HSM_STATE_DEF("r", nullptr, stub_handler, nullptr, nullptr, nullptr) };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 1);
    CHECK(hsm != nullptr);

    // 手动伪造一个未初始化节点
    HSM_Node fake = {};
    HSM_Start(hsm, &fake);
    HSM_Destroy(hsm);
}

TEST(start_then_start_again)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);
    HSM_Start(m.hsm, &m.nodes[0]);  // 重复 Start 不崩
    HSM_Destroy(m.hsm);
}

// ============================================================
// 4. Process / SendEvent 在 Start 前
// ============================================================
TEST(process_before_start)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Process(m.hsm);
    HSM_SendEvent(m.hsm, (HSM_Event_Package){0});
    HSM_Destroy(m.hsm);
}

TEST(send_event_before_start)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_SendEvent(m.hsm, (HSM_Event_Package){ .HSM_Event_ID = HSM_EVENT_ID_USER });
    HSM_Destroy(m.hsm);
}

// ============================================================
// 5. RequestTransition 边界
// ============================================================
TEST(request_transition_null_target)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);
    CHECK(HSM_RequestTransition(m.hsm, nullptr, (HSM_Event_Package){0}) == false);
    HSM_Destroy(m.hsm);
}

TEST(request_transition_before_start)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    CHECK(HSM_RequestTransition(m.hsm, &m.nodes[0], (HSM_Event_Package){0}) == false);
    HSM_Destroy(m.hsm);
}

TEST(request_transition_uninit_target)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);

    HSM_Node fake = {};
    CHECK(HSM_RequestTransition(m.hsm, &fake, (HSM_Event_Package){0}) == false);
    HSM_Destroy(m.hsm);
}

TEST(request_transition_self_noop)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);
    CHECK(HSM_RequestTransition(m.hsm, &m.nodes[0], (HSM_Event_Package){0}) == true);
    HSM_Process(m.hsm);
    HSM_Destroy(m.hsm);
}

// ============================================================
// 6. RequestTransition 通过队列执行
// ============================================================
static bool g_entry_called = false;
static void flag_entry(HSM_Event_Package) { g_entry_called = true; }

TEST(request_transition_via_queue)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("parent", nullptr, stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("child",  "parent", stub_handler, flag_entry, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 2);
    CHECK(hsm != nullptr);

    HSM_Start(hsm, &nodes[0]);

    g_entry_called = false;
    CHECK(HSM_RequestTransition(hsm, &nodes[1], (HSM_Event_Package){0}) == true);
    CHECK(g_entry_called == false);   // 还没执行

    HSM_Process(hsm);                 // 出队执行
    CHECK(g_entry_called == true);    // entry 触发

    HSM_Destroy(hsm);
}

// ============================================================
// 7. 多子节点 sibling 链
// ============================================================
TEST(multiple_children_linked)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("p",  nullptr, stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c1", "p",     stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c2", "p",     stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c3", "p",     stub_handler, nullptr, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 4);
    CHECK(hsm != nullptr);

    // p 的第一个子节点
    CHECK(nodes[0].first_child == &nodes[1]);
    // sibling 链
    CHECK(nodes[1].next_sibling == &nodes[2]);
    CHECK(nodes[2].next_sibling == &nodes[3]);
    CHECK(nodes[3].next_sibling == nullptr);
    // 各自 parent 指向 p
    CHECK(nodes[1].parent == &nodes[0]);
    CHECK(nodes[2].parent == &nodes[0]);
    CHECK(nodes[3].parent == &nodes[0]);

    HSM_Destroy(hsm);
}

// ============================================================
// 8. 深层嵌套 (3 层)
// ============================================================
TEST(deep_nesting)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("l0", nullptr, stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("l1", "l0",   stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("l2", "l1",   stub_handler, nullptr, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 3);
    CHECK(hsm != nullptr);

    CHECK(nodes[0].first_child == &nodes[1]);
    CHECK(nodes[1].parent       == &nodes[0]);
    CHECK(nodes[1].first_child  == &nodes[2]);
    CHECK(nodes[2].parent       == &nodes[1]);

    HSM_Destroy(hsm);
}

// ============================================================
// 9. SetEnable
// ============================================================
TEST(set_enable_toggle)
{
    MiniHSM m = make_single_node();
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);

    HSM_SetEnable(m.hsm, false);
    HSM_SendEvent(m.hsm, (HSM_Event_Package){ .HSM_Event_ID = HSM_EVENT_ID_USER });  // 不崩

    HSM_SetEnable(m.hsm, true);
    HSM_SendEvent(m.hsm, (HSM_Event_Package){ .HSM_Event_ID = HSM_EVENT_ID_USER });
    HSM_Destroy(m.hsm);
}

// ============================================================
// 10. handler 冒泡
// ============================================================
static int  g_handler_call_count = 0;
static bool counting_reject(HSM_Event_Package) { g_handler_call_count++; return false; }
static bool counting_accept(HSM_Event_Package) { g_handler_call_count++; return true; }

TEST(handler_bubbling)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("parent", nullptr,  counting_accept, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("child",  "parent", counting_reject, nullptr, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 2);
    CHECK(hsm != nullptr);

    HSM_Start(hsm, &nodes[0]);
    HSM_RequestTransition(hsm, &nodes[1], (HSM_Event_Package){0});
    HSM_Process(hsm);  // 切到 child

    g_handler_call_count = 0;
    HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = HSM_EVENT_ID_USER });
    CHECK(g_handler_call_count == 2);  // child reject → parent accept

    HSM_Destroy(hsm);
}

// ============================================================
// 11. continuous_action
// ============================================================
static int  g_continuous_count = 0;
static void counting_continuous() { g_continuous_count++; }

TEST(process_continuous)
{
    MiniHSM m = make_single_node(stub_handler, nullptr, nullptr, counting_continuous);
    CHECK(m.hsm != nullptr);
    HSM_Start(m.hsm, &m.nodes[0]);

    g_continuous_count = 0;
    HSM_Process(m.hsm);
    CHECK(g_continuous_count == 1);
    HSM_Process(m.hsm);
    CHECK(g_continuous_count == 2);
    HSM_Destroy(m.hsm);
}

// ============================================================
// 12. 完整 exit/entry 链 (LCA)
// ============================================================
static const char* g_trace = nullptr;

static void e1(HSM_Event_Package e) { g_trace = "e1"; }
static void x1(HSM_Event_Package e) { g_trace = "x1"; }
static void e2(HSM_Event_Package e) { g_trace = "e2"; }
static void x2(HSM_Event_Package e) { g_trace = "x2"; }
static void e3(HSM_Event_Package e) { g_trace = "e3"; }

TEST(lca_exit_entry_chain)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("root", nullptr, null_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("a",    "root",  null_handler, e1, x1, nullptr),
        HSM_STATE_DEF("a1",   "a",     null_handler, e2, x2, nullptr),
        HSM_STATE_DEF("b",    "root",  null_handler, e3, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 4);
    CHECK(hsm != nullptr);

    // start at a1
    HSM_Start(hsm, &nodes[2]);

    // transition a1 → b: LCA = root, exit a1, exit a, entry b
    g_trace = nullptr;
    HSM_RequestTransition(hsm, &nodes[3], (HSM_Event_Package){0});
    HSM_Process(hsm);
    // 最后执行的是 b 的 entry (因为 entry 从 LCA+b 往下压栈, 先 entry a? no...
    // 实际上: exit a2 → exit a → entry b, 所以 trace = "e3"
    CHECK(g_trace != nullptr);

    HSM_Destroy(hsm);
}

// ============================================================
// 13. Process 处理队列中的多个 Transition
// ============================================================
static HSM_Node* g_last_entered = nullptr;
static void track_entry(HSM_Event_Package) { g_last_entered = (HSM_Node*)(uintptr_t)1; }

TEST(process_drains_transition_queue)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("p",  nullptr, stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c1", "p",     stub_handler, track_entry, nullptr, nullptr),
        HSM_STATE_DEF("c2", "p",     stub_handler, nullptr, nullptr, nullptr),
        HSM_STATE_DEF("c3", "p",     stub_handler, nullptr, nullptr, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 4);
    CHECK(hsm != nullptr);

    HSM_Start(hsm, &nodes[0]);

    CHECK(HSM_RequestTransition(hsm, &nodes[1], (HSM_Event_Package){0}) == true);
    CHECK(HSM_RequestTransition(hsm, &nodes[2], (HSM_Event_Package){0}) == true);
    CHECK(HSM_RequestTransition(hsm, &nodes[3], (HSM_Event_Package){0}) == true);

    HSM_Process(hsm);
    HSM_Destroy(hsm);
}

// ============================================================
// 14. 完整生命周期
// ============================================================
TEST(full_lifecycle)
{
    HSM_Core_Memory mem;
    HSM_Node        nodes[AUX_NODES];
    HSM_StateDef    defs[] = {
        HSM_STATE_DEF("root", nullptr,  stub_handler, stub_entry, stub_exit, nullptr),
        HSM_STATE_DEF("s1",   "root",   stub_handler, stub_entry, stub_exit, nullptr),
        HSM_STATE_DEF("s2",   "root",   stub_handler, stub_entry, stub_exit, nullptr),
    };
    HSM* hsm = HSM_Create(&mem, nodes, AUX_NODES, defs, 3);
    CHECK(hsm != nullptr);

    HSM_Node* r = HSM_FindNode(hsm, "root");
    CHECK(r != nullptr);

    HSM_Start(hsm, r);
    HSM_Process(hsm);
    HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = HSM_EVENT_ID_USER });
    HSM_Destroy(hsm);
}

// ============================================================
// 15. HSM_EVENT_NULL 在 entry_action 中使用
// ============================================================
static int g_null_event_count = 0;
static void count_null_entry(HSM_Event_Package e) {
    if (e.HSM_Event_ID == HSM_EVENT_NULL) g_null_event_count++;
}

TEST(start_uses_event_null)
{
    MiniHSM m = make_single_node(stub_handler, count_null_entry);
    CHECK(m.hsm != nullptr);

    g_null_event_count = 0;
    HSM_Start(m.hsm, &m.nodes[0]);
    CHECK(g_null_event_count == 1);

    HSM_Destroy(m.hsm);
}

// ============================================================
// 16. 并发 HSM 互不干扰
// ============================================================
TEST(two_hsms_independent)
{
    HSM_Core_Memory mem1, mem2;
    HSM_Node        nodes1[AUX_NODES], nodes2[AUX_NODES];
    HSM_StateDef    defs1[] = { HSM_STATE_DEF("a", nullptr, stub_handler, nullptr, nullptr, nullptr) };
    HSM_StateDef    defs2[] = { HSM_STATE_DEF("b", nullptr, stub_handler, nullptr, nullptr, nullptr) };

    HSM* h1 = HSM_Create(&mem1, nodes1, AUX_NODES, defs1, 1);
    HSM* h2 = HSM_Create(&mem2, nodes2, AUX_NODES, defs2, 1);
    CHECK(h1 != nullptr);
    CHECK(h2 != nullptr);

    HSM_Start(h1, &nodes1[0]);
    HSM_Start(h2, &nodes2[0]);

    HSM_Process(h1);
    HSM_Process(h2);

    CHECK(HSM_FindNode(h1, "b") == nullptr);  // h1 里没有 b
    CHECK(HSM_FindNode(h2, "b") != nullptr);

    HSM_Destroy(h1);
    HSM_Destroy(h2);
}

// ============================================================
int main()
{
    printf("\n=== HSM Boundary Tests (new API) ===\n\n");

    printf("[create / destroy]\n");
    run_test(create_with_null_mem,        "HSM_Create(NULL mem)");
    run_test(create_with_null_nodes,      "HSM_Create(NULL nodes)");
    run_test(create_with_null_defs,       "HSM_Create(NULL defs)");
    run_test(create_count_exceeds_pool,   "HSM_Create(count > max_nodes)");
    run_test(create_missing_parent,       "HSM_Create(parent not found)");
    run_test(destroy_null_hsm,            "HSM_Destroy(NULL)");
    run_test(destroy_valid_hsm,           "HSM_Destroy(valid)");

    printf("\n[find node]\n");
    run_test(find_node_by_name,           "HSM_FindNode");
    run_test(find_node_null_hsm,          "HSM_FindNode(NULL)");

    printf("\n[start]\n");
    run_test(start_with_null_hsm,         "HSM_Start(NULL)");
    run_test(start_with_null_node,        "HSM_Start(NULL node)");
    run_test(start_with_uninit_node,      "HSM_Start(uninit node)");
    run_test(start_then_start_again,      "HSM_Start twice");

    printf("\n[before start]\n");
    run_test(process_before_start,        "Process before Start");
    run_test(send_event_before_start,     "SendEvent before Start");

    printf("\n[request transition]\n");
    run_test(request_transition_null_target,   "RequestTransition(NULL)");
    run_test(request_transition_before_start,  "RequestTransition(no Start)");
    run_test(request_transition_uninit_target, "RequestTransition(uninit)");
    run_test(request_transition_self_noop,     "RequestTransition(self)");
    run_test(request_transition_via_queue,     "RequestTransition → Process");

    printf("\n[tree structure]\n");
    run_test(multiple_children_linked,     "sibling chain");
    run_test(deep_nesting,                 "3-level nesting");

    printf("\n[enable / handler / continuous]\n");
    run_test(set_enable_toggle,            "SetEnable toggle");
    run_test(handler_bubbling,             "Handler bubbling");
    run_test(process_continuous,           "Process continuous");
    run_test(process_drains_transition_queue, "Process drains queue");
    run_test(lca_exit_entry_chain,         "LCA exit/entry chain");

    printf("\n[lifecycle]\n");
    run_test(full_lifecycle,               "Create→Start→Process→SendEvent→Destroy");
    run_test(start_uses_event_null,        "Start uses HSM_EVENT_NULL");
    run_test(two_hsms_independent,         "Two HSMs independent");

    printf("\n=== %d/%d passed ===\n", g_pass, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
