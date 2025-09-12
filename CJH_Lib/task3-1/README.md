# 任务三：旁路设计
[toc]
***
## 项目介绍
 **一. 说明**
    实现了LED0的闪烁，KEY0控制LED1的亮灭且互不影响
**二. 具体实现**
    1. 实现LED0始终闪烁，周期为500ms
    2. 当按键被按下(完整动作，松手触发按下触发自定)，控制LED1亮灭(toggle)，且不干扰LED0闪烁的执行
    3. 按键响应速度在100~200ms以内(即即可响应)，按下按键立刻改变LED1的状态
    4. 命名为"task3"，并且成功提出PR并通过
    5. **禁止**使用中断，定时器等外设
**三. 项目目的**
    练习非堵塞式延时和状态切换
***
## 引脚说明
|GPIO|名称|功能|
| - | - | - |
| PC2 | LED0 | LED0的亮灭 |
| PC3 | LED1 | LED1的亮灭 |
| PA0 | KEY0 | 控制LED1的亮灭 |
| PA13 | SYS_JIMS-SWDIO | 串行线调试 |
| PA14 | SYS_JIMS-SWCLK | 串行线调试 |
| PD0 | RCC_OSC_IN | 时钟配置 |
| PD1 | RCC_OSC_OUT | 时钟配置 |
***
## 逻辑介绍
```c
#define delay 500
uint32_t last = 0; //上一次的延时时间
int pree = 0; //上一次的按键状态
int now = 0; //现在的按键状态
  while (1)
  {
	  uint32_t cur = HAL_GetTick(); //获取运行时间函数定义为cur
	  if(cur - last >= delay) //判断是否达到500ms
	  {
		  last = cur; //更新状态
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
	  }
	  now = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) ;//定义KEY0的状态为now
	  if(now != pree) //判断状态是否相等
	  {
		  pree = now; //更新状态
		  if(now == 0) //如果处于低电平
		  {
			  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); //翻转状态
		  }
	  }
  }
```
---
CJH: 2025年9月12日 19:41