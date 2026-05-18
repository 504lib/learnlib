#include "Finite_State_Machine.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    TEST_START_ENTRY_ACTION = 1 << 0,
    TEST_NULLABLE_ENTRY_EXIT = 1 << 1,
    TEST_TRANSITION_ORDER = 1 << 2,
    TEST_QUEUE_SINGLE_SLOT = 1 << 3,
    TEST_ENABLE_WINDOW = 1 << 4,
    TEST_INVALID_INPUTS = 1 << 5,
    TEST_ALL = (1 << 6) - 1
};

typedef struct
{
    int state1_entry_count;
    int state1_action_count;
    int state1_exit_count;
    int state2_entry_count;
    int state2_action_count;
    int state2_exit_count;
    int sequence[16];
    int sequence_count;
    int first_transition_result;
    int second_transition_result;
} CallbackRecorder;

typedef struct
{
    FSM_Structure* fsm;
    DWORD step_ms;
    DWORD elapsed_ms;
    DWORD transition_at_ms;
    DWORD disable_at_ms;
    DWORD enable_at_ms;
    int transition_requested;
    int attempt_double_transition;
    int disable_applied;
    int enable_applied;
} SoftwareClock;

static CallbackRecorder g_recorder;
static SoftwareClock g_clock;

static void reset_recorder(void)
{
    memset(&g_recorder, 0, sizeof(g_recorder));
}

static void reset_software_clock(FSM_Structure* fsm, DWORD step_ms)
{
    memset(&g_clock, 0, sizeof(g_clock));
    g_clock.fsm = fsm;
    g_clock.step_ms = step_ms;
}

static void run_software_clock(unsigned int cycle_count)
{
    unsigned int cycle;

    for (cycle = 0; cycle < cycle_count; ++cycle)
    {
        /*
         * 这里使用固定步进的软件时间，Sleep 只负责模拟节拍，
         * 真正的判定基准由 cycle * step_ms 决定，避免系统调度抖动。
         */
        g_clock.elapsed_ms = cycle * g_clock.step_ms;

        /*
         * 用软件时钟模拟主循环外部事件：先改使能，再推进 FSM，
         * 这样可以覆盖“同一拍进入处理前系统被外部逻辑改变”的场景。
         */
        if (!g_clock.disable_applied && g_clock.disable_at_ms > 0U && g_clock.elapsed_ms >= g_clock.disable_at_ms)
        {
            FSM_Set_Enable(g_clock.fsm, false);
            g_clock.disable_applied = 1;
        }
        if (!g_clock.enable_applied && g_clock.enable_at_ms > 0U && g_clock.elapsed_ms >= g_clock.enable_at_ms)
        {
            FSM_Set_Enable(g_clock.fsm, true);
            g_clock.enable_applied = 1;
        }

        FSM_Process(g_clock.fsm);

        if ((cycle + 1U) < cycle_count)
        {
            Sleep(g_clock.step_ms);
        }
    }
}

static void assert_sequence(const int* expected, size_t expected_count)
{
    size_t index;

    assert(g_recorder.sequence_count == (int)expected_count);
    for (index = 0; index < expected_count; ++index)
    {
        assert(g_recorder.sequence[index] == expected[index]);
    }
}

static void record_event(int event_id)
{
    assert(g_recorder.sequence_count < (int)(sizeof(g_recorder.sequence) / sizeof(g_recorder.sequence[0])));
    g_recorder.sequence[g_recorder.sequence_count++] = event_id;
}

static void state1_entry(void)
{
    g_recorder.state1_entry_count += 1;
    record_event(11);
}

static void state1_action(void)
{
    g_recorder.state1_action_count += 1;
    record_event(12);

    /*
     * 这里用时间阈值触发状态切换，模拟主循环运行一段时间后
     * 由业务逻辑发起转换请求。
     */
    if (!g_clock.transition_requested && g_clock.transition_at_ms > 0U && g_clock.elapsed_ms >= g_clock.transition_at_ms)
    {
        g_recorder.first_transition_result = FSM_State_Transition(g_clock.fsm, 2U) ? 1 : 0;
        g_clock.transition_requested = 1;

        if (g_clock.attempt_double_transition)
        {
            g_recorder.second_transition_result = FSM_State_Transition(g_clock.fsm, 1U) ? 1 : 0;
        }
    }
}

static void state1_exit(void)
{
    g_recorder.state1_exit_count += 1;
    record_event(13);
}

static void state2_entry(void)
{
    g_recorder.state2_entry_count += 1;
    record_event(21);
}

static void state2_action(void)
{
    g_recorder.state2_action_count += 1;
    record_event(22);
}

static void state2_exit(void)
{
    g_recorder.state2_exit_count += 1;
    record_event(23);
}

static FSM_Structure* create_fsm(FMS_MEMORY* memory)
{
    FSM_Structure* fsm = FSM_Create(memory);
    assert(fsm != NULL);
    return fsm;
}

static FSM_Function make_callbacks(void (*action)(void), void (*entry)(void), void (*exit)(void))
{
    FSM_Function callbacks;
    callbacks.action = action;
    callbacks.entry = entry;
    callbacks.exit = exit;
    return callbacks;
}

static void test_start_entry_action(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    const int expected_sequence[] = {11, 12};

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, state1_entry, state1_exit)));
    assert(FSM_Start(fsm, 1U));

    reset_software_clock(fsm, 5U);
    run_software_clock(1U);

    assert(g_recorder.state1_entry_count == 1);
    assert(g_recorder.state1_action_count == 0);

    /* 第二拍后进入稳定态 action。 */
    run_software_clock(1U);
    assert(g_recorder.state1_action_count == 1);
    assert_sequence(expected_sequence, sizeof(expected_sequence) / sizeof(expected_sequence[0]));

    FSM_Destroy(fsm);
}

