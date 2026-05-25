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

可选：`-DNO_LOG_ASSERT` 禁用断言，`-include ../Log/Log.h` 启用日志。

## 内存模型

所有内存由调用方静态分配，库不调用 `malloc`/`free`：

```c
FMS_OBJECT_MEMORY  hsm_memory;   // 状态机实例
FMS_NODE_MEMORY    node_memory[8]; // 状态节点
```

存储大小可通过宏调整：

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `HSM_OBJECT_STORAGE_SIZE` | 1024 | HSM 实例大小 |
| `HSM_NODE_STORAGE_SIZE` | 80 | 单个节点大小 |
| `HSM_MAX_STACK_DEPTH` | 16 | 最大状态嵌套深度 |
| `HSM_EVENT_PAYLOAD_SIZE` | 16 | 事件数据载荷大小 |

## API 参考

### 创建与销毁

```c
HSM*      HSM_Create(FMS_OBJECT_MEMORY* memory);
HSM_Node* HSM_Node_Create(FMS_NODE_MEMORY* memory);
void      HSM_Destroy(HSM* hsm);
```

`HSM_Create` / `HSM_Node_Create` 在用户提供的存储上 placement-new，传入 NULL 返回 NULL。`HSM_Destroy` 调用析构函数但不释放内存。

### 构建状态树

```c
bool HSM_RegisterChild(HSM* hsm, HSM_Node* parent, HSM_Node_Param child);
bool HSM_RegisterChildNodes(HSM* hsm, HSM_Node* parent,
                            HSM_Node_Param children[], size_t count);
```

- 首次将节点作为 parent 时自动初始化该节点（标记 `is_initialized`）
- 多个 child 注册到同一 parent 下形成兄弟链（`first_child` → `next_sibling`）
- HSM 使能后（`Start` 之后）拒绝注册

`HSM_Node_Param` 结构：

```c
typedef struct {
    HSM_Node* node;
    bool (*handler)(HSM_Event_Package);          // 必须，事件处理
    void (*entry_action)(HSM_Event_Package);     // 可选，进入状态
    void (*exit_action)(HSM_Event_Package);      // 可选，退出状态
    void (*continuous_action)();                 // 可选，持续执行
} HSM_Node_Param;
```

### 运行

```c
void HSM_Start(HSM* hsm, HSM_Node* initial_state);
void HSM_StartWithEvent(HSM* hsm, HSM_Node* initial_state, HSM_Event_Package event);
void HSM_Process(HSM* hsm);
void HSM_SetEnable(HSM* hsm, bool enable);
```

- `HSM_Start`：从初始状态到根节点执行所有 entry_action，内部调用 `HSM_SetEnable(true)`
- `HSM_Process`：出队并执行所有待处理转换，然后调用当前状态的 `continuous_action`
- `HSM_SetEnable`：禁用时 `SendEvent` 和 `Process` 静默返回

### 事件与转换

```c
void HSM_SendEvent(HSM* hsm, HSM_Event_Package event);
bool HSM_RequestTransition(HSM* hsm, HSM_Node* target, HSM_Event_Package event);
```

`HSM_Event_Package`：

```c
typedef struct {
    uint8_t HSM_Event_ID;
    union {
        uint8_t  data[HSM_EVENT_PAYLOAD_SIZE];
        uint32_t datau32;
        int32_t  datai32;
        float    dataf32;
        // ...
    } HSM_DataPayload;
} HSM_Event_Package;
```

执行模型：

```
SendEvent(event)                RequestTransition(target, event)
  ├→ Dispatch 冒泡                 ├→ 入队
  ├→ 调 handler(event)            └→ 返回
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
Root (锚点, 自动初始化)
├── Idle ──(sibling)── Charging
└── (Idle.child) Running
    ├── Normal ──(sibling)── Avoiding
    └── ...
```

### 1. 定义事件

```c
enum EventID : uint8_t {
    EV_START    = 1,
    EV_OBSTACLE = 2,
    EV_CLEAR    = 3,
};
```

### 2. 定义回调

```c
// handler — 返回 true 表示"我处理了"
static bool handler_idle(HSM_Event_Package e)     { return e.HSM_Event_ID == EV_START; }
static bool handler_normal(HSM_Event_Package e)   { return e.HSM_Event_ID == EV_OBSTACLE; }

// entry/exit — 状态进出时的副作用
static void entry_running(HSM_Event_Package) { printf("开始运行\n"); }
static void exit_avoiding(HSM_Event_Package) { printf("退出避障\n"); }

// continuous — 每帧调用
static void continuous_heartbeat() { printf("心跳\n"); }
```

### 3. 构建状态树

```c
FMS_OBJECT_MEMORY obj;
FMS_NODE_MEMORY  nm[6];

HSM* hsm = HSM_Create(&obj);
HSM_Node *root, *idle, *running, *normal, *avoiding, *charging;
root     = HSM_Node_Create(&nm[0]);
idle     = HSM_Node_Create(&nm[1]);
charging = HSM_Node_Create(&nm[2]);
running  = HSM_Node_Create(&nm[3]);
normal   = HSM_Node_Create(&nm[4]);
avoiding = HSM_Node_Create(&nm[5]);

// Root 的子节点 (兄弟关系)
HSM_RegisterChildNodes(hsm, root,
    (HSM_Node_Param[]){
        { idle,     handler_idle,     entry_idle,     NULL,         continuous_heartbeat },
        { charging, handler_charging, entry_charging, exit_charging, NULL },
    }, 2);

// Running 是 Idle 的子节点
HSM_RegisterChild(hsm, idle,
    (HSM_Node_Param){ running, handler_running, entry_running, NULL, NULL });

// Normal 和 Avoiding 是 Running 的子节点
HSM_RegisterChildNodes(hsm, running,
    (HSM_Node_Param[]){
        { normal,   handler_normal,   entry_normal,   NULL,           continuous_heartbeat },
        { avoiding, handler_avoiding, entry_avoiding, exit_avoiding,  NULL },
    }, 2);
```

### 4. 运行主循环

```c
HSM_Start(hsm, idle);

while (1) {
    // 根据外部输入发送事件
    if (obstacle_detected) {
        HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = EV_OBSTACLE });
        HSM_RequestTransition(hsm, avoiding, (HSM_Event_Package){ EV_OBSTACLE });
    }
    if (obstacle_cleared) {
        HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = EV_CLEAR });
        HSM_RequestTransition(hsm, normal, (HSM_Event_Package){ EV_CLEAR });
    }

    HSM_Process(hsm);  // 执行转换 + continuous_action
}
```

## 树结构约束

1. **必须有公共根节点**：所有状态节点必须是同一棵树的成员，否则 LCA 找不到返回 NULL
2. **注册顺序**：active state 节点**必须先作为 child 注册**（获得完整初始化），之后才能作为 parent 注册其子节点。根节点最先作为 parent，只做容器，自动初始化对它合适。如果反过来——先当 parent 再当 child——回调会永久丢失
3. **注册必须在 Start 前**：`Start` 内部调用 `HSM_SetEnable(true)`，使能后 `RegisterChild` 拒绝注册

## 回调时机

| 回调 | 触发时机 |
|------|---------|
| `handler` | `SendEvent` 时自底向上冒泡，返回 true 停止 |
| `entry_action` | `Start` 时（初始状态路径）/ `Transition` 时（LCA→target 路径） |
| `exit_action` | `Transition` 时（current→LCA 路径） |
| `continuous_action` | 每次 `Process()` |
