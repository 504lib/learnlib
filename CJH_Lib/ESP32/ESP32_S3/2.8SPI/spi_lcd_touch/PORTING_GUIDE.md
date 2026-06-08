# ESP32-S3 + MSP2833/MSP2834 (ILI9341) + FT6336 触摸屏 移植总结

## 项目概览

将 Arduino 框架的 MSP2833/MSP2834 SPI LCD 触摸屏 Demo 移植到 ESP-IDF (v6.0.1) 框架，运行在立创 ESP32-S3-WROOM-1-N8R8 开发板上。

| 组件 | 接口 | 驱动芯片 | 分辨率 |
|------|------|----------|--------|
| LCD 显示屏 | SPI | ILI9341 (兼容 MSP2833/MSP2834) | 240×320 |
| 触摸面板 | I2C | FT6336 (FT5x06 系列) | 240×320 |

---

## 一、问题时间线

### 1. 框架差异分析（可行，需要改）

**原因：** Demo 是 Arduino 框架（`.ino` + TFT_eSPI 库），目标工程 `spi_lcd_touch` 是 ESP-IDF 框架。

**解决：** 不新建项目，直接修改现有的 `spi_lcd_touch` 例程。ESP-IDF 有对应的官方组件：
- LCD：`espressif/esp_lcd_ili9341`
- 触摸：`espressif/esp_lcd_touch_ft5x06`（FT6336 属于 FT5x06 系列）

### 2. ESP-IDF v6.0.1 API 不兼容（编译报错）

**原因：** `esp_lcd_ili9341` 组件发布于 ESP-IDF v5.x 时代，v6.0.1 把 `rgb_endian` 字段改名为 `rgb_ele_order`。

**修复：** 在 `managed_components/espressif__esp_lcd_ili9341/esp_lcd_ili9341.c` 中添加版本判断：
```c
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    // 旧 API: color_space
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
    // v5.x: rgb_endian
#else
    // v6.x: rgb_ele_order
#endif
```

### 3. 头文件名不一致（编译报错）

**原因：** ST7789 的头文件叫 `esp_lcd_panel_st7789.h`（带 `_panel`），但 ILI9341 的叫 `esp_lcd_ili9341.h`（不带 `_panel`）。

**修复：** `#include "esp_lcd_panel_ili9341.h"` → `#include "esp_lcd_ili9341.h"`

### 4. 触摸 I2C 引脚不兼容（运行时崩溃）

**原因：** Demo 原引脚 GPIO 25/32/33 在 ESP32-S3-WROOM-1 模组上不可用：
- GPIO 25/32：WROOM-1 模组未引出
- GPIO 33：被 Octal PSRAM 占用

**修复：** 改用 GPIO 5 (SCL)、GPIO 6 (SDA)、GPIO 7 (RST)，且在代码中硬编码避免 sdkconfig 配置不同步。

**教训：** Kconfig 改默认值后必须删除 `sdkconfig` 重新生成，否则旧值不会更新。教训是直接硬编码更可靠。

### 5. 显示屏白屏闪烁（初始化序列不匹配）

**原因：** MSP2833/MSP2834 需要**专用的初始化序列**，与 ILI9341 通用默认值不同。关键差异在电源控制、VCOM、Gamma、以及 `0xF6`（接口控制）寄存器。

**修复：** 使用 `ili9341_vendor_config_t` 传入 Demo 中的自定义初始化命令表。

**教训：** 只要颜色测试（直接写 `esp_lcd_panel_draw_bitmap`）能正确显示红绿蓝白，就说明 SPI 通信和色彩格式没问题。白屏一定是 LVGL 层面的问题。

### 6. 颜色正常但 LVGL 白屏

**原因（多层次）：**
- **触摸初始化崩溃**：`#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED` 为 true 但 I2C 引脚无效，程序在初始化阶段就 `abort()` 了，根本到不了 LVGL 渲染。日志里一直有 `ESP_ERROR_CHECK failed` 但我们忽略了。
- **红屏测试只显示底部**：LVGL 默认 20 行缓冲区导致 SPI DMA 分块太多，ESP32-S3 上部分传输失败。

**修复：** 先 `#if 0` 关闭触摸验证 LVGL 能正常运行，再加大缓冲区到 50 行。

### 7. 屏幕方向错误

**原因：** 
- `example_lvgl_port_update_callback` 在每次 LVGL flush 时被调用，强制覆盖了我们设置的 mirror/swap 参数。
- `swap_xy(true)` 导致行列交换，物理 240×320 屏幕被当成 320×240，坐标越界只显示半屏。

**修复：** 
- 从 flush 回调中移除 `example_lvgl_port_update_callback` 调用。
- 使用 `swap_xy(false)`，然后通过 `mirror` 的 4 种组合找到正确方向。

### 8. 触摸不响应

**原因：** 新 API `esp_lcd_touch_get_data()` 只**解析**已读取的数据，不会主动从硬件读取。必须先用 `esp_lcd_touch_read_data()` 读取硬件。

**修复：** 在 get_data 前加回 read_data 调用：
```c
esp_lcd_touch_read_data(touch_pad);  // 读硬件
esp_lcd_touch_get_data(touch_pad, point_data, &touchpad_cnt, 1);  // 解析数据
```

