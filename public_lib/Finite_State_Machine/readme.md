# Finite State Machine

基于 C 接口的有限状态机库，内部使用 C++ 实现，对外暴露 `extern "C"` 接口，适用于嵌入式及桌面平台。

## 功能特性

- 最多 16 个状态（`MAX_STATE_NUM` 可配置），每个状态绑定 action / entry / exit 三个回调
- 状态转换通过单槽队列串行化，避免重入问题
- Placement new 方式构造，用户提供对齐内存，无堆分配
- C 与 C++ 均可调用

## 目录结构

- [Finite_State_Machine.h](Finite_State_Machine.h)：类型定义与 C 接口声明
- [Finite_State_Machine.cpp](Finite_State_Machine.cpp)：核心实现
- [static_queue/](static_queue/)：依赖的静态队列库
- [Log/](Log/)：日志模块依赖
- [tests/](tests/)：单元测试

## 核心概念

### 状态回调

每个状态由 `FSM_Function` 描述，包含三个函数指针：

| 回调 | 调用时机 | 是否必须 |
|---|---|---|
| `action` | 状态处于 Active 时每次 `FSM_Process` 调用 | **是** |
| `entry` | 进入该状态时调用一次 | 否（可为 NULL） |
| `exit` | 退出该状态时调用一次 | 否（可为 NULL） |

### 处理循环

`FSM_Process()` 是状态机的驱动入口，每次调用根据当前动作类型执行：

```
Start → Entry → Action → Action → ...  (正常运行)
                  ↓
    FSM_State_Transition() 触发 Exit
                  ↓
              Exit → (出队下一状态) → Entry → Action → ...
```

换言之，在 Action 阶段调用 `FSM_State_Transition()` 后，**下一次** `FSM_Process()` 才会执行 Exit 并切换到目标状态。转换不是立即发生的。

### 状态转换队列

内部使用容量为 **1** 的队列缓存待转换的目标状态 ID。这意味着：

- 同一时刻只能排队一个状态转换
- 在前一个转换尚未被 `FSM_Process` 消费前，再次调用 `FSM_State_Transition()` 会返回 `false`

## 快速上手

```c
#include "Finite_State_Machine.h"

// 1. 定义状态回调
void stateA_action(void) { /* 状态 A 的 Action */ }
void stateA_entry(void) { /* 进入状态 A */ }
void stateA_exit(void) { /* 退出状态 A */ }

void stateB_action(void) { /* 状态 B 的 Action */ }
void stateB_entry(void) { /* 进入状态 B */ }

// 2. 分配内存（对齐 + 足够大小）
FMS_MEMORY memory = {0};

int main(void) {
    // 3. 创建状态机
    FSM_Structure* fsm = FSM_Create(&memory);

    // 4. 注册状态（action 必须非空）
    FSM_Function funcA = {stateA_action, stateA_entry, stateA_exit};
    FSM_Function funcB = {stateB_action, stateB_entry, NULL};
    FSM_Add_State(fsm, 1, funcA);
    FSM_Add_State(fsm, 2, funcB);

    // 5. 启动（指定初始状态）
    FSM_Start(fsm, 1);

    // 6. 主循环中驱动状态机
    while (1) {
        FSM_Process(fsm);
        // 你的其他逻辑...
    }

    // 7. 销毁
    FSM_Destroy(fsm);
}
```

## API 参考

### FSM_Create

```c
FSM_Structure* FSM_Create(FMS_MEMORY* memory);
```

在用户提供的对齐内存上构造状态机。`FMS_MEMORY` 大小为 `FSM_STORAGE_SIZE`（1024 字节），8 字节对齐。传入 `NULL` 返回 `NULL`。

### FSM_Destroy

```c
void FSM_Destroy(FSM_Structure* fsm);
```

显式调用析构函数，标记状态机为未初始化。传入 `NULL` 安全（空操作）。

### FSM_Add_State

```c
bool FSM_Add_State(FSM_Structure* fsm, uint8_t state_id, FSM_Function function);
```

注册一个状态。约束：
- `state_id` 不能是 `0xFF`（内部保留为无效 ID）
- `function.action` 不能为 `NULL`
- 同一 `state_id` 不可重复注册
- 状态总数受 `MAX_STATE_NUM`（默认 16）限制

### FSM_Start

