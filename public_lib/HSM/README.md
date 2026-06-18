# HSM 分层状态机

## 概述

HSM (Hierarchical State Machine) 是一个面向嵌入式平台的 C 语言分层状态机库。C++ 实现，对外暴露 C 接口。

核心机制：
- **冒泡 (Bubbling)**：子节点不处理的事件自动向上冒泡到父节点
- **LCA 转换**：状态切换时自动执行 exit/entry 链，通过最近公共祖先 (LCA) 计算最小动作集
- **延迟转换**：转换请求入队，在 `Process()` 中统一执行，避免重入

## 编译

```
g++ -std=c++11 -I. -Istatic_stack -Istatic_queue \
    your_code.cpp HSM_Core.cpp -o your_app
```

可选：`-DNO_LOG_ASSERT` 禁用断言。

## 内存模型

所有内存由调用方静态分配，类型化存储编译器自动保证对齐，无需手动对齐宏：

```c
HSM_Core_Memory core_mem;        // HSM 实例存储
HSM_Node        nodes[16];       // 状态节点池
```

可配置宏：

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `HSM_CORE_STORAGE_SIZE` | 1024 | HSM 实例存储大小 |
| `HSM_MAX_NODES` | 16 | 单个 HSM 最大状态节点数 |
| `HSM_MAX_STACK_DEPTH` | 16 | 最大状态嵌套深度 |
| `HSM_EVENT_PAYLOAD_SIZE` | 16 | 事件数据载荷大小 |

> `HSM_CORE_STORAGE_SIZE` 被 `static_assert` 验证，不够时编译期提示。

## API 参考

### 事件

```c
#define HSM_EVENT_NULL 0xFF

typedef struct HSM_Event_Package {
    uint8_t HSM_Event_ID;                       // 事件 ID
    uint8_t data[HSM_EVENT_PAYLOAD_SIZE];       // 载荷
} HSM_Event_Package;
```

`HSM_EVENT_NULL` 用作占位事件，用户事件 ID 从 0 起不冲突。

### 字节解析 (static inline, 零依赖)

从 `HSM_Event_Package.data` 中读取/写入小端序多字节值：

```c
// 读取
uint8_t  v_u8  = HSM_ReadU8(buf);
int16_t  v_i16 = HSM_ReadI16LE(buf);
uint32_t v_u32 = HSM_ReadU32LE(buf);
float    v_f32 = HSM_ReadF32LE(buf);

// 写入
uint8_t buf[HSM_EVENT_PAYLOAD_SIZE];
HSM_WriteF32LE(buf,      3.14f);
HSM_WriteI16LE(buf + 4,  500);
HSM_WriteU8 (buf + 6,    0x01);
```

| 函数 | 字节数 |
|------|--------|
| `HSM_ReadU8` / `HSM_WriteU8` | 1 |
| `HSM_ReadI8` / `HSM_WriteI8` | 1 |
| `HSM_ReadU16LE` / `HSM_WriteU16LE` | 2 |
| `HSM_ReadI16LE` / `HSM_WriteI16LE` | 2 |
| `HSM_ReadU32LE` / `HSM_WriteU32LE` | 4 |
| `HSM_ReadI32LE` / `HSM_WriteI32LE` | 4 |
| `HSM_ReadF32LE` / `HSM_WriteF32LE` | 4 |

全 `static inline`，字节移位实现，零调用开销，不依赖 `<string.h>`。

### 创建与销毁

```c
HSM* HSM_Create(HSM_Core_Memory* mem, HSM_Node nodes[], size_t max_nodes,
                const HSM_StateDef states[], size_t count);
void HSM_Destroy(HSM* hsm);
HSM_Node* HSM_FindNode(HSM* hsm, const char* name);
```

`HSM_Create` 在用户提供的存储上 placement-new，根据 `HSM_StateDef` 表自动初始化所有节点并建立父子树。任何参数为 NULL 或建树失败返回 NULL。

`HSM_FindNode` 按名字查找节点，未找到返回 NULL。

### 状态定义

```c
typedef struct HSM_StateDef {
    const char* name;                            // 状态名（FindNode / 日志用）
    const char* parent_name;                     // 父状态名（根节点为 NULL）
    bool (*handler)(HSM_Event_Package event);
    void (*entry_action)(HSM_Event_Package event);
    void (*exit_action)(HSM_Event_Package event);
    void (*continuous_action)(void);
} HSM_StateDef;

#define HSM_STATE_DEF(name, parent, handler, entry, exit, continuous) \
    (HSM_StateDef){ name, parent, handler, entry, exit, continuous }
```

