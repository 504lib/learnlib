# MultiKey 按键处理库
一个轻量级的按键处理库，支持按键消抖、短按、长按和长按重复触发功能。

## 功能特性
✅ 支持按键消抖处理

✅ 短按事件回调

✅ 长按事件回调

✅ 长按重复触发

✅ 支持上升沿/下降沿触发

✅ 多平台支持（FreeRTOS、裸机、自定义）

✅ 可配置的时间参数

## 快速开始
1. 包含头文件
```c
#include "multikey.h"
```
2. 配置平台
在 multikey.h 中选择适合的平台：

```c
// 选择其中之一
#define MENU_USE_CMSIS_OS2    // 使用 CMSIS-OS2 (FreeRTOS)
#define MENU_USE_BARE_METAL   // 使用裸机系统 (HAL库)
#define MENU_USE_CUSTOM       // 使用自定义配置
```
3. 定义按键读取函数
```c
uint8_t ReadKeyPin(MulitKey_t* key) {
    // 根据具体的按键GPIO返回状态
    return HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
}
```
4. 定义事件回调函数
```c
// 短按回调
void OnKeyPressed(MulitKey_t* key) {
    printf("Key pressed!\n");
}

// 长按回调
void OnKeyLongPressed(MulitKey_t* key) {
    printf("Key long pressed!\n");
}
```
5. 初始化和使用
```c
MulitKey_t myKey;

int main(void) {
    // 初始化按键（下降沿触发）
    MulitKey_Init(&myKey, ReadKeyPin, OnKeyPressed, OnKeyLongPressed, FALL_BORDER_TRIGGER);
    
    while(1) {
        // 定期调用按键扫描
        MulitKey_Scan(&myKey);
        HAL_Delay(10);  // 建议扫描间隔10ms
    }
}
```
## API 参考
### 初始化函数
```c
void MulitKey_Init(MulitKey_t* key, 
                   KeyReadPinCallback readPin,
                   KeyPressdCallback onPressed,
                   KeyLongPressdCallback onLongPressed,
                   BorderTrigger trigger);
```

+ `MulitKey_t* key`:所在按钮的对象
+ `KeyReadPinCallback readPin`:读取函数的函数名称(指针)
+ `KeyPressdCallback onPressed`:短按回调函数的函数名称(指针)
+ `KeyLongPressdCallback onLongPressed`:长按回调函数的函数名称(指针)
+ `BorderTrigger trigger`:边缘选择触发
### 按键扫描函数
```c
void MulitKey_Scan(MulitKey_t* key);
```
+ `MulitKey_t* key`:对应的按键实例地址

### 时间参数设置
```c
void MulitKey_SetDebounceTime(uint16_t time);        // 设置消抖时间(ms)
void MulitKey_SetLongPressTime(uint16_t time);       // 设置长按时间(ms)
void MulitKey_SetLongPressRepeatTime(uint16_t time); // 设置长按重复触发时间(ms)
```
## 配置参数
默认时间参数（可在 multikey.h 中修改）：
```c
#define KEY_DEBOUNCE_TIME 10          // 消抖时间 10ms
#define KEY_LONGPRESS_TIME 500        // 长按时间 500ms  
#define KEY_LONGPRESS_REPEAT_TIME 100 // 长按重复触发时间 100ms
```
### 触发模式
```c
typedef enum{
    RISE_BORDER_TRIGGER = 0,  // 上升沿触发（低电平->高电平）
    FALL_BORDER_TRIGGER,      // 下降沿触发（高电平->低电平）
} BorderTrigger;

```
### 状态机说明
按键处理使用状态机，包含以下状态：

`KEY_IDLE`: 空闲状态，等待按键触发

`KEY_DEBOUNCE`: 消抖状态，确认按键稳定

`KEY_PRESSED`: 短按状态，已触发短按回调

`KEY_LONGPRESS`: 长按状态，可重复触发长按回调

### 注意事项
+ 定期调用: MulitKey_Scan() 需要定期调用，建议调用间隔为10ms

+ 时间单位: 所有时间参数的单位为毫秒(ms)

+ 回调函数: 回调函数在中断上下文中执行，应保持简短

+ 多按键: 支持多个按键，每个按键需要单独的 MulitKey_t 实例

## 示例代码
完整的示例代码请example文件夹。