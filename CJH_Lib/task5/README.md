# ����������·���
[toc]
***
## ��Ŀ����
 **һ. ˵��**
    ʵ����LED0��LED1����˸
**��. ����ʵ��**
    1. LED0��500msΪ����ִ����˸
    2. LED1��1000msΪ����ִ����˸
    3. ����LED����˸������ȫ����������Ӱ��
    4. ��ֹʹ��?HAL_Delay()
    5. ��ֹʹ���ж�/��ʱ��
**��. ��ĿĿ��**
    ��ϰ�Ƕ���ʽ��ʱ
***
## ����˵��
|GPIO|����|����|
| - | - | - |
| PC2 | LED0 | LED0������ |
| PC3 | LED1 | LED1������ |
| PA13 | SYS_JIMS-SWDIO | �����ߵ��� |
| PA14 | SYS_JIMS-SWCLK | �����ߵ��� |
| PD0 | RCC_OSC_IN | ʱ������ |
| PD1 | RCC_OSC_OUT | ʱ������ |
***
## �ؼ��߼�����
```c
#define big_delay 500
#define small_delay 250
uint32_t pre = 0; //LED0��һ�ε���ʱʱ��
uint32_t pre1 = 0; //LED1��һ�ε���ʱʱ��
  while (1)
  {
	  uint32_t cur = HAL_GetTick();
	  if(cur - pre >= big_delay) //���ڵ���500ms��ִ��
	  {
		  pre = cur; //����LED0��ʱ��
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin); //��ת״̬
	  }
	  	  if(cur - pre1 >= small_delay) //���ڵ���250ms��ִ��
	  {
		  pre1 = cur; //����LED1��ʱ��
		  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); //��ת״̬
	  }
  }
```
---
CJH: 2025��9��13�� 15:23