一张 `HSM_StateDef[]` 表定义整棵状态树，`HSM_Create` 内部两遍完成：遍一初始化节点回调，遍二按 name→parent_name 建立父子链接。

### 运行

```c
void HSM_Start(HSM* hsm, HSM_Node* initial_state);
void HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event);
void HSM_Process(HSM* hsm);
void HSM_SetEnable(HSM* hsm, bool enable);
```

- `HSM_Start`：从初始状态到根节点执行所有 `entry_action`，内部调用 `HSM_SetEnable(true)`
- `HSM_Process`：出队并执行所有待处理转换，然后调用当前状态的 `continuous_action`
- `HSM_SetEnable`：禁用时 `SendEvent` 和 `Process` 静默返回

### 事件与转换

```c
void HSM_SendEvent(HSM* hsm, HSM_Event_Package event);
bool HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event);
```

执行模型：

```
SendEvent(event)                  RequestTransition(target, event)
  ├→ Dispatch 冒泡                  ├→ 入队
  ├→ 调 handler(event)             └→ 返回
  └→ 出队执行所有待处理转换

Process()
  ├→ 出队执行所有待处理转换
  └→ continuous_action()
```

**handler 返回值**：`true` 表示处理了该事件（停止冒泡），`false` 表示不处理（继续向 parent 冒泡）。

**转换规则**：`Transition(current, target)` 执行：
1. 计算 `LCA(current, target)`
2. 从 current 向上到 LCA（不含），执行 `exit_action`
3. 从 LCA 向下到 target，执行 `entry_action`

## 使用示例

以送药小车为例，状态树：

```
Root
├── Idle
│   └── Running
│       ├── Normal ──(sibling)── Avoiding
│       └── ...
└── Charging
```

### 1. 定义事件和状态

```c
enum {
    EV_START    = 1,
    EV_STOP     = 2,
    EV_CHARGE   = 3,
    EV_OBSTACLE = 4,
    EV_CLEAR    = 5,
};
```

### 2. 定义回调

```c
static bool h_idle(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_START; }
static bool h_running(HSM_Event_Package e)  { return e.HSM_Event_ID == EV_STOP; }
static bool h_normal(HSM_Event_Package e)   { return e.HSM_Event_ID == EV_OBSTACLE; }
static bool h_avoiding(HSM_Event_Package e) { return e.HSM_Event_ID == EV_CLEAR; }
static bool h_charging(HSM_Event_Package e) { return e.HSM_Event_ID == EV_START; }

static void e_idle(HSM_Event_Package)     { /* 进入空闲 */ }
static void e_running(HSM_Event_Package)  { /* 开始运行 */ }
static void e_normal(HSM_Event_Package)   { /* 正常行驶 */ }
static void e_avoiding(HSM_Event_Package) { /* 进入避障 */ }
static void ex_avoiding(HSM_Event_Package){ /* 退出避障 */ }
static void e_charging(HSM_Event_Package) { /* 开始充电 */ }
static void ex_charging(HSM_Event_Package){ /* 停止充电 */ }

static void heartbeat() { /* 每帧心跳 */ }
```

### 3. 构建状态树

```c
static HSM_Core_Memory core_mem;
static HSM_Node        nodes[HSM_MAX_NODES];

HSM_StateDef states[] = {
    HSM_STATE_DEF("root",     NULL,       NULL,       NULL,         NULL,          NULL),
    HSM_STATE_DEF("idle",     "root",     h_idle,     e_idle,       NULL,          heartbeat),
    HSM_STATE_DEF("charging", "root",     h_charging, e_charging,   ex_charging,   NULL),
    HSM_STATE_DEF("running",  "idle",     h_running,  e_running,    NULL,          NULL),
    HSM_STATE_DEF("normal",   "running",  h_normal,   e_normal,     NULL,          heartbeat),
    HSM_STATE_DEF("avoiding", "running",  h_avoiding, e_avoiding,   ex_avoiding,   NULL),
};

HSM* hsm = HSM_Create(&core_mem, nodes, HSM_MAX_NODES, states, 6);
```

对比老 API 三步走（`HSM_Node_Create` → 填 `HSM_Node_Param` → `HSM_RegisterChild`），新 API 一个状态一行，一张表建整棵树。

### 4. 运行主循环

