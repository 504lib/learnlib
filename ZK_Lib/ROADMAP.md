# 公交车项目重构路线图 (ROADMAP)

基于 2026-06-16 代码审查，梳理已知问题与改进计划。

---

## 当前系统架构

```
                       UART (AA 55 ... 0D 0A)
  ┌────────────────┐ ◀───────────────────────▶ ┌─────────────────┐
  │  STM32 C8T6    │                           │  ESP32C3 站台    │
  │  OLED + 按键   │                           │  WiFi AP + Web   │
  │  protocol.c/h  │                           │  protocol.hpp    │ (旧版)
  └────────────────┘                           └────────┬────────┘
                                                        │ WiFi
                                                        │ HTTP /api/*
                                              ┌─────────┴─────────┐
                                              │  ESP32C3 车辆     │
                                              │  路由调度 + 扫描   │
                                              │  protocol.hpp     │ (中间版)
                                              └───────────────────┘
```

**核心问题**: 三套设备用了三套不同的 protocol 实现, 同一个帧格式写了三遍代码。

---

## 阶段 0: 打基础 (不涉及代码逻辑变更)

### 0.1 补文档 ✅ (本次已完成)

- [x] C8T6_FreeRtos/README.MD (引脚、外设、菜单、协议帧格式)
- [x] ESP32C3/README.md (引脚、Web API、任务、数据流)
- [x] ESP32C3_Slave/README.md (引脚、状态机、站点评分、通信流程)

### 0.2 补编译验证

- [ ] 确认 C8T6_FreeRtos 当前可编译通过 (Keil MDK)
- [ ] 确认 ESP32C3 当前可编译通过 (PlatformIO)
- [ ] 确认 ESP32C3_Slave 当前可编译通过 (PlatformIO)

---

## 阶段 1: 协议层统一 (高收益, 低风险)

**目标**: 三套设备统一使用 `public_lib/protocol` (33KB C 版本, 注册制 Handler + 自定义队列 + 看门狗)。

### 1.1 STM32 端替换

**文件**: `C8T6_FreeRtos/protocol/protocol.{c,h}` → 替换为 `public_lib/protocol/protocol.{c,h}`

当前 STM32 的 protocol 已经是注册制 (handler table), 与 public_lib 版本最接近。替换要点:
- [ ] 适配 `#include "main.h"` 路径 (public_lib 用 `<stdint.h>`, STM32 用 HAL 类型)
- [ ] 确认 `MAX_PAYLOAD_LEN` / `MAX_CMD_TYPE_HANDLERS` 宏兼容
- [ ] 确认环形缓冲区 API 兼容 (public_lib 可能用 static_queue)
- [ ] freertos.c 中 Handler 注册代码调整 (如有 API 变化)

### 1.2 ESP32C3 站台端替换

**文件**: `ESP32C3/src/protocol/protocol.{hpp,cpp}` → 替换为 `public_lib/protocol/protocol.{c,h}`

当前是最老版本 (硬编码 if-else, 单一回调), 改动最大:
- [ ] main.cpp 中 `setAckCallback/setPassengerNumCallback/setClearCallback` 改为 `Register_Hander`
- [ ] `ACK_Queue_t` union 替换为 public_lib 的标准 `CmdPacket`
- [ ] 确认大端序列化函数 `rd_u32_be/wr_u32_be` 与 public_lib 一致
- [ ] 确认帧解析的字节级状态机可复用

### 1.3 ESP32C3_Slave 车辆端替换

**文件**: `ESP32C3_Slave/src/protocol/protocol.{hpp,cpp}` → 替换为 `public_lib/protocol/protocol.{c,h}`

当前是中间版 (Register_Hander + CmdHandler), 改动中等:
- [ ] main.cpp 中 `Register_Hander` 调用参数适配
- [ ] `DataPacket_t` → 统一为 public_lib 的结构体
- [ ] RouterScheduler 中 `commandQueueCallback` 适配新 DataPacket 类型

---

## 阶段 2: STM32 端代码拆分

**目标**: 880 行的 `freertos.c` 拆分为独立模块, 不改变逻辑。

### 2.1 菜单初始化独立

- [ ] 抽取 `menu_init.c/h`: 所有 `create_*_item()` + `Link_*()` 调用 → `menu_init()`
- [ ] `#if SLAVE_MODE` 分支保留, 保持编译时选择

### 2.2 协议回调独立

- [ ] 抽取 `uart_callbacks.c/h`: `INT_Handler`, `FLOAT_Handler`, `synchronized_passengers`, `ACK_Event_mutex`, `Clear_Event`, `VehicleStatus_Callback`

### 2.3 ACK 任务独立

- [ ] 抽取 `ack_task.c/h`: `ACK_TASK` 函数 (ACK 重传逻辑)
- [ ] 抽取 `uart_task.c/h`: `uart_task` 函数 (接收 + 乘客同步)

### 2.4 按键任务保留

- [ ] KEY_Task 保留在 freertos.c (或抽取 `key_task.c`), 按键回调 `KEY_UP_Pressed` 等保留

---

## 阶段 3: ESP32C3_Slave 状态机重构

**目标**: 消除 String 重复函数, 拆分 `CheckArrivingAndMaybeLeave`, 用 FSM 框架替代 switch。

### 3.1 砍掉 String 版本函数

