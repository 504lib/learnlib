# 公交车项目 STM32 端 (C8T6_FreeRtos)

STM32F103C8T6 端，负责 OLED 显示、按键交互、UART 与 ESP32 站台通信。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz) |
| 封装 | LQFP48 |
| 显示屏 | SSD1306 128x64 OLED (SPI) |
| 调试接口 | SWD (PA13/PA14) |
| 系统时钟 | HSE 8MHz → PLL ×9 = 72MHz |
| 操作系统 | FreeRTOS (CMSIS-V2) |

---

## 引脚分配

| 引脚 | 标签 | 功能 | 备注 |
|------|------|------|------|
| PC13 | LED | 板载 LED 指示灯 | GPIO Output |
| PA0 | CS_OLED | OLED 片选线 | GPIO Output |
| PA1 | DC_OLED | OLED 数据/命令选择 | GPIO Output |
| PA2 | USART2_TX | 日志串口 TX | 115200bps, 连 PC 或调试器 |
| PA3 | USART2_RX | 日志串口 RX | 未使用 |
| PA4 | RES_OLED | OLED 复位引脚 | GPIO Output |
| PA5 | SPI1_SCK | OLED SPI 时钟 | SPI 主机模式 |
| PA6 | SPI1_MISO | SPI 主机输入 | 本项目未使用 |
| PA7 | SPI1_MOSI | OLED SPI 数据线 | SPI 主机模式 |
| PA9 | USART1_TX | 与 ESP32 通信 TX | 115200bps, DMA |
| PA10 | USART1_RX | 与 ESP32 通信 RX | 115200bps, DMA + 空闲中断 |
| PB0 | KEY_UP | 向上按钮 | GPIO Input, 下拉 |
| PB1 | KEY_DOWN | 向下按钮 | GPIO Input, 下拉 |
| PB10 | KEY_ENTER | 确认按钮 | GPIO Input, 下拉 |
| PB11 | KEY_CANCEL | 取消/返回按钮 | GPIO Input, 下拉 |
| PC14/15 | - | 外部低速晶振 (LSE) | 32.768kHz, RTC 时钟源 |
| PD0/1 | - | 外部高速晶振 (HSE) | 8MHz |

---

## 外设分配

| 外设 | 功能 | DMA | 中断 |
|------|------|-----|------|
| SPI1 | OLED 显示 (SSD1306) | TX: DMA1_Ch3 | - |
| USART1 | 与 ESP32 站台通信 | RX: DMA1_Ch5, TX: DMA1_Ch4 | USART1_IRQn, 空闲中断 |
| USART2 | 日志输出 | TX: DMA1_Ch7 | - |
| RTC | 实时时钟, 定时保存 | - | - |
| TIM4 | FreeRTOS 时基 (1ms) | - | TIM4_IRQn |

**FreeRTOS 内核对象** (从 .ioc 文件):

| 类型 | 名称 | 说明 |
|------|------|------|
| 任务 | U8G2_TASK | OLED 渲染 + 菜单交互, 优先级 Normal |
| 任务 | KEY_TASK | 按键扫描, 优先级 Low |
| 任务 | UART_TASK | 串口帧接收 + 乘客同步, 优先级 High7 |
| 任务 | ACK_Task | ACK 重传 + 数据发送, 优先级 BelowNormal |
| 事件组 | UART_EVENT | UART 数据到达通知 + ACK 接收通知 |
| 互斥量 | UART_TXMute | 保护 USART1 发送操作 |
| 消息队列 | ACK_Queue | 发送命令队列 (10 × `prama_Cmd_packet`) |

---

## 目录结构

```
C8T6_FreeRtos/
├── Core/
│   ├── Inc/                  # HAL 外设头文件 (gpio, usart, spi, dma, rtc)
│   └── Src/
│       ├── main.c            # 入口, 外设初始化, DMA 空闲中断回调
│       ├── freertos.c        # FreeRTOS 任务 + 菜单构建 + 协议回调 (880行)
│       ├── stm32f1xx_it.c    # 中断服务函数
│       └── usart.c           # USART1/USART2 初始化
├── protocol/
│   ├── protocol.h            # 帧协议定义 (CmdType, Rounter, VehicleStatus)
│   └── protocol.c            # 帧组包/解包, 注册制 Handler, 环形缓冲区
├── menu/
│   ├── menu.h                # 菜单系统 API (树状链表 + u8g2 渲染)
│   └── menu.c                # 菜单系统实现
├── u8g2/                     # u8g2 图形库 (SSD1306 驱动)
├── u8g2_user/                # u8g2 HAL 适配层 (SPI 引脚配置)
├── OLED/                     # 旧 OLED 驱动 (被 u8g2 替代, 保留兼容)
├── Log/                      # 日志输出模块
├── C8T6_FreeRtos.ioc         # STM32CubeMX 工程文件
└── MDK-ARM/                  # Keil MDK 工程文件
```

> **注意**: `multikey` 按键库不在本目录下, 已抽取到 `public_lib/multikey/` 共享。

---

## 当前菜单结构

菜单通过 `SLAVE_MODE` 宏区分站台模式和车辆模式, 两种模式编译出不同固件。

