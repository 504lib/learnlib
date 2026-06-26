# Log

轻量级嵌入式 C 日志库，支持运行时级别过滤和可选异步队列。

## 快速开始

```c
#include "Log.h"

void my_tx(const char* buffer, size_t buffer_size) {
    // 将 buffer 发送到串口/SWO/文件
}

int main(void) {
    LOG_Set_Transmit(my_tx);
    LOG_Set_Level(LOG_LEVEL_DEBUG);

    LOG_DEBUG("init done, count=%d", 42);
    LOG_INFO("system started");
    LOG_WARN("battery low");
    LOG_ERROR("timeout");
    LOG_FATAL("hard fault");
    LOG_RAW("raw bytes without prefix");
}
```

输出：

```
[DEBUG] main.c:12 init done, count=42
[INFO] main.c:13 system started
[WARN] main.c:14 battery low
[ERROR] main.c:15 timeout
[FATAL] main.c:16 hard fault
raw bytes without prefix
```

## 日志级别

`DEBUG(0) < INFO(1) < WARN(2) < ERROR(3) < FATAL(4) < RAW(5)`

设置级别后，低于该级别的日志被屏蔽。例如 `LOG_Set_Level(LOG_LEVEL_WARN)` 只输出 WARN 及以上。

## API

| 宏/函数 | 说明 |
|---|---|
| `LOG_DEBUG(fmt, ...)` | 调试信息，自动附带文件名和行号 |
| `LOG_INFO(fmt, ...)` | 一般信息 |
| `LOG_WARN(fmt, ...)` | 警告 |
| `LOG_ERROR(fmt, ...)` | 错误 |
| `LOG_FATAL(fmt, ...)` | 致命错误 |
| `LOG_RAW(fmt, ...)` | 无前缀原始输出（函数，非宏） |
| `LOG_ASSERT(x)` | 断言失败时打印并阻塞 |
| `LOG_Set_Level(level)` | 设置全局日志级别 |
| `LOG_Get_Level()` | 获取当前日志级别 |
| `LOG_Set_Transmit(fn)` | 设置输出接口 |

## 异步队列

定义 `LOG_USE_QUEUE` 为 1，日志写入静态队列，由 `LOG_Process()` 在非中断上下文中逐步发送：

```c
#define LOG_USE_QUEUE 1
#include "Log.h"

LOG_Init(my_tx);
// ... 中断中调用 LOG_DEBUG 等 ...
for (;;) {
    LOG_Process();  // 主循环中消费队列
}
```

## 缓冲区

默认 `LOG_BUFFER_SIZE` 为 64，可通过编译定义覆盖：

```cmake
target_compile_definitions(Log PUBLIC LOG_BUFFER_SIZE=128)
```

## 构建 & 测试

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
./test_log
```
