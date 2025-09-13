# ����������ֵ�仯
[toc]
***
## ��Ŀ����
 **һ. ˵��**
    ʵ����LED0�Ķ���������˸
**��. ����ʵ��**
    1. LED0��ʼ��500msΪ����ִ����˸
	2. ����0����(�������������´������ִ����Զ�)ʱ����������500ms
	3. ����1����ʱ�����ڼ���500ms
	4. ��ֹʹ��HAL_Delay()
	5. ��ֹʹ���жϺͶ�ʱ��
**��. ��ĿĿ��**
    ʵ��LED0���ڱ任
***
## ����˵��
|GPIO|����|����|
| - | - | - |
| PC2 | LED0 | LED0������ |
| PA0 | KEY0 | ����LED0��˸���������� |
| PC13 | KEY1 | ����LED0��˸�����ڼ�С |
| PA13 | SYS_JIMS-SWDIO | �����ߵ��� |
| PA14 | SYS_JIMS-SWCLK | �����ߵ��� |
| PD0 | RCC_OSC_IN | ʱ������ |
| PD1 | RCC_OSC_OUT | ʱ������ |
***
## �ؼ��߼�����
```c
int delay_250 = 250; //������ʱʱ��
uint32_t pre = 0; //����LED0����һ����˸ʱ��
int KEY0LastState = 0; //����KEY0����һ��״̬
int KEY1LastState = 0; //����KEY1����һ��״̬
  while (1)
  {
	  uint32_t cur = HAL_GetTick();
	  int KEY0State = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin);
	  int KEY1State = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);	  
	  if(cur - pre >= delay_250 ) //�ж���ʱ
	  {
		  pre = cur; //����״̬
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
	  }
	  if(KEY0State != KEY0LastState) //�ж�KEY0��״̬
      {
	      KEY0LastState = KEY0State; //����״̬
		  if(KEY0State == 0)
		  {
			  if(delay_250 < 1000) //�������ֵ
			  {
			  delay_250 = delay_250 + 250;
			  }
		  }
      }
	  if(KEY1State != KEY1LastState) //�ж�KEY1��״̬
      {
	      KEY1LastState = KEY1State; //����״̬
		  if(KEY1State == 0)
		  {
			  if(delay_250 > 250) //������Сֵ
			  {
			  delay_250 = delay_250 - 250;
			  }
		  }
      }	    
  }   
```
---
CJH: 2025��9��13�� 19:18
