#include <stdio.h>
#include "./PID_Node.h"

int main()
{
    PID_Link testLink;
    PID_Link_Init(&testLink);

    // 创建两个PID节点
    PID_Node node1, node2;
    PID_Node_Init(&node1, "Node1", 1.0f, 0.5f, 0.1f);
    PID_Node_Init(&node2, "Node2", 0.8f, 0.3f, 0.05f);

    // 设置节点参数
    PID_Node_SetSetpoint(&node1, 10.0f);
    PID_Node_SetSetpoint(&node2, 5.0f);
    PID_Node_SetMaxOutput(&node1, 100.0f);
    PID_Node_SetMinOutput(&node1, 0.0f);
    PID_Node_SetMaxOutput(&node2, 50.0f);
    PID_Node_SetMinOutput(&node2, -50.0f);

    // 添加到链表
    PID_Link_AddNode(&testLink, &node1);
    PID_Link_AddNode(&testLink, &node2);

    // 模拟输入和运行
    float input = 2.0f;
    float dt = 1.0f;
    for (int i = 0; i < 10; ++i)
    {
        printf("\n=== Step %d ===\n", i+1);
        PID_Node_Update(&testLink, input, dt);
        PID_Node* cur = testLink.head;
        int idx = 1;
        while (cur)
        {
            printf("Node%d: setpoint=%.2f, measured=%.2f, output=%.2f\n", idx, cur->setpoint, cur->measured_value, cur->output);
            cur = cur->next;
            idx++;
        }
        // 可选：将下一个循环的输入设置为上一个尾节点的输出
        input = testLink.tail->output;
    }

    return 0;
}