```c
HSM_Node* idle = HSM_FindNode(hsm, "idle");   // 或 &nodes[1] 零开销
HSM_Node* normal   = &nodes[4];
HSM_Node* avoiding = &nodes[5];
HSM_Node* charging = &nodes[2];

HSM_Start(hsm, idle);

while (1) {
    if (obstacle_detected) {
        HSM_SendEvent(hsm,   (HSM_Event_Package){ .HSM_Event_ID = EV_OBSTACLE });
        HSM_RequestTransition(hsm, avoiding, (HSM_Event_Package){ .HSM_Event_ID = EV_OBSTACLE });
    }
    if (obstacle_cleared) {
        HSM_SendEvent(hsm,   (HSM_Event_Package){ .HSM_Event_ID = EV_CLEAR });
        HSM_RequestTransition(hsm, normal, (HSM_Event_Package){ .HSM_Event_ID = EV_CLEAR });
    }
    if (battery_low) {
        HSM_RequestTransition(hsm, charging, (HSM_Event_Package){ .HSM_Event_ID = EV_CHARGE });
    }

    HSM_Process(hsm);
}
```

> **性能提示**：`&nodes[i]` 是 O(1) 零开销寻址，`HSM_FindNode` 适合调试/启动。

## 树结构约束

1. **必须有公共根节点**：所有状态节点必须是同一棵树的成员，否则 LCA 找不到返回 NULL
2. **注册在 Create 时完成**：`HSM_Create` 根据 `parent_name` 自动建树，无需手动注册
3. **根节点无 parent_name**：表中第一个 `parent_name == NULL` 的节点作为树根

## 回调时机

| 回调 | 触发时机 |
|------|---------|
| `handler` | `SendEvent` 时自底向上冒泡，返回 true 停止 |
| `entry_action` | `Start` 时（初始状态路径）/ `Transition` 时（LCA→target 路径） |
| `exit_action` | `Transition` 时（current→LCA 路径） |
| `continuous_action` | 每次 `Process()` |

## HSM_Bus — 事件总线

解决多维度行为交织导致的状态枚举爆炸。多个小 HSM 各自独立，通过 Bus 广播事件。

### API

```c
#include "HSM_Bus.h"

#define HSM_BUS_MAX 8        // 默认，可覆盖

typedef struct HSM_Bus {
    HSM* hsms[HSM_BUS_MAX];
    int  count;
} HSM_Bus;

void HSM_Bus_Add       (HSM_Bus* bus, HSM* hsm);
void HSM_Bus_SendEvent (HSM_Bus* bus, HSM_Event_Package ev);
void HSM_Bus_Process   (HSM_Bus* bus);
```

`HSM_Bus_Add` 在启动前注册 HSM 实例，容量由 `HSM_BUS_MAX` 控制。`HSM_Bus_SendEvent` 遍历广播到所有注册的 HSM，`HSM_Bus_Process` 遍历驱动所有 HSM 的 `continuous_action`。

### 示例 — 运动 × 通讯 × 电量

老方案：单 HSM 承载三维行为，笛卡尔积 `4×3×3 = 36` 个枚举。Bus 方案：

```c
// 三个独立的小 HSM，各管一个维度
HSM* motion = HSM_Create(&mem1, nodes1, 8, motion_states, 4);   // 4 状态
HSM* radio  = HSM_Create(&mem2, nodes2, 8, radio_states, 3);    // 3 状态
HSM* power  = HSM_Create(&mem3, nodes3, 8, power_states, 3);    // 3 状态
// 总共 10 个状态，不是 36 个

HSM_Bus bus = {0};
HSM_Bus_Add(&bus, motion);
HSM_Bus_Add(&bus, radio);
HSM_Bus_Add(&bus, power);

// 广播事件 — 三个 HSM 独立处理，互不阻塞
HSM_Bus_SendEvent(&bus, (HSM_Event_Package){ .HSM_Event_ID = EV_TICK });
```

跨维度协调：直接读状态 + 发送事件到特定 HSM。

```c
if (HSM_FindNode(power, "low")) {              // 电量低
    HSM_SendEvent(motion, (HSM_Event_Package){ .HSM_Event_ID = EV_GO_CHARGE });
}
```

### 与正交区域 (AND-state) 的对比

| | HSM_Bus | 正交区域 |
|------|---------|----------|
| 实现 | 指针数组 + for 循环 | region 链表 |
| 独立性 | 完全独立的小 HSM | 耦合在同一棵状态树 |
| 事件路由 | 广播 | 需指定 region |
| 代码量 | ~30 行 | ~200+ 行 |
| 零动态分配 | ✓ | 同 |
| 独立测试 | 各维度可单独测 | 需整体测 |
