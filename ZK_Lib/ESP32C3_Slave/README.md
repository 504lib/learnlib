# 公交车项目 ESP32C3 车辆端 (Slave/Vehicle)

ESP32-C3 车载节点，WiFi 扫描站点 → 连接 → 上报状态 → 按路线调度。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| 芯片 | ESP32-C3 (RISC-V 160MHz) |
| 开发板 | airm2m_core_esp32c3 |
| 框架 | Arduino (PlatformIO) |
| 操作系统 | FreeRTOS (Arduino 内建) |

---

## 引脚分配

| GPIO | 功能 | 备注 |
|------|------|------|
| GPIO4 | UART1 TX | 与 STM32 USART1 或 站台通信, 115200bps |
| GPIO5 | UART1 RX | 与 STM32 USART1 或 站台通信, 115200bps |
| LED_BUILTIN | 板载 LED | 状态指示 (测试闪烁) |

> **注意**: 串口引脚通过 `Serial1.begin(115200, SERIAL_8N1, 5, 4)` 配置, RX=GPIO5, TX=GPIO4。

---

## 目录结构

```
ESP32C3_Slave/
├── platformio.ini                         # PlatformIO 构建配置
├── src/
│   ├── main.cpp                           # 入口 (737行), 任务创建 + 协议回调注册 + Web 服务器
│   ├── protocol/
│   │   ├── protocol.hpp                   # 协议类 (C++ class, Register_Hander 注册制)
│   │   └── protocol.cpp                   # 帧发送/接收, 字节级状态机解析
│   ├── Station/
│   │   ├── Station.hpp                    # 站点数据结构 (Station_t)
│   │   └── Station.cpp
│   ├── StationRepo/
│   │   ├── StationRepo.hpp               # 站点仓库 (固定数组 N=20, 顺序管理)
│   │   └── StationRepo.cpp
│   ├── Vehicle/
│   │   ├── Vehicle.hpp                    # 车辆信息 (Vehicle_t, Vehicle_Info)
│   │   └── Vehicle.cpp
│   ├── RouterScheduler/
│   │   ├── RouterScheduler.hpp           # 路由调度器 (9 状态状态机)
│   │   └── RouterScheduler.cpp           # 634行, 含双版本函数 (String/_C_style)
│   ├── NetworkClient/
│   │   ├── NetworkClient.hpp             # WiFi/HTTP 客户端封装
│   │   └── NetworkClient.cpp
│   └── Log/
│       ├── Log.h                          # 日志模块
│       └── Log.cpp
├── include/
├── lib/
└── test/
```

---

## 依赖库

| 库 | 用途 |
|---|------|
| `esphome/AsyncTCP-esphome` | 异步 TCP |
| `ottowinter/ESPAsyncWebServer-esphome` | 异步 Web 服务器 (Slave 自带调试页面) |
| `bblanchon/ArduinoJson` | JSON 序列化/反序列化 |

---

## FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 功能 |
|------|--------|--------|------|
| Serial_Task | 2 | 1024 | 从 Serial1 读取字节 → `xUartRxQueue` |
| Rx_Task | 3 | 2048 | 从 `xUartRxQueue` 取字节 → protocol 解析 |
| ACK_Task | 2 | 2048 | 从 `xCommandQueue` 取 DataPacket → 发送 + ACK 等待 (3次重试,200ms超时) |
| Bus_scheduler_Task | 2 | 8192 | 核心调度: `RouterScheduler_Executer()` 每 100ms 执行一次 |

**内核对象**:

| 类型 | 名称 | 用途 |
|------|------|------|
| 队列 | xUartRxQueue | UART 接收字节队列 (32 × uint8_t) |
| 队列 | xCommandQueue | 命令发送队列 (32 × DataPacket_t) |
| 事件组 | evt | UART ACK 事件 `UART_ACK_REQUIRED (1 << 0)` |

---

## 车辆状态机 (RouterScheduler)

`RouterScheduler::RouterScheduler_Executer()` 每 100ms tick 一次, 9 状态 switch:

```
                    ┌─────────────┐
                    │ STATUS_IDLE │ (未使用)
                    └─────────────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
     ┌────────────┐ ┌───────────┐ ┌──────────────┐
     │  SCANNING  │◀│ DISCONNECT│◀│   LEAVING    │
     │ (WiFi扫描) │ │ (断开连接) │ │ (离站, 下一站)│
     └─────┬──────┘ └───────────┘ └──────────────┘
           │                              ▲
           ▼                              │
     ┌──────────┐                         │
     │  GROPE   │ (计算评分选最佳站点)      │
     └─────┬────┘                         │
           ▼                              │
     ┌────────────┐                       │
     │ CONNECTING │ (连接选中站点)          │
     └─────┬──────┘                       │
           ▼                              │
     ┌────────────┐               ┌───────┴──────┐
     │ CONNECTED  │──────────────▶│   WAITING    │
     │ (已连接)   │ (乘客>0)       │ (候车中,5s轮询)│
     └────────────┘               └───────┬──────┘
           │ (乘客=0)                     │
           ▼                              │
     ┌────────────┐                       │
     │   LEAVING  │◀──────────────────────┘
     └────────────┘
```

