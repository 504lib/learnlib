# UART3：你妈的点灯时间到
[toc]
***
## 项目介绍
 **一. 说明**
    实现了通过串口控制LED0的三种状态切换
**二. 具体实现**

1. 默认状态下LED0闪烁，周期自定

2. 当上位机对单片机通过串口发送1时(ASCII字符)，LED0立刻停止闪烁状态，并持续保持亮的状态

3. 当上位机对单片机通过串口发送2时(ASCII字符)，LED0立刻停止闪烁状态，并持续保持灭的状态

4. 禁止使用HAL_Delay(),且要求立刻响应(虽然可能是堵塞访问不会立刻响应，但是为了程序流畅，不允许太慢)

5. 禁用串口中断，但定时器及其中断可用

**三. 项目目的**
       实现通过串口控制LED0的状态变换

***
## 引脚说明
|GPIO|名称|功能|
| - | - | - |
| PC2 | LED0 | LED0的亮灭 |
| PA10 | USART1_RX | 串行数据的接收 |
| PA9 | USART1_TX | 串行数据的发送 |
| PA13 | SYS_JIMS-SWDIO | 串行线调试 |
| PA14 | SYS_JIMS-SWCLK | 串行线调试 |
| PD0 | RCC_OSC_IN | 时钟配置 |
| PD1 | RCC_OSC_OUT | 时钟配置 |
***
## 关键逻辑介绍
```c
#define delay 500 //定义延时时间
uint32_t last = 0;//定义上一次的时间状态
uint8_t data = '0';//定义要接收的数据名
  while (1)
  {
	  uint32_t cur = HAL_GetTick();//定义系统捕获时间为cur
	  	  HAL_UART_Receive(&huart1, &data, 1,500);//串口接收数据
	  if(data == '1') 
	  {
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET); //保持常亮
	  }
	  else if(data == '2') 
	  {
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET); //保持常灭
	  }
	  else if(data == '0')
	  {	
			if(cur - last >= delay)
			{
				last = cur;
				HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
			}
	  }
  }  
```
---
CJH: 2025年9月27日 14:40
