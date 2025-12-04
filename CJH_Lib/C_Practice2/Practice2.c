#include <stdio.h>
#include <stdint.h>

int8_t find(int ccb[], int size,int flag)
{
    for(int i = 0;i < size;i++)
    {
        if(ccb[i] == flag)
        {
            return i;
        }
    }
    return -1;
}
int main()
{
    int ccb[] = {11,22,33,44,55};
    int size = 5;
    int flag = 44;
    int8_t found = find(ccb,size,flag);
    if(found != -1)
    {
        printf("Found at index: %d\n",found);
    }
    else
    {
        printf("Not Found\n");
    } 
}