### 状态说明

| 状态 | 说明 | 动作 |
|------|------|------|
| STATUS_SCANNING | WiFi 扫描周边 AP | 启动扫描, 完成后 → GROPE |
| STATUS_GROPE | 评估站点优先级 | `FindBestStation()` 按信号/历史/距离评分 |
| STATUS_CONNECTING | 连接目标站点 WiFi | `Connect_To_Station()`, 成功 → CONNECTED |
| STATUS_CONNECTED | 已连接, 检查乘客 | GET `/api/info`, 检查乘客数 → ARRIVING 或 LEAVING |
| STATUS_ARRIVING | 即将进站 (有乘客) | 5s 间隔 POST 状态报告 + 轮询乘客数 |
| STATUS_WAITING | 候车中 | 5s 间隔 POST 状态报告 + 等待清零 |
| STATUS_LEAVING | 离站 | POST 离站报告 (重试5次), `Move_To_Next_Station()` |
| STAUS_DISCONNECTED | WiFi 断开 | 清理连接状态, 重新 → SCANNING |
| STATUS_IDLE | 空闲 (未使用) | → SCANNING |

### 站点评分算法 (`CalculateStationScore`)

```
score = 0
  + 50 分    如果该站点是下一个目标
  + 30/20/10/5 分  按信号强度 (>-50 / >-70 / >-80 / 其他)
  + 15 分    如果从未访问过
  + 10 分    如果超过 5 分钟未访问
  + (10 - min(访问次数, 10)) 分  惩罚频繁访问的站点
  = -1000 分 如果 SSID 不在扫描结果中
```

---

## UART 协议

与 STM32/站台使用相同的帧格式:

```
+------+------+------+------+-----------+------+------+------+------+
| 0xAA | 0x55 | Type | Len  | Payload[] | CHK_H| CHK_L| 0x0D | 0x0A |
+------+------+------+------+-----------+------+------+------+------+
```

### 命令类型

| CmdType | 值 | 说明 | 处理方式 |
|---------|------|------|----------|
| INT | 0 | 整数值 | 直接 ACK |
| FLOAT | 1 | 浮点值 | 直接 ACK |
| ACK | 2 | 确认 | 设置 `evt` 事件组 |
| PASSENGER_NUM | 3 | 乘客数量 | 直接 ACK |
| CLEAR | 4 | 清空乘客 | 直接 ACK |
| VEHICLE_STATUS | 5 | 车辆状态控制 | 权限校验 + 状态转换 + ACK |

### VEHICLE_STATUS 权限校验

```
仅允许 ARRIVING / WAITING / LEAVING 三种状态
  ├── WAITING 状态下仅允许切换到 LEAVING
  ├── 非 ARRIVING/WAITING 状态不可切换
  └── 禁止从 IDLE/SCANNING 等状态直接切换
```

---

## WiFi 通信流程 (车辆 ↔ 站台)

```
车辆端                                    站台端
  │                                         │
  │── 扫描 WiFi ──▶ 发现 Station AP          │
  │── 连接 AP ────▶                          │
  │                                         │
  │── GET /api/info ──────────────────────▶│
  │◀── JSON {passenger_list, vehicles, ...}│
  │                                         │
  │── 检查乘客数:                            │
  │    > 0 → POST /api/vehicle_report ────▶│ (status=arriving)
  │    = 0 → POST /api/vehicle_report ────▶│ (status=leaving)
  │                                         │
  │── 候车中每5s轮询:                        │
  │    GET  /api/info                     │ (检查是否清零)
  │    POST /api/vehicle_report           │ (status=waiting)
```

---

## 编译配置

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `MAX_CMD_TYPE_NUM` | 10 | 最大命令类型数 |
| `N` (StationRepo) | 20 | 最大站点数 |
| `MAX_VEHICLE_JSON_LENGTH` | 512 | 车辆信息 JSON 缓冲区 |
| `MAX_STATION_STRING_LENGTH` | 16 | 站名字符串最大长度 |
| `WAIT_ACK_TIMEOUS_MS` | 200 | ACK 超时 (ms) |
| `WAIT_ACK_RETRAY_COUNT` | 3 | ACK 重传次数 |
| `RESPONSE_JSON_BUFFER_SIZE` | 256 | HTTP 响应 JSON 缓冲区 |

---

## 已知问题

1. **双版本函数**: `sendSinglePost` / `sendSinglePost_C_style`, `CheckArrivingAndMaybeLeave` / `CheckArrivingAndMaybeLeave_C_style` 逻辑重复, String 版本待删除
2. **状态机**: 9 状态 switch, 计时器用 static 局部变量, 未使用 FSM 框架
3. **站点硬编码**: `freertos.c` 中写死站点数据, 未持久化到 NVS
4. **std::function**: `CommandQueueCallback` 使用 C++ `std::function`, 引入额外依赖
5. **无 OTA**: 缺少固件无线更新
6. **无压力测试**: 未验证多车辆同时连接的并发行为
7. **StationRepo**: 命名为"链表"但实际是固定数组, 混淆

详见 [ROADMAP.md](../ROADMAP.md) (待创建)
