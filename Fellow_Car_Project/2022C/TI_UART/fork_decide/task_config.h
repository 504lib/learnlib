#pragma once

#include "../grayscale/grayscale.h"
#include <stdbool.h>

// ============================================================
// Task 参数集中管理
// ============================================================

typedef struct {
    float    base_speed;        // 巡线速度 m/s
    float    overtake_speed;    // 超车速度(0=不用), m/s
    int      finish_count;      // 第几次全黑算终点
    int      fork_timeout_ms;   // FORK超时(ms)
} TaskConfig;

static const TaskConfig task_configs[] = {
    { 0.00f, 0.00f, 0,    0 },  // [0] 占位
    { 0.30f, 0.00f, 2, 1000 },  // 题1: 外圈1圈
    { 0.50f, 0.00f, 4,  600 },  // 题2: 外圈2圈
    { 0.30f, 0.36f, 4,  900 },  // 题3: 3圈=4次全黑, 外圈0.3内圈0.42超车
    { 1.00f, 0.00f, 2,  400 },  // 题4: 外圈1圈
};

#define TASK_COUNT (sizeof(task_configs) / sizeof(task_configs[0]))

static inline const TaskConfig* TaskConfig_Get(uint8_t task)
{
    if (task >= 1 && task < (uint8_t)TASK_COUNT)
        return &task_configs[task];
    return &task_configs[0];
}

// ============================================================
// 动态权值: Task 3 不同全黑次数走不同方向
// 3圈共4次全黑: black_count=1岔路1, =2岔路2, =3岔路3, =4终点
// ============================================================
static inline ForkMode TaskConfig_GetForkMode(uint8_t task, int black_count, bool is_lead)
{
    if (task != 3) {
        return FORK_MODE_STRAIGHT; // Task 1/2/4 全外圈
    }
    switch (black_count) {
        case 1: return FORK_MODE_STRAIGHT;                            // Lap1 两车外圈
        case 2: return is_lead ? FORK_MODE_STRAIGHT : FORK_MODE_LEFT; // Lap2 跟随内圈超车
        case 3: return is_lead ? FORK_MODE_LEFT : FORK_MODE_STRAIGHT; // Lap3 领头内圈反超
        default:return FORK_MODE_STRAIGHT;                            // black_count=4终点不触发FORK
    }
}

// ============================================================
// 超车检测阈值
// ============================================================
#define OVERTAKE_DETECT_MM  400   // 40cm, 超车完成后恢复距离PID
