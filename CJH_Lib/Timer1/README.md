# Timer1：定时器中断
[toc]
***
## 项目介绍
 **一. 说明**
    实现了LED的闪烁

**二. 具体实现**

只允许使用定时器中断，且主函数除初始化以外不允许有任何语句和变量

**三. 项目目的**
       实现通过串口控制LED0的状态变换

***
## 引脚说明
|GPIO|名称|功能|
| - | - | - |
| PC2 | LED0 | LED0的亮灭 |
| PA13 | SYS_JIMS-SWDIO | 串行线调试 |
| PA14 | SYS_JIMS-SWCLK | 串行线调试 |
| PD0 | RCC_OSC_IN | 时钟配置 |
| PD1 | RCC_OSC_OUT | 时钟配置 |
***
## 关键逻辑介绍
```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //回调函数
	{
		if(htim == &htim2) //判断句柄
		{
			HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
		}
	} 
int main(void)
{
  HAL_TIM_Base_Start_IT(&htim2); //启用定时器中断
    while()
    {
    }
}
```
## 定时器配置流程
1. 将TIM2设为内部时钟
2. PSC为7199
3. ARR为2499
4. 开启预加载器
5. 在NVIC里启动中断
---
CJH: 2025年10月1日 21:42
