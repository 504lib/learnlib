# UART3������ĵ��ʱ�䵽
[toc]
***
## ��Ŀ����
 **һ. ˵��**
    ʵ����ͨ�����ڿ���LED0������״̬�л�
**��. ����ʵ��**

1. Ĭ��״̬��LED0��˸�������Զ�

2. ����λ���Ե�Ƭ��ͨ�����ڷ���1ʱ(ASCII�ַ�)��LED0����ֹͣ��˸״̬����������������״̬

3. ����λ���Ե�Ƭ��ͨ�����ڷ���2ʱ(ASCII�ַ�)��LED0����ֹͣ��˸״̬���������������״̬

4. ��ֹʹ��HAL_Delay(),��Ҫ��������Ӧ(��Ȼ�����Ƕ������ʲ���������Ӧ������Ϊ�˳���������������̫��)

5. ���ô����жϣ�����ʱ�������жϿ���

**��. ��ĿĿ��**
       ʵ��ͨ�����ڿ���LED0��״̬�任

***
## ����˵��
|GPIO|����|����|
| - | - | - |
| PC2 | LED0 | LED0������ |
| PA10 | USART1_RX | �������ݵĽ��� |
| PA9 | USART1_TX | �������ݵķ��� |
| PA13 | SYS_JIMS-SWDIO | �����ߵ��� |
| PA14 | SYS_JIMS-SWCLK | �����ߵ��� |
| PD0 | RCC_OSC_IN | ʱ������ |
| PD1 | RCC_OSC_OUT | ʱ������ |
***
## �ؼ��߼�����
```c
#define delay 500 //������ʱʱ��
uint32_t last = 0;//������һ�ε�ʱ��״̬
uint8_t data = '0';//����Ҫ���յ�������
  while (1)
  {
	  uint32_t cur = HAL_GetTick();//����ϵͳ����ʱ��Ϊcur
	  	  HAL_UART_Receive(&huart1, &data, 1,500);//���ڽ�������
	  if(data == '1') 
	  {
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET); //���ֳ���
	  }
	  else if(data == '2') 
	  {
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET); //���ֳ���
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
CJH: 2025��9��27�� 14:40
