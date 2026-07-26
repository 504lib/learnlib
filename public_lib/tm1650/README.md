# TM1650 四位数码管驱动库

[![Language](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/platform-any-lightgrey.svg)](#硬件适配)
[![License](https://img.shields.io/badge/license-Public%20Domain-green.svg)]()

跨平台的 TM1650 四位数码管驱动 C 库。采用**硬件抽象层**设计，不依赖特定 MCU 平台。

> 📦 资料目录：`../新建文件夹/` — 含使用手册、原理图、器件手册、51/Arduino 例程

---

## 目录

- [硬件简介](#硬件简介)
- [特性](#特性)
- [文件结构](#文件结构)
- [快速开始](#快速开始)
  - [STM32 (HAL)](#stm32-hal-示例)
  - [51 单片机](#51-单片机示例)
  - [Arduino](#arduino-示例)
- [API 参考](#api-参考)
  - [查表函数](#查表函数)
  - [初始化与配置](#初始化与配置)
  - [低层显示操作](#低层显示操作)
  - [高层显示操作](#高层显示操作)
  - [按键扫描](#按键扫描)
  - [调试测试](#调试测试)
- [段码参考](#段码参考)
- [硬件接线](#硬件接线)
- [设计说明](#设计说明)
- [兼容性](#兼容性)
- [许可协议](#许可协议)

---

## 硬件简介

**TM1650** 是天微电子推出的一款 LED 数码管驱动 IC，特性如下：

| 项目 | 参数 |
|------|------|
| 通信接口 | I2C-like 两线串行（SCK + DIO） |
| 数码管位数 | 4 位（8段/7段可配） |
| 亮度等级 | 8 级 |
| 按键扫描 | 最多支持 4 个按键 |
| 工作电压 | 3.0V ~ 5.5V |
| 低功耗 | 支持待机模式 |

**通信协议要点：**

```
帧格式: [START] → [CMD(8bit)] → [ACK] → [DATA(8bit)] → [ACK] → [STOP]
              ↓ MSB先发 ↓                      ↓ MSB先发 ↓

时序: SCK高电平期间，DIO的跳变决定START(↓)或STOP(↑)
      数据在SCK上升沿被采样，MSB先发送
      每字节后跟1位ACK（从机将DIO拉低应答）
```

---

## 特性

- ✅ **跨平台**：通过函数指针注入 GPIO 操作，支持 STM32 / 51 / Arduino / RISC-V / Linux 等
- ✅ **完整的 API**：从原始段码到 `display_integer(1234)` 一句话显示
- ✅ **自动译码**：0-9 / A-F / 负号 / 小数点 自动转换段码
- ✅ **按键扫描**：完整支持带 4 个按键的模块版本
- ✅ **多实例**：一个程序可同时驱动多片 TM1650
- ✅ **防御式编程**：所有函数入参校验，通信超时保护
- ✅ **零依赖**：仅需 `stdint.h` + `stdbool.h`
- ✅ **兼容旧版 API**：提供旧版宏映射（`LV1` → `TM1650_LV1` 等）

---

## 文件结构

```
tm1650/
├── tm1650.h      # 头文件：宏定义 / 结构体 / API 声明 / 段码表
├── tm1650.c      # 实现文件：通信协议 / 显示逻辑 / 按键扫描
└── README.md     # 本说明文档
```

---

## 快速开始

### 核心概念：三个回调函数

使用本库只需提供 **3 个回调函数**：

```c
// 1. 写GPIO引脚
void my_pin_write(void *user_data, bool level);

// 2. 读GPIO引脚
bool my_pin_read(void *user_data);

// 3. 微秒延时
void my_delay_us(uint32_t us);
```

### 通用步骤

```c
#include "tm1650.h"

// 1) 定义硬件实例和抽象层
tm1650_t       disp;
tm1650_hal_t   hal;

// 2) 填充硬件接口
hal.pin_write_sck = my_pin_write;
hal.pin_write_dio = my_pin_write;
hal.pin_read_dio  = my_pin_read;
hal.delay_us      = my_delay_us;
hal.user_data_sck = &sck_pin_id;   // 传递给回调的参数
hal.user_data_dio = &dio_pin_id;

// 3) 初始化
tm1650_init(&disp, &hal);

// 4) 使用
tm1650_set_brightness(&disp, TM1650_LV4);
tm1650_display_integer(&disp, 1234, false);   // 显示 "1234"
```

---

### STM32 HAL 示例

```c
#include "tm1650.h"
#include "main.h"

/* ---- 引脚定义 ---- */
#define TM1650_SCK_PIN   GPIO_PIN_0
#define TM1650_SCK_PORT  GPIOA
#define TM1650_DIO_PIN   GPIO_PIN_1
#define TM1650_DIO_PORT  GPIOA

/* ---- 硬件回调 ---- */
static void gpio_write(void *data, bool level)
{
    GPIO_PinState state = level ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin((GPIO_TypeDef *)data,
                      (uint32_t)(uintptr_t)((struct {GPIO_TypeDef *port; uint16_t pin;}*)data)->pin,
                      state);
}

static bool gpio_read(void *data)
{
    // 先切换DIO为输入模式
    return HAL_GPIO_ReadPin(...) == GPIO_PIN_SET;
}

static void delay_us(uint32_t us)
{
    // 使用定时器微秒延时或 DWT
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < us * (SystemCoreClock / 1000000));
}

/* ---- 使用 ---- */
tm1650_t g_display;

void app_init(void)
{
    tm1650_hal_t hal = {
        .pin_write_sck = gpio_write,
        .pin_write_dio = gpio_write,
        .pin_read_dio  = gpio_read,
        .delay_us      = delay_us,
        .user_data_sck = ...,
        .user_data_dio = ...,
    };
    tm1650_init(&g_display, &hal);
    tm1650_set_brightness(&g_display, TM1650_LV5);
}

void show_temperature(float temp)
{
    int16_t val = (int16_t)(temp * 10);  // 保留1位小数
    tm1650_display_integer_with_dot(&g_display, val, 3);  // 小数点在第3位
}
```

---

### 51 单片机示例

```c
#include <reg52.h>
#include "tm1650.h"

/* 引脚定义 */
sbit SCK = P1 ^ 0;
sbit DIO = P1 ^ 1;

/* 硬件回调 - SCK */
void sck_write(void *data, bool level) { SCK = level; }

/* 硬件回调 - DIO (写) */
void dio_write(void *data, bool level) { DIO = level; }

/* 硬件回调 - DIO (读) */
bool dio_read(void *data)
{
    DIO = 1;     // 先置1释放总线
    return DIO;  // 读回
}

/* 硬件回调 - 延时 */
void my_delay(uint32_t us)
{
    while (us--) {
        unsigned char i = 5;   // 根据主频调整
        while (i--);
    }
}

void main()
{
    tm1650_t disp;
    tm1650_hal_t hal = {
        .pin_write_sck = sck_write,
        .pin_write_dio = dio_write,
        .pin_read_dio  = dio_read,
        .delay_us      = my_delay,
        .user_data_sck = NULL,
        .user_data_dio = NULL,
    };

    tm1650_init(&disp, &hal);
    tm1650_set_brightness(&disp, TM1650_LV4);

    while (1) {
        for (int i = 0; i <= 9999; i++) {
            tm1650_display_integer(&disp, i, false);
            // delay ~100ms
        }
    }
}
```

---

### Arduino 示例

```c
#include "tm1650.h"

const int SCK_PIN = 9;
const int DIO_PIN = 10;

void sck_write(void *data, bool level) {
    digitalWrite(*(int *)data, level ? HIGH : LOW);
}
void dio_write(void *data, bool level) {
    digitalWrite(*(int *)data, level ? HIGH : LOW);
}
bool dio_read(void *data) {
    digitalWrite(*(int *)data, HIGH);  // 释放
    pinMode(*(int *)data, INPUT);
    bool val = digitalRead(*(int *)data);
    pinMode(*(int *)data, OUTPUT);
    return val;
}
void my_delay(uint32_t us) { delayMicroseconds(us); }

tm1650_t disp;

void setup() {
    pinMode(SCK_PIN, OUTPUT);
    pinMode(DIO_PIN, OUTPUT);

    tm1650_hal_t hal = {
        .pin_write_sck = sck_write,
        .pin_write_dio = dio_write,
        .pin_read_dio  = dio_read,
        .delay_us      = my_delay,
        .user_data_sck = &SCK_PIN,
        .user_data_dio = &DIO_PIN,
    };
    tm1650_init(&disp, &hal);
    tm1650_display_integer(&disp, 8888, false);  // 自检全亮
}

void loop() {
    // 显示运行秒数
    tm1650_display_integer(&disp, millis() / 1000, false);
    delay(200);
}
```

---

## API 参考

### 查表函数

| 函数 | 说明 |
|------|------|
| `uint8_t tm1650_seg_for_digit(uint8_t num)` | 数字 0-9 → 段码 |
| `uint8_t tm1650_seg_for_hex(uint8_t hex_val)` | 十六进制 0x0-0xF → 段码 |
| `uint8_t tm1650_seg_for_char(char ch)` | 任意字符 → 段码（支持数字、A-F、'-'） |

### 初始化与配置

| 函数 | 说明 |
|------|------|
| `bool tm1650_init(tm1650_t *self, const tm1650_hal_t *hal)` | 初始化设备实例，发送默认配置 |
| `bool tm1650_config(tm1650_t *self, brightness, seg_mode, work_mode, display_on)` | 完整配置系统参数 |
| `bool tm1650_set_brightness(tm1650_t *self, uint8_t brightness)` | 快捷调亮度（`TM1650_LV1` ~ `TM1650_LV8`） |
| `bool tm1650_display_on(tm1650_t *self)` | 打开显示 |
| `bool tm1650_display_off(tm1650_t *self)` | 关闭显示 |

### 低层显示操作

| 函数 | 说明 |
|------|------|
| `bool tm1650_display_segment(self, position, seg_data)` | 指定位置写入原始段码 |
| `bool tm1650_display_segments(self, seg0, seg1, seg2, seg3)` | 同时刷4位段码 |
| `bool tm1650_clear(self)` | 全屏熄灭 |

### 高层显示操作

| 函数 | 说明 |
|------|------|
| `bool tm1650_display_integer(self, number, leading_zero)` | 显示整数（0-9999），可选前导零 |
| `bool tm1650_display_integer_with_dot(self, number, dot_pos)` | 显示整数+指定小数点位置 |
| `bool tm1650_display_string(self, str)` | 显示字符串（最多4字符，自动译码） |

### 按键扫描

| 函数 | 说明 |
|------|------|
| `uint8_t tm1650_scan_key(self)` | 读按键，返回低4位（bit0=KEY1, ..., bit3=KEY4） |

### 调试测试

| 函数 | 说明 |
|------|------|
| `void tm1650_test_segments(self, delay_ms)` | 逐段跑马灯测试（阻塞） |

---

## 段码参考

### 段位映射

```
    [dp] [g] [f] [e] [d] [c] [b] [a]
    0x80 0x40 0x20 0x10 0x08 0x04 0x02 0x01

         a
       ━━━━
     f ┃   ┃ b
       ━━━━  g
     e ┃   ┃ c
       ━━━━  · dp
         d
```

### 数字段码速查

| 字符 | 段码 | 二进制 (dp-g-f-e-d-c-b-a) |
|------|------|---------------------------|
| `0` | `0x3F` | `0 0 1 1 1 1 1 1` |
| `1` | `0x06` | `0 0 0 0 0 1 1 0` |
| `2` | `0x5B` | `0 1 0 1 1 0 1 1` |
| `3` | `0x4F` | `0 1 0 0 1 1 1 1` |
| `4` | `0x66` | `0 1 1 0 0 1 1 0` |
| `5` | `0x6D` | `0 1 1 0 1 1 0 1` |
| `6` | `0x7D` | `0 1 1 1 1 1 0 1` |
| `7` | `0x07` | `0 0 0 0 0 1 1 1` |
| `8` | `0x7F` | `0 1 1 1 1 1 1 1` |
| `9` | `0x6F` | `0 1 1 0 1 1 1 1` |
| `A` | `0x77` | `0 1 1 1 0 1 1 1` |
| `-` | `0x40` | `0 1 0 0 0 0 0 0` |
| 全灭 | `0x00` | `0 0 0 0 0 0 0 0` |

---

## 硬件接线

### TM1650 四位数码管模块

```
     TM1650 模块
  ┌─────────────┐
  │   □□□□      │  4位数码管
  │  ┌──┐ ┌──┐ │
  │  │  │ │  │ │  4个微动按键（部分版本）
  │  └──┘ └──┘ │
  └─────────────┘
   |  |  |  |
   VCC GND SCL SDA (或 SCK / DIO)

接线:
  VCC  → 3.3V / 5V
  GND  → GND
  SCL  → MCU GPIO (SCK)
  SDA  → MCU GPIO (DIO)
```

⚠️ **注意**：DIO 是双向开漏引脚，部分平台需要切换 GPIO 方向（输出/输入），请在读写回调中处理。

### 模块版本差异

| 版本 | 数码管 | 按键 | 原理图 |
|------|--------|------|--------|
| 标准版 | 4位8段 | 无 | `02-原理图/TM1650四位数码管模块原理图.pdf` |
| 带按键版 | 4位8段 | 4个 | `02-原理图/TM1650四位数码管带四个按键模块原理图.pdf` |

---

## 设计说明

### 为什么用函数指针而不是宏？

本项目的其他库（`multikey`、`protocol`）均采用 **struct + 回调函数** 的硬件抽象模式：

- **宏方案**（如直接 `#define SCK P1^0`）：锁定特定平台，换MCU要改代码
- **回调方案**（本库）：只需改三个函数，核心逻辑零改动

### 防御式编程

每个公开 API 的入口都有空指针检查：

```c
#define CHECK_NULL(ptr) do { if ((ptr) == NULL) { return false; } } while (0)
```

通信失败也通过返回值上报，不吞错误。

### .h/.c 分离原则（来自项目驱动编写规范）

- `.h` 文件：完整的接口文档、宏定义、结构体定义 — **看.h就能用库**
- `.c` 文件：纯通信协议框架 — **没有一行业务逻辑**

---

## 兼容性

本库与旧版 TM1650 参考代码（`新建文件夹/51单片机例程/`）的宏命名兼容：

```c
// 旧版代码中的宏自动映射到新命名
LV1    → TM1650_LV1
LV8    → TM1650_LV8
_8_SEGMENT_MODE → TM1650_8_SEGMENT_MODE
// ... 等等
```

迁移时只需替换函数调用为 `tm1650_xxx` 风格即可。

---

## 许可协议

Public Domain — 可自由使用、修改和分发。

硬件资料（原理图、器件手册）位于 `../新建文件夹/`，版权归原作者所有。

---

## 参考资料

- TM1650 数据手册：`../新建文件夹/06-器件手册/TM1650_V2.0.pdf`
- 原理图：`../新建文件夹/02-原理图/`
- 使用说明：`../新建文件夹/01-使用手册/`
- 51 单片机参考例程：`../新建文件夹/51单片机例程/`
- Arduino 参考例程：`../新建文件夹/Arduino例程/`