static void test_nullable_entry_exit(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    const int expected_sequence[] = {12};

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, NULL, NULL)));
    assert(FSM_Start(fsm, 1U));

    reset_software_clock(fsm, 5U);
    run_software_clock(2U);

    assert(g_recorder.state1_entry_count == 0);
    assert(g_recorder.state1_exit_count == 0);
    assert(g_recorder.state1_action_count == 1);
    assert_sequence(expected_sequence, sizeof(expected_sequence) / sizeof(expected_sequence[0]));

    FSM_Destroy(fsm);
}

static void test_transition_order(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    const int expected_sequence[] = {11, 12, 13, 21, 22};

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, state1_entry, state1_exit)));
    assert(FSM_Add_State(fsm, 2U, make_callbacks(state2_action, state2_entry, state2_exit)));
    assert(FSM_Start(fsm, 1U));

    reset_software_clock(fsm, 5U);
    g_clock.transition_at_ms = 5U;
    run_software_clock(5U);

    assert(g_recorder.first_transition_result == 1);
    assert_sequence(expected_sequence, sizeof(expected_sequence) / sizeof(expected_sequence[0]));

    FSM_Destroy(fsm);
}

static void test_queue_single_slot(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, NULL, NULL)));
    assert(FSM_Add_State(fsm, 2U, make_callbacks(state2_action, NULL, NULL)));
    assert(FSM_Start(fsm, 1U));

    reset_software_clock(fsm, 5U);
    g_clock.transition_at_ms = 5U;
    g_clock.attempt_double_transition = 1;
    run_software_clock(2U);

    assert(g_recorder.first_transition_result == 1);
    assert(g_recorder.second_transition_result == 0);

    FSM_Destroy(fsm);
}

static void test_enable_window(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    const int expected_sequence[] = {12, 12};

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, NULL, NULL)));
    assert(FSM_Start(fsm, 1U));

    reset_software_clock(fsm, 5U);
    g_clock.disable_at_ms = 5U;
    g_clock.enable_at_ms = 20U;
    run_software_clock(6U);

    /* 禁用窗口内 FSM_Process 为 no-op，恢复后 action 再继续累计。 */
    assert(g_recorder.state1_action_count == 2);
    assert_sequence(expected_sequence, sizeof(expected_sequence) / sizeof(expected_sequence[0]));

    FSM_Destroy(fsm);
}

static void test_invalid_inputs(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    FSM_Function invalid_callbacks = make_callbacks(NULL, NULL, NULL);

    fsm = create_fsm(&memory);

    assert(!FSM_Add_State(fsm, 1U, invalid_callbacks));
    assert(!FSM_Start(fsm, 1U));
    assert(!FSM_State_Transition(fsm, 1U));
    assert(FSM_Create(NULL) == NULL);
    assert(!FSM_Add_State(NULL, 1U, invalid_callbacks));
    assert(!FSM_Start(NULL, 1U));
    assert(!FSM_State_Transition(NULL, 1U));
    FSM_Process(NULL);
    FSM_Set_Enable(NULL, true);
    FSM_Destroy(NULL);

    FSM_Destroy(fsm);
}

typedef void (*TestFunction)(void);

typedef struct
{
    int mask;
    const char* name;
    TestFunction function;
} TestCase;

static const TestCase kTests[] = {
    {TEST_START_ENTRY_ACTION, "start_entry_action", test_start_entry_action},
    {TEST_NULLABLE_ENTRY_EXIT, "nullable_entry_exit", test_nullable_entry_exit},
    {TEST_TRANSITION_ORDER, "transition_order", test_transition_order},
    {TEST_QUEUE_SINGLE_SLOT, "queue_single_slot", test_queue_single_slot},
    {TEST_ENABLE_WINDOW, "enable_window", test_enable_window},
    {TEST_INVALID_INPUTS, "invalid_inputs", test_invalid_inputs},
};

static int parse_mask(const char* arg)
{
    char* end = NULL;
    long value;

    if (arg == NULL || strcmp(arg, "all") == 0)
    {
        return TEST_ALL;
    }

    if (strcmp(arg, "list") == 0)
    {
        return -1;
    }

    value = strtol(arg, &end, 0);
    if (end == arg || *end != '\0' || value < 0 || value > TEST_ALL)
    {
        fprintf(stderr, "invalid test mask: %s\n", arg);
        return -2;
    }

    return (int)value;
}

int main(int argc, char* argv[])
{
    int mask = TEST_ALL;
    size_t index;
    int run_count = 0;

    if (argc > 1)
    {
        mask = parse_mask(argv[1]);
        if (mask == -1)
        {
            for (index = 0; index < sizeof(kTests) / sizeof(kTests[0]); ++index)
            {
                printf("0x%02X %s\n", kTests[index].mask, kTests[index].name);
            }
            return 0;
        }
        if (mask < 0)
        {
            return 2;
        }
    }

    for (index = 0; index < sizeof(kTests) / sizeof(kTests[0]); ++index)
    {
        if ((mask & kTests[index].mask) == 0)
        {
            continue;
        }
        kTests[index].function();
        printf("[PASS] %s\n", kTests[index].name);
        run_count += 1;
    }

    if (run_count == 0)
    {
        fprintf(stderr, "no tests selected\n");
        return 3;
    }

    printf("finite_state_machine tests passed (%d selected)\n", run_count);
    return 0;
}
