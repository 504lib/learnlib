#include <stdio.h>
#include <stdint.h>
uint16_t sum1To100()
    {
        uint16_t sum = 0;
        for(int i=0;i<=100;i++)
        {
            sum +=i;
        }
        return sum;
    }
    
int main()
{
    uint16_t result = sum1To100();
    printf("1+2+3+4+...+100=%u\n",result);
    return 0;
}