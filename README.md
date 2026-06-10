# learnlib

> 多人协作嵌入式 C 学习与项目实践资料库  
> Collaborative C Language Learning & Embedded Systems Project Library

## 简介 Introduction

本仓库由 504lib 团队维护，旨在为嵌入式系统开发、单片机实践与 C 语言学习者提供一个集体协作、经验和项目共享的平台。仓库以 C 语言为主，包含多位成员的学习总结、练习代码、实际项目及解决方案。

This repository is maintained by the 504lib team and is designed as a resource-sharing and collaborative platform for those learning the C language, embedded systems, and microcontroller project development. It consists mainly of code in C and offers summaries, exercises, and hands-on projects from multiple contributors.

## 目录结构 Structure

### 公共库 public_lib

`public_lib/` 为团队共享的可复用嵌入式 C 库，所有成员均可使用和贡献：

- HSM / FSM — 层次状态机与有限状态机
- static_queue / static_stack — 编译时静态容器
- multikey — 多按键驱动
- menu — 菜单框架
- protocol — 通信协议栈
- PID_fibric — PID 控制器
- Protothreads — 协程/原型线程
- Log — 日志模块
- distance_sensor — 红外测距

### 成员目录

| 成员 | 目录 | 主要方向 |
|---|---|---|
| whyP762 | `ZK_Lib/` | STM32/ESP32/FreeRTOS/LVGL 项目与移植 |
| Yoke486 | `CJH_Lib/`, `Fellow_Car_Project/` | C 基础、外设驱动、竞赛小车 |
| wangyutong234565 | `WYT_Lib/` | C51、C 基础 |
| tzozhengjie | `TZJ_Lib/` | 嵌入式实践、学习笔记 |
| s h e e p (陈名扬) | `CMY_Lib/` | C 入门 🆕 |
| ysy0128 | `YSY_Lib/` | C 入门 🆕 |
| rwh931 | `RWH_Lib/` | C 入门 🆕 |

- `TI_G3507_Template/` — TI 电机驱动板参考工程
- `Team_collaborate/` — 团队协作资料

Each member has a dedicated directory for their own learning archive or specific projects. Typical content includes:

- Peripheral drivers for microcontrollers (UART, GPIO, Timers, etc.)
- FreeRTOS, LVGL, ESP32, STM32 相关开发实践
- 系统级项目、实验与应用
- Notes and code examples for basic to advanced C programming

## 特色 Features

- 多人协作代码与知识共享
- 丰富的嵌入式项目案例和练习
- 覆盖主流微控制器平台（STM32、ESP32、C51、TI 等）
- 适合工程学生、嵌入式初学者和自学开发者

- Collaborative learning and code sharing
- Real embedded projects and exercises
- Projects covering STM32, ESP32, C51, TI and more
- Suitable for engineering students and new developers

## 快速开始 Quick Start

1. 克隆仓库 Clone the repo
   ```sh
   git clone https://github.com/504lib/learnlib.git
   ```
2. **[重要]** 阅读 [Git 协作流程指南](Git_operation.MD)，了解分支管理、PR 流程和提交规范
3. 按需进入各自目录，参考对应子项目的 README 和源码
4. 有问题？去 [Discussions](https://github.com/504lib/learnlib/discussions) 的 ❓ Q&A 区提问
5. 做练习？去 [Discussions](https://github.com/504lib/learnlib/discussions) 的 📝 练习题区跟帖

## 贡献 Contributing

欢迎成员提交代码、实验报告、项目建议。请遵循以下流程：

1. **Fork 或创建功能分支** — 永远不要在 `main` 上直接开发
2. **提交遵循约定** — `feat(模块): 描述` / `fix(模块): 描述`
3. **PR 原子化** — 一个 PR 只做一件事
4. **合并后清理分支**

Pull requests, code contributions, and project suggestions are welcome! Please observe coding conventions as documented in each subdirectory.

## License

本仓库暂未声明相关 License，使用时请注意代码归属和责任问题。

_No explicit license; please respect code ownership and consult repository maintainers before commercial or derivative use._