- [ ] 删除 `sendSinglePost()` (String 版, 保留 `sendSinglePost_C_style`)
- [ ] 删除 `CheckArrivingAndMaybeLeave()` (String 版, 保留 `CheckArrivingAndMaybeLeave_C_style`)
- [ ] 删除 `Get_RouterInfo_JSON()` (String 版, 保留 C_style)
- [ ] StationRepo: `Get_Index_Station_Name()` 等 String 版标记 `// deprecated`, 全部走 C 字符串版

### 3.2 拆分 CheckArrivingAndMaybeLeave_C_style

当前 87 行, 同时做 HTTP GET + JSON 解析 + 乘客检查 + 状态上报 + 状态更新:

```
station_fetch_passenger_count(index) → int    // 纯 HTTP GET + JSON 解析, 返回乘客数, -1 失败
station_check_and_report(status, pnum) → void // 检查乘客数 → 决定停留/离站 → 发状态报告
```

### 3.3 使用 public_lib/FSM 替代 switch

当前 switch 的问题:
- 计时器状态用 `static uint32_t lastPostTime` 藏在 case 内
- 状态转换靠 `Update_Vehicle_Status()` + 下个 tick 匹配对应 case
- entry/exit 动作手动散落在每个 case 里

目标:
- [ ] 每个状态定义 `{entry, exit, action}`
- [ ] `STATUS_CONNECTING.entry` → `Connect_To_Station`
- [ ] `STAUS_DISCONNECTED.exit` → 清理 WiFi 状态
- [ ] `STATUS_LEAVING.entry` → `Move_To_Next_Station`
- [ ] 计时器在 FSM entry 重置, `action` 做持续逻辑
- [ ] `FSM_State_Transition` 替代 `vehicle_info.Update_Vehicle_Status`

### 3.4 其他小修

- [ ] `CommandQueueCallback` 从 `std::function` 改为纯 C 函数指针
- [ ] `StationRepo` 注释/命名修正: 它是固定数组, 不是链表

---

## 阶段 4: 功能增强

### 4.1 站点持久化 (NVS)

- [ ] ESP32C3 站台: 站点列表从 ESP32 NVS 加载, 不在代码中硬编码
- [ ] Web API 增加:
  - `GET /api/stations` → 站点列表
  - `POST /api/stations` → 添加站点 `{name, name_ch, ssid, password, ip}`
  - `DELETE /api/stations/:index` → 删除站点
- [ ] Web 前端加站点管理界面

### 4.2 OTA 无线更新

**ESP32 端**:
- [ ] 集成 `ArduinoOTA` 或 `AsyncElegantOTA` (与现有 AsyncWebServer 同生态)
- [ ] Web 前端加固件上传按钮

**STM32 端** (可选, 复杂度高):
- [ ] ESP32 作为桥梁: Web 上传 STM32 固件 → ESP32 UART 写 STM32 flash
- [ ] STM32 端需要 bootloader 或 IAP 支持
- [ ] 复用现有 UART 帧协议做固件传输

### 4.3 多从机压力测试

- [ ] 并发连接测试: 多个 ESP32C3_Slave 同时连接站台 AP
- [ ] 并发 POST 测试: 多个车辆同时 POST `/api/vehicle_report`
- [ ] ACK 冲突测试: 多个设备同时 UART 通信时的事件组行为
- [ ] vehicles[] 数组并发保护: 当前无锁操作, 需加临界区
- [ ] 内存泄漏测试: 长时间运行后 `ESP.getFreeHeap()` 趋势

---

## 阶段 5: 长期规划

### 5.1 OLED_menu 库 (独立 Issue)

- [ ] 将 menu.c 从 u8g2 解耦: 抽象渲染接口 (draw_str, draw_box, get_width...)
- [ ] 支持多种后端: u8g2, LVGL, 裸 SSD1306
- [ ] 参考 public_lib 风格: 纯 C, 零动态分配, 编译期配置

### 5.2 ESP32C3 站台 main.cpp 拆分

- [ ] HTML/CSS/JS 字符串抽取到 `web_page.h`
- [ ] HTTP 路由处理抽取到 `api_handlers.cpp`
- [ ] 车辆管理逻辑抽取到 `vehicle_manager.c` (替换全局 `vehicles[]`)

### 5.3 统一构建系统

- [ ] 考虑从 Keil MDK + PlatformIO 迁移到统一 CMake (已有 public_lib 的 CMake 经验)
- [ ] CI/CD: GitHub Actions 自动编译三端 + Cppcheck 静态分析

---

## 改动优先级总结

| 优先级 | 阶段 | 说明 | 时间估计 |
|--------|------|------|----------|
| 🔴 P0 | 0.2 | 确认三端可编译 | 1h |
| 🔴 P0 | 1.1 | STM32 protocol → public_lib | 2h |
| 🔴 P0 | 3.1 | 砍 String 重复函数 | 1h |
| 🟡 P1 | 1.2 | ESP32C3 站台 protocol → public_lib | 3h |
| 🟡 P1 | 1.3 | ESP32C3_Slave protocol → public_lib | 2h |
| 🟡 P1 | 3.2 | 拆分 CheckArrivingAndMaybeLeave | 2h |
| 🟢 P2 | 2.x | freertos.c 拆分 | 3h |
| 🟢 P2 | 3.3 | FSM 替代 switch | 3h |
| 🟢 P2 | 4.1 | NVS 站点持久化 | 3h |
| 🔵 P3 | 4.2 | OTA | 4h |
| 🔵 P3 | 4.3 | 压力测试 | 4h |
| ⚪ P4 | 5.x | OLED_menu / CMake / CI | 待排期 |

---

> 更新日期: 2026-06-16
