#include "static_queue.h"
#include <stdio.h>

// 定义两个队列类型：容量 64 的 int 队列，容量 1 的 int 队列
DECLARE_STATIC_QUEUE(INT,int,64);
DECLARE_STATIC_QUEUE(ONE,int,1);
// DECLARE_STATIC_QUEUE(CHAR,char,0);

int main()
{
    // 创建并初始化队列实例
    INT_t queue;
    INT_INIT(&queue);

    // 边界：空队列 POP/PEEK/BACK（应全部失败）
    int value = -1;
    if (!INT_POP(&queue, &value))
    {
        printf("OK: empty POP rejected\n");
    }
    if (!INT_PEEK(&queue, &value))
    {
        printf("OK: empty PEEK rejected\n");
    }
    if (!INT_BACK(&queue, &value))
    {
        printf("OK: empty BACK rejected\n");
    }

    // 边界：恰好填满（容量为 64）
    for (int i = 0; i < 64; i++)
    {
        if (!INT_PUSH(&queue, i))
        {
            printf("ERROR: push failed at %d\n", i);
        }
    }
    if (!INT_IS_FULL(&queue))
    {
        printf("ERROR: queue should be full\n");
    }
    if (INT_SIZE(&queue) != 64)
    {
        printf("ERROR: size should be 64, got %zu\n", INT_SIZE(&queue));
    }

    // 边界：满队列 PUSH（应失败）
    if (!INT_PUSH(&queue, 99))
    {
        printf("OK: full PUSH rejected\n");
    }

    // 边界：环回（出队 5 个，再入队 5 个）
    for (int i = 0; i < 5; i++)
    {
        INT_POP(&queue, &value);
    }
    for (int i = 100; i < 105; i++)
    {
        if (!INT_PUSH(&queue, i))
        {
            printf("ERROR: wrap push failed at %d\n", i);
        }
    }

    // 边界：PEEK/BACK 不改变 SIZE
    size_t size_before = INT_SIZE(&queue);
    int front = -1;
    int back = -1;
    if (INT_PEEK(&queue, &front) && INT_BACK(&queue, &back))
    {
        if (INT_SIZE(&queue) != size_before)
        {
            printf("ERROR: size changed after PEEK/BACK\n");
        }
    }

    // 顺序校验：输出剩余元素顺序
    printf("Sequence:\n");
    while (!INT_IS_EMPTY(&queue))
    {
        INT_POP(&queue, &value);
        printf("%d ", value);
    }
    printf("\n");

    // 边界：清空后 SIZE/IS_EMPTY 一致性
    if (!INT_IS_EMPTY(&queue) || INT_SIZE(&queue) != 0)
    {
        printf("ERROR: empty invariants broken\n");
    }

    // 环回极限：多轮 push/pop（压力测试）
    for (int round = 0; round < 3; round++)
    {
        for (int i = 0; i < 64; i++)
        {
            if (!INT_PUSH(&queue, i + round * 1000))
            {
                printf("ERROR: stress push failed at round %d, i=%d\n", round, i);
            }
        }
        for (int i = 0; i < 64; i++)
        {
            if (!INT_POP(&queue, &value))
            {
                printf("ERROR: stress pop failed at round %d, i=%d\n", round, i);
            }
        }
        if (!INT_IS_EMPTY(&queue))
        {
            printf("ERROR: stress round not empty %d\n", round);
        }
    }

    // 容量为 1 的队列：满队列 PUSH 校验
    ONE_t one;
    ONE_INIT(&one);
    ONE_PUSH(&one, 7);
    if (!ONE_PUSH(&one, 8))
    {
        printf("OK: capacity-1 full PUSH rejected\n");
    }
    ONE_POP(&one, &value);
    printf("capacity-1 pop: %d\n", value);

    // 误用测试（会触发断言，默认不启用）
    // INT_PUSH(NULL, 1);
    // INT_POP(&queue, NULL);
    // INT_INIT(&queue); // 重复 INIT

    return 0;
}