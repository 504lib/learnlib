#include <stdio.h>
#include <stdint.h>

uint16_t sum_for();
uint16_t sum_Formula(uint8_t sum);
uint16_t sum_while();

int main()
{
    uint16_t sum = 0;
    sum = sum_for();
    printf("for循环百数之和是%d\n",sum);
    sum = sum_Formula(100);
    printf("公式百数之和是%d\n",sum);
    sum = sum_while();
    printf("while百数之和是%d\n",sum);
}

uint16_t sum_for()
{
    printf("-----------------for循环执行-----------------\n");
    /* ***************内容填写区域*********************************** */
    uint16_t i = 1;
    uint16_t j = 0; 
    for(i=1; i<=100; i++)
    {
        j += i;
    }
    return j;
    /* ***************内容结束区域*********************************** */
}

uint16_t sum_Formula(uint8_t num)
{
    printf("-----------------公式法执行-----------------\n");
    /* ***************内容填写区域*********************************** */
    return num+num*(num-1)/2;
    /* ***************内容结束区域*********************************** */
}

uint16_t sum_while()
{
    printf("-----------------while循环执行-----------------\n");
    /* ***************内容填写区域*********************************** */
    uint8_t i = 1;
    uint16_t j = 0;
    while (i <= 100)
    {
        j += i;
        i++;
    }
    return j;
    /* ***************内容结束区域*********************************** */
}