```c
bool FSM_Start(FSM_Structure* fsm, uint8_t first_state_id);
```

指定初始状态并准备进入 Entry 阶段。要求状态机已初始化且 `first_state_id` 已注册。

### FSM_State_Transition

```c
bool FSM_State_Transition(FSM_Structure* fsm, uint8_t state_id);
```

请求从当前状态切换到 `state_id`。切换在**下一次** `FSM_Process()` 时执行。转换队列满（已有未消费的转换）时返回 `false`。

### FSM_Process

```c
void FSM_Process(FSM_Structure* fsm);
```

驱动状态机执行一步。调用频率决定了状态机的 "心跳" 速度。典型用法是放在主循环或定时中断中。

当 `isEnable == false` 或当前状态无效时，直接返回不做任何事。

### FSM_Set_Enable

```c
void FSM_Set_Enable(FSM_Structure* fsm, bool enable);
```

使能 / 暂停状态机。暂停后 `FSM_Process()` 为 no-op，但状态保持不变。`FSM_State_Transition` 在暂停期间仍可正常调用。

## 注意事项

1. **action 回调必须提供**：`FSM_Add_State` 会拒绝 `action == NULL` 的状态。entry / exit 可以为 NULL，此时对应阶段被跳过。

2. **状态 ID 不可使用 0xFF**：该值被内部用作 "无效状态" 标记，注册 0xFF 会触发 `LOG_FATAL`。

3. **转换不是立即的**：`FSM_State_Transition()` 只是将目标 ID 推入队列，实际切换发生在下一次 `FSM_Process()`。这意味着调用后当前状态的 action 还可能再执行若干次（取决于你何时调用 `FSM_Process`）。

4. **转换队列只有 1 个槽**：在消费前连续调用两次 `FSM_State_Transition()` 会失败。如果需要在一次 action 中触发多个状态转换，需要自行设计调度逻辑。

5. **FSM_Process 不是线程安全的**：内部队列操作有临界区钩子（`STATIC_QUEUE_ENTER_CRITICAL` / `STATIC_QUEUE_EXIT_CRITICAL`），但默认展开为空。多线程 / 中断环境使用时需要自行实现这两个宏。

6. **内存由调用者管理**：`FMS_MEMORY` 必须是 8 字节对齐的 1024 字节缓冲区。放在栈上、全局区或静态区均可，但生命周期必须覆盖状态机的整个使用期。`FSM_Destroy` 只调用析构函数，不释放内存。

7. **使能标志只影响 Process**：`FSM_Set_Enable(false)` 暂停后，`FSM_State_Transition` 仍可正常调用，只是 `FSM_Process` 不会推进状态机。

8. **LOG 依赖**：状态机内部使用了 `LOG_WARN`、`LOG_INFO`、`LOG_FATAL` 宏，依赖 [Log/Log.h](Log/Log.h)。如果未定义 `NO_LOG_ASSERT`，`static_queue` 也会链接到 `Log.h` 的 `LOG_ASSERT`。

## 典型模式

### 条件转换

```c
void stateA_action(void) {
    if (some_condition) {
        FSM_State_Transition(g_fsm, 2); // 转到 B
    } else if (error_condition) {
        FSM_State_Transition(g_fsm, 3); // 转到 Error
    }
    // 无转换则保持在当前状态
}
```

### 暂停与恢复

```c
// 进入低功耗前暂停
FSM_Set_Enable(fsm, false);
enter_low_power_mode();

// 唤醒后恢复
FSM_Set_Enable(fsm, true);
```

## 配置宏

| 宏 | 默认值 | 说明 |
|---|---|---|
| `MAX_STATE_NUM` | 16 | 最大状态数量，可在编译时通过 `-D` 覆盖 |
| `FSM_STORAGE_SIZE` | 1024 | FSM 对象所需内存大小 |

## 测试

测试文件位于 [tests/fsm_test.cpp](tests/fsm_test.cpp)，覆盖：

- 初始 Entry → Action 流程
- NULL entry/exit 的容许
- 状态转换顺序（Entry → Action → Exit → Entry → Action）
- 转换队列单槽限制
- 使能 / 禁用窗口
- 各类非法输入（NULL 指针、无效 ID、空回调等）

构建运行：

```bash
cd build-ninja && cmake -G Ninja .. && ninja && ctest
```