### 9. 触摸坐标错位

**原因：** 触摸面板的坐标系统与显示屏的镜像设置不匹配。显示屏 `mirror(true, false)` 但触摸 `mirror_y=0`，导致 Y 轴反向。

**修复：** 将触摸配置的 `mirror_x` 和 `mirror_y` 与显示屏对齐。

---

## 二、系统排查方法论

### 遇到问题时，按这个顺序排查：

```
1. 看日志！不要猜
   ↓
2. 隔离问题：先不用高级功能，用最简单的方式验证硬件
   ↓  
3. 对比测试：能工作的代码 vs 不能工作的代码，找差异
   ↓
4. 一次只改一个变量
   ↓
5. 搜资料：英文关键词优于中文，官方文档优先于博客
```

### 显示问题排查链：

```
SPI 通信正常？
  → 写颜色测试（直接调用 esp_lcd_panel_draw_bitmap 画纯色）
  → 颜色正确？说明 SPI + 初始化 OK
  → 颜色不对？查 RGB/BGR 顺序、SPI 模式、初始化序列

LVGL 问题？
  → 先用 LVGL 画纯色背景（绕过复杂 UI）
  → 纯色正常？说明 LVGL 渲染管线 OK
  → 白屏？检查：触摸是否崩溃、缓冲区大小、DMA 配置
```

### 触摸问题排查链：

```
I2C 通信正常？
  → 看日志有无 I2C 报错
  → 有报错？检查引脚、接线、上拉电阻

驱动初始化成功？
  → 日志有无 "Initialize touch controller FT5x06" 
  → 无？检查组件依赖、sdkconfig

数据读取？
  → 确认 read_data() + get_data() 都调用了
  → 确认 LVGL input device 正确注册
  → 添加日志看 callback 是否被调用

坐标正确？
  → 触摸 mirror/swap 与显示屏一致
```

---

## 三、最终引脚接线表

### LCD (SPI)

| 屏幕丝印 | GPIO | 代码宏 |
|----------|------|--------|
| LCD_RS (DC) | 2 | EXAMPLE_PIN_NUM_LCD_DC |
| LCD_RST | 4 | EXAMPLE_PIN_NUM_LCD_RST |
| LCD_CS (SD_CS) | 15 | EXAMPLE_PIN_NUM_LCD_CS |
| SCK | 14 | EXAMPLE_PIN_NUM_SCLK |
| SDI (MOSI) | 13 | EXAMPLE_PIN_NUM_MOSI |
| SDO (MISO) | 12 | EXAMPLE_PIN_NUM_MISO |
| LED | 21 | EXAMPLE_PIN_NUM_BK_LIGHT |

### 触摸 (I2C)

| 屏幕丝印 | GPIO |
|----------|------|
| CTP_SCL | 5 |
| CTP_SDA | 6 |
| CTP_RST | 7 |
| CTP_INT | 不接 |

---

## 四、需要的知识储备

| 知识点 | 用途 | 学习资源 |
|--------|------|----------|
| ESP-IDF 项目结构 | 理解 CMake、组件管理 | ESP-IDF 官方文档 |
| SPI 通信基础 | 排查显示问题 | `esp_lcd` API 文档 |
| I2C 通信基础 | 排查触摸问题 | `i2c_master` API 文档 |
| LVGL 显示框架 | UI 开发 | lvgl.io 官方文档 |
| ESP32-S3 引脚功能 | 硬件选型 | ESP32-S3 数据手册 |
| 看芯片数据手册 | 寄存器/时序 | LCD 控制器 datasheet |
| 读启动日志 | 定位崩溃点 | ESP-IDF monitor 输出 |

### 怎么搜资料：

1. **英文关键词优先**：`ESP32-S3 ILI9341 LVGL ESP-IDF example` 比中文搜索结果质量高
2. **官方文档第一**：`docs.espressif.com`、`components.espressif.com`、`lvgl.io`
3. **GitHub 源码**：搜索 `esp_lcd_touch_ft5x06` 看源码和 issue
4. **LVGL Forum**：`forum.lvgl.io` 有大量 ESP32 相关讨论
5. **CSDN/博客园**：中文场景的具体案例，但信息质量参差不齐

---

## 五、关键文件修改清单

| 文件 | 修改内容 |
|------|----------|
| `main/idf_component.yml` | 替换依赖：ILI9341 + FT5x06 |
| `main/Kconfig.projbuild` | 触摸引脚配置 |
| `main/spi_lcd_touch_example_main.c` | 全部核心代码：引脚、初始化、LVGL UI |
| `main/lv_conf.h` | LVGL 配置覆盖（未使用） |
| `managed_components/.../esp_lcd_ili9341.c` | 修复 v6.0.1 API 兼容 |

---

## 六、未完成 / 待优化

- [ ] `CONFIG_EXAMPLE_LCD_TOUCH_ENABLED` 仍依赖 sdkconfig，建议改为无条件编译
- [ ] 触摸镜像参数可能需要根据实际使用微调
- [ ] 字体大小可配置更大的（需要启用对应 LVGL 字体）
- [ ] 可考虑启用 PSRAM 获得更多内存
