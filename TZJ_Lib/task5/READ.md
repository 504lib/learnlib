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
``` 
 uint32_t last_tick = HAL_GetTick(); 
 while (1)
  {   
    
    if (HAL_GetTick() - last_tick >= 500) 
		{
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
			last_tick = HAL_GetTick();
		}
    
  }
  ```
#### 2.**实现LED1始终闪烁，周期为1000ms**
|引脚|功能|
|:-:|:-:|
|PA1|LED1|
```  
uint32_t last_tick = HAL_GetTick(); 
 while (1)
  {   
    
    if (HAL_GetTick() - last_tick >= 500) 
		{
			HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
			last_tick = HAL_GetTick();
		}
    
  }
  ```

  #### 3.**实现LED1始终闪烁，周期为1000ms,LED0始终闪烁，周期为500ms**
  |引脚|功能|
|:-:|:-:|
|PA1|LED1|
|PC13|LED0|
```
uint32_t last_tick_led0 = HAL_GetTick(); 
uint32_t last_tick_led1 = HAL_GetTick(); 
 while (1)
  {   
    
    if (HAL_GetTick() - last_tick_led0 >= 500) 
		{
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
			last_tick_led0 = HAL_GetTick();
		}
		   if (HAL_GetTick() - last_tick_led1 >= 1000) 
		{
			HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
			last_tick_led1 = HAL_GetTick();
		}
    
  }
  ```