# Multikey - 多按键处理库

这是一个共享的按键处理库，支持按键消抖、单按和长按检测。

## 特性

- 按键消抖处理
- 单按检测
- 长按检测
- 长按重复触发
- 支持上升沿和下降沿触发
- 支持多种平台（裸机、FreeRTOS CMSIS_OS2、自定义）

## 使用方法

### 1. 配置平台

在项目中定义以下宏之一来选择平台：

- `MENU_USE_BARE_METAL` - 使用裸机平台（STM32 HAL）
- `MENU_USE_CMSIS_OS2` - 使用 FreeRTOS CMSIS_OS2
- `MENU_USE_CUSTOM` - 自定义平台（需要自己定义 GetTick 宏）

如果不定义任何宏，默认使用 `MENU_USE_BARE_METAL`。

### 2. 包含头文件

在你的项目中使用相对路径包含头文件：

```c
#include "../../shared/multikey/multikey.h"
```

或者将 `ZK_Lib/shared/multikey` 添加到你的项目包含路径中：

```c
#include "multikey.h"
```

// 对于 FreeRTOS (CMSIS_OS2) 项目，请在包含头文件前定义宏：

```c
// For FreeRTOS projects:
#define MENU_USE_CMSIS_OS2
#include "../../shared/multikey/multikey.h"
### 3. 初始化按键

```c
// 定义按键对象
MulitKey_t key1;

// 定义读取引脚的回调函数
uint8_t readKeyPin(MulitKey_t* key) {
    return HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
}

// 定义单按回调函数
void onKeyPressed(MulitKey_t* key) {
    // 处理单按事件
}

// 定义长按回调函数
void onKeyLongPressed(MulitKey_t* key) {
    // 处理长按事件
}

// 初始化按键
MulitKey_Init(&key1, readKeyPin, onKeyPressed, onKeyLongPressed, FALL_BORDER_TRIGGER);
```

### 4. 周期性调用扫描函数

在定时器中断或主循环中周期性调用：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim1) {
        MulitKey_Scan(&key1);
    }
}
```

## API 参考

### MulitKey_Init
初始化按键对象

```c
void MulitKey_Init(MulitKey_t* key, 
                   KeyReadPinCallback readPin,
                   KeyPressdCallback onPressed,
                   KeyLongPressdCallback onLongPressed,
                   BorderTrigger trigger);
```

### MulitKey_Scan
扫描按键状态（需要周期性调用）

```c
void MulitKey_Scan(MulitKey_t* key);
```

### MulitKey_SetDebounceTime
设置消抖时间

```c
void MulitKey_SetDebounceTime(uint16_t time);
```

### MulitKey_SetLongPressTime
设置长按检测时间

```c
void MulitKey_SetLongPressTime(uint16_t time);
```

### MulitKey_SetLongPressRepeatTime
设置长按重复触发时间

```c
void MulitKey_SetLongPressRepeatTime(uint16_t time);
```

## 从旧版本迁移

如果你的项目中使用了旧的本地 multikey 副本，请按以下步骤迁移：

1. 删除项目中的本地 `multikey` 目录
2. 更新包含路径指向共享库
3. 如果使用 FreeRTOS，在源文件中包含头文件前定义 `MENU_USE_CMSIS_OS2`，或在编译选项中定义
4. 重新编译项目

## 注意事项

- 确保选择正确的平台宏定义
- 确保沿边触发方式与实际电路相符
- 扫描周期建议在 1-10ms 之间