```
main_display  (主界面 - 显示时间 + 状态/乘客数)
  └── root  (main_menu)
        ├── sub1  (param_int) ─── 通信测试子菜单
        │     ├── sub1_sub1  SendUART_INT        [FUNCTION]  发送整数帧
        │     ├── sub1_sub2  Change_int           [PARAM_INT] 修改整数值
        │     ├── sub1_sub3  SendUART_FLOAT       [FUNCTION]  发送浮点帧
        │     └── sub1_sub4  SendUART_ACK         [FUNCTION]  发送 ACK 测试
        ├── sub2  (param_enum) ─── 字符串枚举测试
        │     └── sub2_sub1  Change_param         [PARAM_ENUM]
        ├── sub3  (toggle) ─── 开关测试
        ├── sub4  (Clock_Set) ─── 时钟设置
        │     ├── sub4_sub1  seconds              [PARAM_INT]  0-59
        │     ├── sub4_sub2  minutes              [PARAM_INT]  0-59
        │     ├── sub4_sub3  hours                [PARAM_INT]  0-23
        │     ├── sub4_sub4  years                [PARAM_INT]  0-99
        │     ├── sub4_sub5  monthes              [PARAM_INT]  1-12
        │     ├── sub4_sub6  days                 [PARAM_INT]  1-31
        │     └── sub4_sub7  Set_time             [FUNCTION]  写入 RTC
        └── sub5  (Set_passenger) ─── 乘客管理

# 站台模式 (SLAVE_MODE=0)                    # 车辆模式 (SLAVE_MODE=1)
  sub5_sub1  Set_R1_Passenger [PARAM_INT]       sub5_sub1  Waiting_Signal  [FUNCTION]
  sub5_sub2  Set_R2_Passenger [PARAM_INT]       sub5_sub2  Leaving_Signal  [FUNCTION]
  sub5_sub3  Set_R3_Passenger [PARAM_INT]
  sub5_sub4  Set_Ring_Passenger [PARAM_INT]
  sub5_sub5  SendUART_Passenger [FUNCTION]
  sub5_sub6  clear             [FUNCTION]
```

### 菜单节点类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `MENU_TYPE_MAIN` | 主界面, 用户自定义渲染 | 时间 + 状态/乘客数 |
| `MENU_TYPE_SUB_MENU` | 子菜单, 可设进入/退出回调 | Clock_Set |
| `MENU_TYPE_FUNCTION` | 功能节点, 点击执行回调 | SendUART_INT |
| `MENU_TYPE_PARAM_INT` | 整数参数, 可增减 + 范围限制 | seconds (0-59) |
| `MENU_TYPE_PARAM_ENUM` | 枚举参数, 从字符串列表选择 | Change_param |
| `MENU_TYPE_TOGGLE` | 开关, 布尔值切换 | toggle |

---

## UART 通信帧格式

帧格式 (十六进制):

```
+------+------+------+------+-----------+------+------+------+------+
| 0xAA | 0x55 | Type | Len  | Payload[] | CHK_H| CHK_L| 0x0D | 0x0A |
+------+------+------+------+-----------+------+------+------+------+
  帧头         命令    长度   数据 (0~10B)    校验和(大端)       帧尾
```

校验覆盖: Type + Len + Payload (不含帧头帧尾)

### 命令类型

| CmdType | 值 | Payload 长度 | 说明 |
|---------|------|-------------|------|
| INT | 0 | 4 | 整数值 (大端) |
| FLOAT | 1 | 4 | 浮点值 (大端 IEEE 754) |
| ACK | 2 | 0 | 确认帧 |
| PASSENGER_NUM | 3 | 2 | [路线上 1B, 乘客数 1B] |
| CLEAR | 4 | 1 | [路线上 1B] 清空该路线乘客 |
| VEHICLE_STATUS | 5 | 1 | [状态值 1B] 车辆状态变化 |

### 通信流程

```
STM32                    ESP32C3 站台                  ESP32C3_Slave 车辆
  │                          │                              │
  │── PASSENGER_NUM ────────▶│ (通过 UART)                  │
  │                          │── VEHICLE_STATUS ──────────▶│ (通过 WiFi)
  │                          │◀── ACK ────────────────────│
  │◀── ACK ─────────────────│                              │
  │                          │                              │
  │                          │◀── /api/vehicle_report ────│ (HTTP POST)
  │                          │── JSON response ───────────▶│
```

- **ACK 重传**: 最多重试 3 次, 每次超时 200ms
- **UART 接收**: USART1 DMA + 空闲中断 → 环形缓冲区 → 事件通知 UART_TASK
- **发送保护**: USART1 TX 操作使用互斥量 `UART_TXMute`
- **乘客同步**: UART_TASK 每秒检查 `passenger_num[]` 是否变化, 变化则入队发送
- **ORE 恢复**: UART_TASK 检测到溢出错误时自动清除标志并重启 DMA

---

## 编译配置

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `SLAVE_MODE` | 0 | 0=站台模式, 1=车辆模式 |
| `MAX_PAYLOAD_LEN` | 10 | 帧 Payload 最大长度 |
| `MAX_CMD_TYPE_HANDLERS` | 20 | 最大命令类型数 |

---

## 已知问题与待改进

1. `freertos.c` 880 行过重, 需拆分为 `menu_init.c` / `uart_callbacks.c` / `ack_task.c`
2. `protocol.c` 与 `public_lib/protocol` 是不同实现, 需统一
3. 站点列表硬编码在 ESP32_Slave 端, STM32 端不需要关心
4. ORE 恢复逻辑在 UART_TASK 和 `HAL_UART_ErrorCallback` 中重复

详见 [ROADMAP.md](../ROADMAP.md) (待创建)
