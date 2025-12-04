#include <stdio.h>
#include <stdint.h>
int8_t findTarget(int arr[],int size,int target)
{
    for(int8_t i = 0;i<size;i++)
    {
        if(arr[i]==target)
        {
            return i;
        }
    }
    return -1;
}
int main()
{
    int arr[]={1,2,3,4,5,6,7,8,9,10};
    int target = 0;
    int size = sizeof(arr)/sizeof(arr[0]);
    printf("请输入要查找的目标值：");
    scanf("%d",&target);
    int8_t fideTarget(int arr[],int size,int target);
    int8_t A = findTarget(arr,size,target);
    if(A !=-1)
    {
        printf("找到目标值，索引为：%d\n",A);

    }
    else{
        printf("未找到目标值\n");
    }
    return 0;
}