//1
// #include <stdio.h>
// int main()
// {
//     int i,j,k;
//     printf("All the three digit number formed by 1,2,3,4 are:\n");
//     for(i = 1; i < 5; i++)
//     {
//         for(j=1;j<5;j++)
//         {
//             for(k=1;k<5;k++)
//             if(i !=k&&i !=k&&j !=k)
//             {
//                 printf("%d%d%d ",i,j,k);
//             }
//         }
         
//     }   
    
//     return 0;
// }

//3
//x+100=a^2
//x+100+168=b^2
//b>a
//b^2 - a^2=168
//(b-a)(b+a)=168
//璁緈 = b-a;n = b+a
//鍒檓n=168
//n > m
//b=(m+n)/2
//a=(n-m)/2


//5
#include <stdio.h>
int main()
{
    int x,y,z,t;
    printf("请输入三个数:\n");
    scanf("%d %d %d",&x,&y,&z);
    if(x > y)
    {
        t = x;x = y; y = t;
    }
    if(x > z)
    {
        t = x;x = z;z = t;
    }
    if(y > z)
    {
        t = y;y = z;z = t;
    }
    printf("从大到小的顺序是：%d %d %d \n",x,y,z);
}