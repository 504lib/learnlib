# 任务六：数值变化
[toc]
***
## 项目介绍
 **一. 说明**
    实现了LED0的多种周期闪烁
**二. 具体实现**
    1. LED0开始以500ms为周期执行闪烁
	2. 按键0按下(完整动作，按下触发松手触发自定)时，周期增加500ms
	3. 按键1按下时，周期减少500ms
	4. 禁止使用HAL_Delay()
	5. 禁止使用中断和定时器
**三. 项目目的**
    实现LED0周期变换
***
## 引脚说明
|GPIO|名称|功能|
| - | - | - |
| PC2 | LED0 | LED0的亮灭 |
| PA0 | KEY0 | 控制LED0闪烁的周期增加 |
| PC13 | KEY1 | 控制LED0闪烁的周期减小 |
| PA13 | SYS_JIMS-SWDIO | 串行线调试 |
| PA14 | SYS_JIMS-SWCLK | 串行线调试 |
| PD0 | RCC_OSC_IN | 时钟配置 |
| PD1 | RCC_OSC_OUT | 时钟配置 |
***
## 关键逻辑介绍
```c
int delay_250 = 250; //定义延时时间
uint32_t pre = 0; //定义LED0的上一次闪烁时间
int KEY0LastState = 0; //定义KEY0的上一次状态
int KEY1LastState = 0; //定义KEY1的上一次状态
  while (1)
  {
	  uint32_t cur = HAL_GetTick();
	  int KEY0State = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin);
	  int KEY1State = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);	  
	  if(cur - pre >= delay_250 ) //判断延时
	  {
		  pre = cur; //更新状态
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
	  }
	  if(KEY0State != KEY0LastState) //判断KEY0的状态
      {
	      KEY0LastState = KEY0State; //更新状态
		  if(KEY0State == 0)
		  {
			  if(delay_250 < 1000) //限制最大值
			  {
			  delay_250 = delay_250 + 250;
			  }
		  }
      }
	  if(KEY1State != KEY1LastState) //判断KEY1的状态
      {
	      KEY1LastState = KEY1State; //更新状态
		  if(KEY1State == 0)
		  {
			  if(delay_250 > 250) //限制最小值
			  {
			  delay_250 = delay_250 - 250;
			  }
		  }
      }	    
  }   
```
---
CJH: 2025年9月13日 19:18
