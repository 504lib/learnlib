# MultiKey 按键处理库

轻量级按键处理库，支持按键消抖、短按、长按和长按重复触发功能。

## 功能特性

- 按键消抖处理
- 短按事件回调
- 长按事件回调
- 长按重复触发（可开关）
- 支持上升沿/下降沿触发
- 多平台支持（FreeRTOS、裸机 HAL、自定义）
- 每个按键独立可配的时间参数

## 快速开始

### 1. 包含头文件

```c
#include "multikey.h"
```

### 2. 配置平台

在 `multikey.h` 中选择适合的平台：

```c
// 选择其中之一
#define MENU_USE_CMSIS_OS2       // 使用 CMSIS-OS2 (FreeRTOS)
#define MENU_USE_BARE_METAL_HAL  // 使用裸机系统 (HAL 库)
#define MENU_USE_CUSTOM          // 使用自定义配置
```

使用 `MENU_USE_CUSTOM` 时需自行定义 `MULTIKEY_GET_TICK` 宏。

### 3. 定义按键读取函数

```c
uint8_t ReadKeyPin(MulitKey_t *key) {
    return HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
}
```

### 4. 定义事件回调函数

```c
void OnKeyPressed(MulitKey_t *key) {
    // 短按回调
}

void OnKeyLongPressed(MulitKey_t *key) {
    // 长按回调（长按触发时调用一次，之后按重复间隔调用）
}
```

### 5. 初始化和使用

```c
MulitKey_t myKey;

int main(void) {
    MulitKey_Init(&myKey, ReadKeyPin, OnKeyPressed, OnKeyLongPressed, FALL_BORDER_TRIGGER);

    while (1) {
        MulitKey_Scan(&myKey);
        HAL_Delay(10);
    }
}
```

## API 参考

### 初始化

```c
void MulitKey_Init(MulitKey_t *key,
                   KeyReadPinCallback readPin,
                   KeyPressdCallback onPressed,
                   KeyLongPressdCallback onLongPressed,
                   BorderTrigger trigger);
```

| 参数 | 说明 |
|------|------|
| `key` | 按键实例指针 |
| `readPin` | 引脚读取回调（必须，传入 NULL 则初始化无效） |
| `onPressed` | 短按回调（可为 NULL） |
| `onLongPressed` | 长按回调（可为 NULL） |
| `trigger` | 触发边沿：`RISE_BORDER_TRIGGER` 或 `FALL_BORDER_TRIGGER` |

### 按键扫描

```c
void MulitKey_Scan(MulitKey_t *key);
```

需要周期性调用，建议间隔 10ms。

### 时间参数设置

以下函数均为 **per-key** 设置，时间单位与 `MULTIKEY_GET_TICK` 一致（HAL 平台下为 ms）：

```c
void MulitKey_SetDebounceTime(MulitKey_t *key, uint16_t time);
void MulitKey_SetLongPressTime(MulitKey_t *key, uint16_t time);
void MulitKey_SetLongPressRepeatTime(MulitKey_t *key, uint16_t time);
```

### 长按重复开关

```c
void MulitKey_EnableLongPressRepeat(MulitKey_t *key, bool enable);
```

默认开启。关闭后长按状态只触发一次回调，不再重复。

## 数据结构

### MulitKey_Time_t

```c
typedef struct {
    uint16_t Key_Debounce_Time;         // 消抖时间
    uint16_t Key_LongPress_Time;        // 长按判定时间
    uint16_t Key_LongPress_Repeat_Time; // 长按重复触发间隔
} MulitKey_Time_t;
```

### MulitKey_t

| 字段 | 类型 | 说明 |
|------|------|------|
| `isEnable_LongPress_Repeat` | `bool` | 是否启用长按重复触发 |
| `Border_trigger` | `BorderTrigger` | 触发边沿 |
| `readPin` | `KeyReadPinCallback` | 引脚读取回调 |
| `onPressed` | `KeyPressdCallback` | 短按回调 |
| `onLongPressed` | `KeyLongPressdCallback` | 长按回调 |
| `state` | `KeyState` | 当前状态机状态 |
| `time` | `MulitKey_Time_t` | 时间参数 |
| `press_last_time` | `uint32_t` | 按键时间戳（内部使用） |

## 配置参数

默认时间参数（在 `multikey.c` 中定义，初始化时写入）：

```c
#define KEY_DEBOUNCE_TIME 10           // 消抖时间
#define KEY_LONGPRESS_TIME 500         // 长按判定时间
#define KEY_LONGPRESS_REPEAT_TIME 100  // 长按重复触发间隔
```

时间单位与 `MULTIKEY_GET_TICK` 一致。初始化后可通过 `MulitKey_Set*Time()` 系列函数按实例修改。

### 触发模式

```c
typedef enum {
    RISE_BORDER_TRIGGER = 0,  // 上升沿触发（低→高）
    FALL_BORDER_TRIGGER,      // 下降沿触发（高→低）
} BorderTrigger;
```

## 状态机

```
KEY_IDLE ──(边沿触发)──> KEY_DEBOUNCE ──(消抖通过)──> KEY_PRESSED
                            │                            │
                            └──(抖动)──> KEY_IDLE        │(超时)
                                                         ↓
KEY_IDLE <──(释放)── KEY_LONGPRESS <──(长按触发)────────┘
    ↑                     │
    └──(释放)─────────────┘
```

| 状态 | 说明 |
|------|------|
| `KEY_IDLE` | 空闲，等待按键触发 |
| `KEY_DEBOUNCE` | 消抖，确认按键稳定 |
| `KEY_PRESSED` | 已按下，等待释放（触发短按）或超时（进入长按） |
| `KEY_LONGPRESS` | 长按中，按重复间隔触发回调，直到释放 |

## 注意事项

- `MulitKey_Scan()` 需定期调用，建议间隔 10ms
- 时间参数单位与 `MULTIKEY_GET_TICK` 一致（HAL 平台为 ms，FreeRTOS 下为系统 tick）
- 回调函数在 `MulitKey_Scan()` 的调用上下文中执行，应保持简短
- 支持多按键，每个按键需要独立的 `MulitKey_t` 实例
- `MulitKey_Init` 中 `readPin` 为 NULL 时初始化不生效
