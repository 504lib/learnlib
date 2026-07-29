#include "static_stack.h"

#include <assert.h>
#include <stdio.h>

DECLARE_STATIC_STACK(INT, int, 64);
DECLARE_STATIC_STACK(ONE, int, 1);

static void test_empty_stack_behaviour(void)
{
    INT_t stack;
    int value = -1;

    INT_INIT(&stack);

    assert(INT_IS_EMPTY(&stack));
    assert(INT_SIZE(&stack) == 0U);
    assert(!INT_POP(&stack, &value));
    assert(!INT_TOP(&stack, &value));
}

static void test_push_and_lifo_order(void)
{
    INT_t stack;
    int value = -1;

    INT_INIT(&stack);

    for (int index = 0; index < 64; ++index)
    {
        assert(INT_PUSH(&stack, index));
    }

    assert(INT_IS_FULL(&stack));
    assert(INT_SIZE(&stack) == 64U);
    assert(!INT_PUSH(&stack, 100));
    assert(INT_TOP(&stack, &value));
    assert(value == 63);

    for (int expected = 63; expected >= 0; --expected)
    {
        assert(INT_POP(&stack, &value));
        assert(value == expected);
    }

    assert(INT_IS_EMPTY(&stack));
}

static void test_top_does_not_change_size(void)
{
    INT_t stack;
    size_t size_before;
    int value = -1;

    INT_INIT(&stack);
    assert(INT_PUSH(&stack, 11));
    assert(INT_PUSH(&stack, 22));

    size_before = INT_SIZE(&stack);
    assert(INT_PEEK(&stack, &value));
    assert(value == 22);
    assert(INT_SIZE(&stack) == size_before);
}

static void test_clear_and_reuse(void)
{
    INT_t stack;
    int value = -1;

    INT_INIT(&stack);
    assert(INT_PUSH(&stack, 1));
    assert(INT_PUSH(&stack, 2));
    INT_CLEAR(&stack);

    assert(INT_IS_EMPTY(&stack));
    assert(INT_SIZE(&stack) == 0U);
    assert(!INT_POP(&stack, &value));

    assert(INT_PUSH(&stack, 99));
    assert(INT_POP(&stack, &value));
    assert(value == 99);
}

static void test_capacity_one_stack(void)
{
    ONE_t stack;
    int value = -1;

    ONE_INIT(&stack);
    assert(ONE_PUSH(&stack, 7));
    assert(ONE_IS_FULL(&stack));
    assert(!ONE_PUSH(&stack, 8));
    assert(ONE_TOP(&stack, &value));
    assert(value == 7);
    assert(ONE_POP(&stack, &value));
    assert(value == 7);
    assert(ONE_IS_EMPTY(&stack));
}

static void test_stress_rounds(void)
{
    INT_t stack;
    int value = -1;

    INT_INIT(&stack);

    for (int round = 0; round < 3; ++round)
    {
        for (int index = 0; index < 64; ++index)
        {
            assert(INT_PUSH(&stack, round * 1000 + index));
        }

        for (int index = 63; index >= 0; --index)
        {
            assert(INT_POP(&stack, &value));
            assert(value == round * 1000 + index);
        }

        assert(INT_IS_EMPTY(&stack));
    }
}

int main(void)
{
    test_empty_stack_behaviour();
    test_push_and_lifo_order();
    test_top_does_not_change_size();
    test_clear_and_reuse();
    test_capacity_one_stack();
    test_stress_rounds();

    printf("static_stack tests passed\n");
    return 0;
}