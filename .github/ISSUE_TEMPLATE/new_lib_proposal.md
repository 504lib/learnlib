---
name: 新库提案
about: 提议向 public_lib 新增一个公共库
title: '[Proposal]: '
labels: proposal, public_lib
assignees: whyP762
---

## 库名称

<!-- 建议的库名，如 sensor_fusion、ring_buffer 等 -->

## 解决的问题

<!-- 这个库解决什么痛点？什么场景下会用到？ -->

## API 草案

<!-- 大致的使用方式、核心 API 接口，伪代码即可 -->

```c
// 示例
```

## 设计约束确认

- [ ] 零动态分配（静态内存或调用者提供）
- [ ] 不依赖 `<stdlib.h>`
- [ ] 目标平台: STM32 / ESP32 / C51 / TI / 跨平台
- [ ] 提供编译时容量检查

## 与现有库的关系

<!-- 是否依赖现有 public_lib 中的库？是否与某个库功能重叠？ -->

## 关联 Issue / 讨论

<!-- 引用相关讨论 -->
