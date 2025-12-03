#include <stdio.h>
#include <stdint.h>

    int8_t fideTarget(int arr[],int size,int target)
    {
        for(int8_t i = 0;i < size;i++)
        {
            if(arr[i] == target)
            {
                return i;
            }
        }
        return -1;
    }
    int main()
    {
        int target=0;
        int arr[]={10,20,30,40,50};
        int size=sizeof(arr)/sizeof(arr[0]);
        printf("请输入要查找的目标值:\n");
        scanf("%d",&target);
        int8_t fideTarget(int arr[],int size,int target);
        int8_t result = fideTarget(arr,size,target);
        if(result !=-1 )
        {
            printf("找到目标值，索引为：%d\n",result);
        }
        else
        {
            printf("未找到目标值\n");
        }
        return 0;
    }

    // int main()
    // {
        // int a=0;
        // int arr[] = {1,2,3,4,5,6,7,8};
        // printf("请输入一个数:\n");
        // scanf("%d",&a);
        // int  size = sizeof(arr)/sizeof (arr[0]);
        // for(int i = 0;i<size;i++)
        // {
            // if(arr[i] == a)
            // {
                // printf("找到了，索引为：%d\n",i);
                // break;
            // }
            // else if(i == size-1)
            // {
                // printf("未找到\n");
            // }
        // }
    // }
