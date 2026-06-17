# 公交车项目 ESP32C3 站台端 (Station)

ESP32-C3 站台节点，作为 WiFi AP 提供 Web 服务，接收 STM32 的 UART 数据并转发给车辆端。

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
| GPIO4 | UART1 TX | 与 STM32 USART1 通信, 115200bps |
| GPIO5 | UART1 RX | 与 STM32 USART1 通信, 115200bps |
| LED_BUILTIN | 板载 LED | WiFi 状态指示 (AP 模式常亮, STA 模式闪烁) |

> **注意**: 串口引脚通过 `Serial1.begin(115200, SERIAL_8N1, 5, 4)` 配置, RX=GPIO5, TX=GPIO4。

---

## 目录结构

```
ESP32C3/
├── platformio.ini                         # PlatformIO 构建配置
├── src/
│   ├── main.cpp                           # 入口 (1521行), HTML Web 页面 + HTTP API + 任务创建
│   └── protocol/
│       ├── protocol.hpp                   # 旧版协议类 (C++ class, 单一回调)
│       └── protocol.cpp                   # 帧发送/接收, 硬编码 if-else 解析
├── include/
├── lib/
└── test/
```

---

## 依赖库 (platformio.ini)

| 库 | 用途 |
|---|------|
| `esphome/AsyncTCP-esphome` | 异步 TCP 服务器基础 |
| `ottowinter/ESPAsyncWebServer-esphome` | 异步 Web 服务器 |
| `bblanchon/ArduinoJson` | JSON 序列化/反序列化 |

---

## FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 功能 |
|------|--------|--------|------|
| Serial_Task | 2 | 1024 | 从 Serial1 读取字节 → 放入 `xUartRxQueue` |
| Rx_Task | 3 | 2048 | 从 `xUartRxQueue` 取字节 → 喂给 protocol 解析器 |
| ACK_Task | 2 | 2048 | 从 `xCommandQueue` 取命令 → 发送 + 等待 ACK (重试3次,超时200ms) |
| AP_Task / STA_Task | 1 | 1024 | AP模式: 每500ms更新乘客总数 / STA模式: 每3s请求站点数据 |
| VehicleCleanupTask | 1 | 2048 | 每30s清理超过5分钟的过期车辆数据 |
| (loop) | - | - | LED 闪烁 + 内存监控输出 |

**内核对象**:

| 类型 | 名称 | 说明 |
|------|------|------|
| 队列 | xUartRxQueue | UART 接收字节队列 (32 × uint8_t) |
| 队列 | xCommandQueue | 命令发送队列 (32 × ACK_Queue_t) |
| 队列 | xPassengerUpdateQueue | 乘客更新队列 (已弃用) |
| 事件组 | evt | UART ACK 事件标志 `UART_ACK_REQUIRED (1 << 0)` |

---

## Web 服务 (AP 模式)

### HTTP 路由

| 路由 | 方法 | 说明 |
|------|------|------|
| `/` | GET | 返回公交站台监控 HTML 页面 |
| `/api/info` | GET | 返回站点 JSON: 站名、总乘客数、各路线乘客、车辆列表 |
| `/api/clear` | GET | 清空指定路线乘客 `?rounter=0` |
| `/api/vehicle_report` | POST | 车辆上报状态 `route=&plate=&status=` |

### `/api/info` 返回 JSON

```json
{
  "station": "师范学院",
  "station_ch": "normal university",
  "passengers_total": 25,
  "ip": "192.168.4.1",
  "clients": 2,
  "ssid": "ESP32-Access-Point",
  "passenger_list": [5, 10, 8, 2],
  "route_names": ["路线1", "路线2", "路线3", "环线"],
  "vehicles": [
    {"route": 0, "plate": "黑A12345", "status": "arriving", "timestamp": 12345}
  ]
}
```

### `/api/vehicle_report` POST 参数

| 参数 | 类型 | 说明 |
|------|------|------|
| route | int | 路线枚举 (0-3) |
| plate | string | 车牌号 (≤20字符) |
| status | string | `waiting` / `arriving` / `leaving` |

### WiFi 配置

| 模式 | SSID | 密码 | IP |
|------|------|------|-----|
| AP 模式 (`AP_MODE=1`) | ESP32-Access-Point | 12345678 | 192.168.4.1 |
| STA 模式 (`AP_MODE=0`) | 连接目标 AP | - | DHCP |

---

## 数据流

```
STM32 ──UART──▶ ESP32C3 站台 ──WiFi AP──▶ ESP32C3_Slave 车辆
                   │
                   ├── 接收 STM32 的 PASSENGER_NUM / CLEAR 命令
                   ├── 维护 passenger[] 数组 (按路线索引)
                   ├── 接收车辆的 vehicle_report (HTTP POST)
                   ├── 管理 vehicles[] 数组 (最多10辆, LRU淘汰)
                   └── 向车辆返回 /api/info (含各路线乘客数)
```

## 路线枚举

| 枚举值 | 名称 |
|--------|------|
| 0 | Route_1 (路线1) |
| 1 | Route_2 (路线2) |
| 2 | Route_3 (路线3) |
| 3 | Ring_road (环线) |

## 车辆状态

| 状态 | 说明 |
|------|------|
| WAITING | 候车中, 在站点等待乘客 |
| ARRIVING | 即将进站 |
| LEAVING | 离站中, 触发清除该路线乘客并移除车辆 |

---

## 编译配置

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `AP_MODE` | 1 | 1=AP 模式 (Web 服务器), 0=STA 模式 (连接另一个 AP) |
| `MAX_VEHICLES` | 10 | 最大车辆跟踪数 |
| `UART_ACK_REQUIRED` | `(1 << 0u)` | ACK 事件标志位 |

---

## 已知问题

1. `main.cpp` 1521 行, HTML/CSS/JS 内联, 需拆分
2. `protocol.hpp/cpp` 为最初版本, 硬编码 if-else 解析, 与 Slaves/STM32 的协议实现不一致
3. 单 callback 模式 (setIntCallback/setFloatCallback/...) 不支持多 handler
4. 车内数组固定 10 个, 无持久化
5. 站点信息为硬编码 `String station_name = "师范学院"`
6. 无 OTA 更新
7. `ack_queue_t` union 依赖隐式大小端, portability 有问题
8. `passenger[]` 数组操作无锁

详见 [ROADMAP.md](../ROADMAP.md) (待创建)
