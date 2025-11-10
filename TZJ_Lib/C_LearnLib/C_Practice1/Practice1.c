#include <stdio.h>
#include <stdint.h>

uint16_t sum_for();
uint16_t sum_Formula(uint8_t sum);
uint16_t sum_while();
//

int main()
{
    uint16_t sum = 0;
    sum = sum_for();
    printf("forѭ������֮����%d\n",sum);
    sum = sum_Formula(100);
    printf("��ʽ����֮����%d\n",sum);
    sum = sum_while();
    printf("while����֮����%d\n",sum);
}

uint16_t sum_for()
{   printf("-----------------forѭ��ִ��-----------------\n");
    /* ***************������д����*********************************** */
    uint16_t i = 1;
    uint16_t sum1 = 0;
    for(i=1;i<=100;i++)
    {
        sum1+=i;
    }
    return sum1;
    /* ***************���ݽ�������*********************************** */

}

uint16_t sum_Formula(uint8_t num)
{
    printf("-----------------��ʽ��ִ��-----------------\n");
    /* ***************������д����*********************************** */
  
    uint16_t sum2=0;
    sum2=(num*(num+1))/2;
    return sum2;
    
    /* ***************���ݽ�������*********************************** */
}

uint16_t sum_while()
{
    printf("-----------------whileѭ��ִ��-----------------\n");
    /* ***************������д����*********************************** */
    uint16_t i = 1;
    uint16_t sum3 = 0;
    while(i<101)
    {
    sum3+=i;
    i++;
    }
    return sum3;
    
    /* ***************���ݽ�������*********************************** */
}