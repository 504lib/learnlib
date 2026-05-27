# static_stack

一个基于宏的 C 静态栈实现，提供编译期容量检查、运行时断言和一组边界测试，适合嵌入式与桌面环境。

## 功能特性

- 固定容量栈，支持入栈、出栈、查看栈顶、清空
- 编译期容量检查，限制容量大于 0 且不超过最大值
- 运行时断言，便于在嵌入式环境定位误用
- 自带覆盖空栈、满栈、LIFO 顺序和容量 1 边界的测试用例

## 目录结构

- [static_stack.h](static_stack.h)：栈宏与实现
- [tests/static_stack_test.c](tests/static_stack_test.c)：测试用例
- [CmakeLists.txt](CmakeLists.txt)：构建配置

## 构建测试

使用 CMake 构建并开启测试目标：

1. 配置时打开 STATIC_STACK_BUILD_TESTS
2. 构建生成的 static_stack_test 可执行文件

## 使用方式

1. 声明一个栈类型：
   - DECLARE_STATIC_STACK(NAME, TYPE, CAPACITY)
2. 创建实例并初始化：
   - NAME##_t stack;
   - NAME##_INIT(&stack);
3. 常用接口：
   - NAME##_PUSH(&stack, item)
   - NAME##_POP(&stack, &item)
   - NAME##_TOP(&stack, &item)
   - NAME##_PEEK(&stack, &item)
   - NAME##_CLEAR(&stack)
   - NAME##_IS_EMPTY(&stack) / NAME##_IS_FULL(&stack) / NAME##_SIZE(&stack)

## 断言与嵌入式说明

默认使用 [../Log/Log.h](../Log/Log.h) 中的 LOG_ASSERT。若希望关闭断言，可在编译时定义 NO_LOG_ASSERT。

## 编译期容量限制

- 容量必须大于 0
- 容量不能超过 STATIC_STACK_MAX_CAPACITY

如需调整最大容量，可修改 [static_stack.h](static_stack.h) 中的 STATIC_STACK_MAX_CAPACITY。

## 测试覆盖

[tests/static_stack_test.c](tests/static_stack_test.c) 包含以下测试：

- 空栈的 POP/TOP 失败路径
- 满栈 PUSH 失败路径
- LIFO 顺序校验
- PEEK 不修改 SIZE
- CLEAR 后复用
- 容量为 1 的边界
- 多轮压测