# ����������·���
[toc]
***
## ��Ŀ����
 **һ. ˵��**
    ʵ����LED0����˸��KEY0����LED1�������һ���Ӱ��
**��. ����ʵ��**
    1. ʵ��LED0ʼ����˸������Ϊ500ms
    2. ������������(�������������ִ������´����Զ�)������LED1����(toggle)���Ҳ�����LED0��˸��ִ��
    3. ������Ӧ�ٶ���100~200ms����(��������Ӧ)�����°������̸ı�LED1��״̬
    4. ����Ϊ"task3"�����ҳɹ����PR��ͨ��
    5. **��ֹ**ʹ���жϣ���ʱ��������
**��. ��ĿĿ��**
    ��ϰ�Ƕ���ʽ��ʱ��״̬�л�
***
## ����˵��
|GPIO|����|����|
| - | - | - |
| PC2 | LED0 | LED0������ |
| PC3 | LED1 | LED1������ |
| PA0 | KEY0 | ����LED1������ |
| PA13 | SYS_JIMS-SWDIO | �����ߵ��� |
| PA14 | SYS_JIMS-SWCLK | �����ߵ��� |
| PD0 | RCC_OSC_IN | ʱ������ |
| PD1 | RCC_OSC_OUT | ʱ������ |
***
## �߼�����
```c
#define delay 500
uint32_t last = 0; //��һ�ε���ʱʱ��
int pree = 0; //��һ�εİ���״̬
int now = 0; //���ڵİ���״̬
  while (1)
  {
	  uint32_t cur = HAL_GetTick(); //��ȡ����ʱ�亯������Ϊcur
	  if(cur - last >= delay) //�ж��Ƿ�ﵽ500ms
	  {
		  last = cur; //����״̬
		  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
	  }
	  now = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) ;//����KEY0��״̬Ϊnow
	  if(now != pree) //�ж�״̬�Ƿ����
	  {
		  pree = now; //����״̬
		  if(now == 0) //������ڵ͵�ƽ
		  {
			  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); //��ת״̬
		  }
	  }
  }
```
---
CJH: 2025��9��12�� 19:41