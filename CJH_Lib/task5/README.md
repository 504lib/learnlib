# 任务三：旁路设计
[toc]
***
## 项目介绍
 **一. 说明**
    实现了LED0和LED1的闪烁
**二. 具体实现**
    1. LED0以500ms为周期执行闪烁
    2. LED1以1000ms为周期执行闪烁
    3. 两个LED的闪烁必须完全独立，互不影响
    4. 禁止使用?HAL_Delay()
    5. 禁止使用中断/定时器
**三. 项目目的**
    练习非堵塞式延时
***
## 引脚说明
|GPIO|名称|功能|
| - | - | - |
| PC2 | LED0 | LED0的亮灭 |
| PC3 | LED1 | LED1的亮灭 |
| PA13 | SYS_JIMS-SWDIO | 串行线调试 |
| PA14 | SYS_JIMS-SWCLK | 串行线调试 |
| PD0 | RCC_OSC_IN | 时钟配置 |
| PD1 | RCC_OSC_OUT | 时钟配置 |
***
## 关键逻辑介绍
```c
#define big_delay 500
#define small_delay 250
uint32_t pre = 0; //LED0上一次的延时时间
uint32_t pre1 = 0; //LED1上一次的延时时间
  while (1)
  {
	  uint32_t cur = HAL_GetTick();
	  if(cur - pre >= big_delay) //大于等于500ms就执行
	  {
		  pre = cur; //更新LED0的时间
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin); //翻转状态
	  }
	  	  if(cur - pre1 >= small_delay) //大于等于250ms就执行
	  {
		  pre1 = cur; //更新LED1的时间
		  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); //翻转状态
	  }
  }
```
---
CJH: 2025年9月13日 15:23
