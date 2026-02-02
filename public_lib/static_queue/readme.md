# static_queue

一个基于宏的 C 静态队列实现，支持编译期容量检查与运行时断言，适合嵌入式与桌面环境。

## 功能特性

- 固定容量环形队列（入队/出队/查看队头/查看队尾）
- 编译期容量检查（容量 > 0，且不超过最大容量）
- 运行时断言（支持输出一次后永久堵塞）
- 简单边界测试示例

## 目录结构

- [static_queue.h](static_queue.h)：队列宏与实现
- [static_queue_demo.c](static_queue_demo.c)：演示与边界测试
- [Log.h](Log.h)：日志与断言宏
- [CmakeLists.txt](CmakeLists.txt)：构建配置

## 构建与运行

使用 CMake 构建：

1. 生成构建目录并构建
2. 运行生成的可执行文件 `static_queue_demo`

## 使用方式

1. 在代码中声明一个队列类型：
	- `DECLARE_STATIC_QUEUE(NAME, TYPE, CAPACITY)`
2. 创建实例并初始化：
	- `NAME##_t q;`
	- `NAME##_INIT(&q);`
3. 常用接口：
	- `NAME##_PUSH(&q, item)`
	- `NAME##_POP(&q, &item)`
	- `NAME##_PEEK(&q, &item)`
	- `NAME##_BACK(&q, &item)`
	- `NAME##_IS_EMPTY(&q)` / `NAME##_IS_FULL(&q)` / `NAME##_SIZE(&q)`

## 断言与嵌入式说明

`Log.h` 中的 `LOG_ASSERT` 默认会打印一次并进入永久堵塞循环。适用于嵌入式故障定位。

如需改为标准断言或自定义处理，可在 `Log.h` 中修改 `LOG_ASSERT` 宏。

## 编译期容量限制

队列容量会在编译期检查：

- 容量必须 > 0
- 容量不能超过 `STATIC_QUEUE_MAX_CAPACITY`

修改最大容量：

- 在 [static_queue.h](static_queue.h) 中修改 `STATIC_QUEUE_MAX_CAPACITY`

## 测试用例覆盖

示例测试覆盖了：

- 空队列 `POP/PEEK/BACK`
- 满队列 `PUSH`
- 环回与顺序校验
- 容量为 1 的边界
- 多轮压测（多次填满/清空）

如需添加更严格的随机测试或不变式校验，可以在 [static_queue_demo.c](static_queue_demo.c) 中扩展。
