#include <stdio.h>
#include "./PID_Node.h"

void printall(PID_Node* node)
{
    printf("Node: %s, setpoint=%.2f, measured=%.2f, output=%.2f, output_range:[%.2f,%.2f]\n",
           node->name, node->setpoint, node->measured_value, node->output,
           node->limit.output_min, node->limit.output_max);
}

int main()
{
    PID_Link testLink;
    PID_Link_Init(&testLink);

    // 创建两个PID节点（外环位置，内环速度）
    PID_Node pos_node, vel_node;
    PID_Node_Init(&vel_node, "Velocity", 0.8f, 0.00001f, 0.05f);
    PID_Node_Init(&pos_node, "Position", 0.5f, 0.002f, 0.0f);

    // 设置节点参数
    PID_Node_SetSetpoint(&pos_node, -10.0f);       // 目标位置

    // 配置外环限制（输出范围对应速度修正量，例如 -50~50）
    PID_Node_SetLimit(&pos_node, (PID_Limit){
        .setpoint_min = -1000.0f, .setpoint_max = 1000.0f,
        .input_min = -1000.0f, .input_max = 1000.0f,
        .output_min = -50.0f, .output_max = 50.0f,
        .integral_max = 100.0f, .derivative_max = 100.0f, .deadband = 0.0f
    });

    // 配置内环限制
    PID_Node_SetLimit(&vel_node, (PID_Limit){
        .setpoint_min = -100.0f, .setpoint_max = 100.0f,
        .input_min = -1000.0f, .input_max = 1000.0f,
        .output_min = -100.0f, .output_max = 100.0f,
        .integral_max = 200.0f, .derivative_max = 200.0f, .deadband = 0.0f
    });

    // 启用内环的基准值（基础速度），并设置基础速度为 20.0
    // 这样内环的最终设定值 = 外环输出（位置环输出的速度修正量） + 20.0
    PID_Node_SetUserBaseValue(&vel_node, true, 1.0f);

    // 添加到链表（顺序：外环先，内环后）
    PID_Link_AddNode(&testLink, &pos_node);
    PID_Link_AddNode(&testLink, &vel_node);

    // 模拟控制循环
    float actual_position = 0.0f;   // 实际位置
    float actual_velocity = 0.0f;   // 实际速度
    float dt = 0.01f;               // 10ms 控制周期

    for (int i = 0; i < 500; ++i)   // 模拟5秒
    {
        // 1. 更新测量值（从传感器读取）
        PID_Node_UpdateMeasurement(&pos_node, actual_position);
        PID_Node_UpdateMeasurement(&vel_node, actual_velocity);

        // 2. 执行串级PID计算（外环输出 + 基准值 自动成为内环设定值）
        PID_Node_Link_Update(&testLink, dt);

        // 3. 将内环输出应用到执行器（这里模拟速度变化）
        float pwm = vel_node.output;
        actual_velocity += pwm * dt;
        actual_position += actual_velocity * dt;

        // 打印关键步骤
        if (i % 100 == 0) {
            printf("\n=== Step %d ===\n", i);
            printf("Position: setpoint=%.2f, measured=%.2f, output=%.2f\n",
                   pos_node.setpoint, pos_node.measured_value, pos_node.output);
            printf("Velocity: setpoint=%.2f (base=%.2f + offset=%.2f), measured=%.2f, output=%.2f\n",
                   vel_node.setpoint, 20.0f, pos_node.output, vel_node.measured_value, vel_node.output);
        }
    }

    printf("\n=== Final Node States ===\n");
    PID_AllNode(&testLink, printall);
    return 0;
}