
# UART3::你妈的点灯时间到 



## 陶正杰
**目录**
[toc]

***
###  UART3::你妈的点灯时间到 
#### 1.**默认状态下LED0闪烁，周期自定**
|引脚|功能|
|:-:|:-:|
|GPIOC13|闪灯|
```
  while (1)
  {
    		uint16_t cur = HAL_GetTick();
		if(cur - per >= 100 && dataRcvd == NULL )
		{
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
		per = cur;	
		}
  }
```
#### 2.**当上位机对单片机通过串口发送1时(ASCII字符)，LED0立刻停止闪烁状态，并持续保持亮的状态**
|引脚|功能|
|:-:|:-:|
|GPIOC13|闪灯|
|GPIOA10|接收数据|
|GPIOA9|传输数据|
```
uint8_t dataRcvd = NULL;
  while (1)
{
    		HAL_UART_Receive(&huart1,&dataRcvd,1,10);
		if(dataRcvd == '2')
		{
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_SET);
			
		}
		 else if(dataRcvd == '1')
		{
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
		}
}
```
### 3.**默认状态下LED0闪烁，周期自定，当上位机对单片机通过串口发送1时(ASCII字符)，LED0立刻停止闪烁状态，并持续保持亮的状态，当上位机对单片机通过串口发送2时(ASCII字符)，LED0立刻停止闪烁状态，并持续保持灭的状态**
|引脚|功能|
|:-:|:-:|
|GPIOC13|闪灯|
|GPIOA10|接收数据|
|GPIOA9|传输数据|

```
uint8_t dataRcvd = NULL;
uint16_t per = 0;

  while (1)
  {
    		uint16_t cur = HAL_GetTick();
		if(cur - per >= 100 && dataRcvd == NULL )
		{
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
		per = cur;	
		}
		HAL_UART_Receive(&huart1,&dataRcvd,1,10);
		if(dataRcvd == '2')
		{
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_SET);
			
		}
		 else if(dataRcvd == '1')
		{
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
		}

  }
```
