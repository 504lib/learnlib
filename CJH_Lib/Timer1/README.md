# Timer1����ʱ���ж�
[toc]
***
## ��Ŀ����
 **һ. ˵��**
    ʵ����LED����˸

**��. ����ʵ��**

ֻ����ʹ�ö�ʱ���жϣ�������������ʼ�����ⲻ�������κ����ͱ���

**��. ��ĿĿ��**
       ʵ��ͨ�����ڿ���LED0��״̬�任

***
## ����˵��
|GPIO|����|����|
| - | - | - |
| PC2 | LED0 | LED0������ |
| PA13 | SYS_JIMS-SWDIO | �����ߵ��� |
| PA14 | SYS_JIMS-SWCLK | �����ߵ��� |
| PD0 | RCC_OSC_IN | ʱ������ |
| PD1 | RCC_OSC_OUT | ʱ������ |
***
## �ؼ��߼�����
```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //�ص�����
	{
		if(htim == &htim2) //�жϾ��
		{
			HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
		}
	} 
int main(void)
{
  HAL_TIM_Base_Start_IT(&htim2); //���ö�ʱ���ж�
    while()
    {
    }
}
```
## ��ʱ����������
1. ��TIM2��Ϊ�ڲ�ʱ��
2. PSCΪ7199
3. ARRΪ2499
4. ����Ԥ������
5. ��NVIC�������ж�
---
CJH: 2025��10��1�� 21:42
