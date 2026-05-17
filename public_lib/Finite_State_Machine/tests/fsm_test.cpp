#include "Finite_State_Machine.h"

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
    TEST_DATA_PACKAGE_FIFO = 1 << 4,
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
} CallbackRecorder;

static CallbackRecorder g_recorder;

static void reset_recorder(void)
{
    memset(&g_recorder, 0, sizeof(g_recorder));
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

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, state1_entry, state1_exit)));
    assert(FSM_Start(fsm, 1U));

    FSM_Process(fsm);
    assert(g_recorder.state1_entry_count == 1);
    assert(g_recorder.state1_action_count == 0);

    FSM_Process(fsm);
    assert(g_recorder.state1_action_count == 1);
    assert(g_recorder.sequence_count == 2);
    assert(g_recorder.sequence[0] == 11);
    assert(g_recorder.sequence[1] == 12);

    FSM_Destroy(fsm);
}

static void test_nullable_entry_exit(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, NULL, NULL)));
    assert(FSM_Start(fsm, 1U));

    FSM_Process(fsm);
    FSM_Process(fsm);

    assert(g_recorder.state1_entry_count == 0);
    assert(g_recorder.state1_exit_count == 0);
    assert(g_recorder.state1_action_count == 1);
    assert(g_recorder.sequence_count == 1);
    assert(g_recorder.sequence[0] == 12);

    FSM_Destroy(fsm);
}

static void test_transition_order(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;

    reset_recorder();
    fsm = create_fsm(&memory);

    assert(FSM_Add_State(fsm, 1U, make_callbacks(state1_action, state1_entry, state1_exit)));
    assert(FSM_Add_State(fsm, 2U, make_callbacks(state2_action, state2_entry, state2_exit)));
    assert(FSM_Start(fsm, 1U));

    FSM_Process(fsm);
    FSM_Process(fsm);
    assert(FSM_State_Transition(fsm, 2U));

    FSM_Process(fsm);
    FSM_Process(fsm);
    FSM_Process(fsm);

    assert(g_recorder.sequence_count == 5);
    assert(g_recorder.sequence[0] == 11);
    assert(g_recorder.sequence[1] == 12);
    assert(g_recorder.sequence[2] == 13);
    assert(g_recorder.sequence[3] == 21);
    assert(g_recorder.sequence[4] == 22);

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

    FSM_Process(fsm);
    assert(FSM_State_Transition(fsm, 2U));
    assert(!FSM_State_Transition(fsm, 1U));

    FSM_Destroy(fsm);
}

static void test_data_package_fifo(void)
{
    FMS_MEMORY memory = {0};
    FSM_Structure* fsm;
    DataPackage in_first = {0};
    DataPackage in_second = {0};
    DataPackage out = {0};

    fsm = create_fsm(&memory);

    in_first.target_ID = 1U;
    in_first.data.data_u32 = 0x12345678U;
    in_first.data_size = sizeof(in_first.data.data_u32);

    in_second.target_ID = 2U;
    in_second.data.data_i16[0] = -12;
    in_second.data.data_i16[1] = 34;
    in_second.data_size = sizeof(in_second.data.data_i16);

    assert(FSM_Push_Data_Package(fsm, in_first));
    assert(FSM_Push_Data_Package(fsm, in_second));

    assert(FSM_Get_Data_Package(fsm, &out));
    assert(out.target_ID == 1U);
    assert(out.data.data_u32 == 0x12345678U);
    assert(out.data_size == sizeof(in_first.data.data_u32));

    assert(FSM_Get_Data_Package(fsm, &out));
    assert(out.target_ID == 2U);
    assert(out.data.data_i16[0] == -12);
    assert(out.data.data_i16[1] == 34);
    assert(out.data_size == sizeof(in_second.data.data_i16));

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
    assert(!FSM_Push_Data_Package(NULL, (DataPackage){0}));
    assert(!FSM_Get_Data_Package(NULL, NULL));
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
    {TEST_DATA_PACKAGE_FIFO, "data_package_fifo", test_data_package_fifo},
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
