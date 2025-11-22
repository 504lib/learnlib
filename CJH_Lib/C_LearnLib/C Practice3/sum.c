#include<stdio.h>
#include<stdint.h>
unsigned int sum_array(unsigned int* arr, unsigned int size);
unsigned int sum_range(unsigned int i);
int main()
{
    unsigned int size = 0;
    unsigned int i = 0;
    unsigned int sum = sum_range(i);
    printf("1 + 2 + 3 + ... +98 + 99 + 100 = %d\n",sum);
    printf("please input a number (100~500):");
    scanf("%d", &size);
    unsigned int arr[size];
    unsigned int all = sum_array(arr,size);
    printf("the 1 ~ %d sum is %d\n", size,all); 
    return 0;
}
unsigned int sum_range(unsigned int i)
{
    unsigned int sum = 0;
    for (i = 1; i <= 100; i++)
    {
        sum += i;
    }
    return sum;
}
unsigned int sum_array(unsigned int* arr, unsigned int size)
{
    
    for(unsigned int j = 0; j < size; j++)
    {
        arr[j] = j + 1;
    }
    unsigned int all = 0;
    for (unsigned int k = 0; k < size; k++)
    {
        all += arr[k];
    }
    return all;
}