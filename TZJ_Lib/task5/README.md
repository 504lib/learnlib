# 任务三：旁路设计
## 陶正杰
**目录**
[toc]

***
###  任务三：旁路设计
#### 1.**实现LED0始终闪烁，周期为500ms**
|引脚|功能|
|:-:|:-:|
|PC13|LED0|
```  uint32_t last_tick = HAL_GetTick(); 
 while (1)
  {   
    
    if (HAL_GetTick() - last_tick >= 200) 
		{
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
			last_tick = HAL_GetTick();
		}
    
  }
  ```
#### 2.**按键驱动LED1灯**
|引脚|功能|
|:-:|:-:|
|PA1|LED1|
|PA0|按钮输入|




```
        uint8_t pre = 1,cur = 1;
        uint8_t ledState = 0;	

    while (1)
  {
		pre = cur;
		if(HAL_GPIO_ReadPin(IN_GPIO_Port, IN_Pin)==GPIO_PIN_SET)
		{
			cur = 1;
		}
		else
		{
		cur = 0;
		}
		if(pre!=cur)
		{  HAL_Delay(10);
			if(cur == 0){}
				else
				{
					if(ledState == 0)
					{HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
						ledState = 1;
					}
					else
					{HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
						ledState = 0;
					}
				}
				
		}
  
  }
  ```
#### 3.**实现LED0始终闪烁，周期为500ms,按键驱动LED1灯**
|引脚|功能|
|:-:|:-:|
|PA1|LED1|
|PC13|LED0|
|PA0|按钮输入|
 ```  
    uint8_t pre = 1,cur = 1;
	uint8_t ledState = 0;	
	uint32_t last_tick = HAL_GetTick();
	
	while (1)
  {
		    if (HAL_GetTick() - last_tick >= 500) 
		{
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
			last_tick = HAL_GetTick();
		}
				pre = cur;
		if(HAL_GPIO_ReadPin(IN_GPIO_Port, IN_Pin)==GPIO_PIN_SET)
		{
			cur = 1;
		}
		else
		{
		cur = 0;
		}
		if(pre!=cur)
		{  HAL_Delay(10);
			if(cur == 0){}
				else
				{
					if(ledState == 0)
					{HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
						ledState = 1;
					}
					else
					{HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
						ledState = 0;
					}
				}
				
		}

  }
```