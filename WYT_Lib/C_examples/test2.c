
// #include <stdio.h>
// int main()
// {
// 	char a = 176, b = 219;
// 	printf("%c%c%c%c%c\n", b, a, a, a, b);
// 	printf("%c%c%c%c%c\n", a, b, a, b, a);
// 	printf("%c%c%c%c%c\n", a, a, b, a, a);
// 	printf("%c%c%c%c%c\n", a, b, a, b, a);
// 	printf("%c%c%c%c%c\n", b, a, a, a, b);
// 	return 0;
// }

//8
// #include <stdio.h>
// int main()
// {
//     int i,j,result;
//     printf("\n");
//     for(i = 1; i <= 9; i++)
//    {
//     for(j = 1;j <=i; j++)
//     {
//        result = i*j;
//        printf("%d*%d=%-3d  ",i,j,result);
//     }
//     printf("\n");
//    }
//     return 0;
// }

//9
// #include <stdio.h>
// int main()
// {
    // for(int i =1;i <=8;i++)
    // {
        // for(int j =1;j<=8;j++)
        // {
        // if((i+j)%2 == 0)
        // printf("%c%c",219,219);
        // else printf("  ");
        // printf("\n");
        // }
    // }
    // return 0;
// }

//10
// #include <stdio.h>
// int main()
// {
    // int a = 1,b = 1,temp,i;
    // printf("%12d%12d",a,b);
// 
    // for(i = 3;i <= 20;i++)
    // {
        // temp = a + b;
        // printf("%12d",temp);
        // a = b;
        // b = temp;
        // temp = a+b;
        // printf("%12d\n",temp);
        // a = b;
        // b = temp;
    // }
    // return 0;
//}


//°´Î»Óë
// #include <stdio.h>
// int main()
// {
    // int aa= 1112;
    // int aa1= 1111;
    // int res = aa & 1;
    // int res1 = aa1 & 1;
    // printf("res = %d,res = %d\n",res,res1);
    // return 0;
// }


#include <stdio.h>
int main()
{
    int a=7;
    int shift1 = a << 1;
    int shift2 = a >> 1;
    printf("shift1 = %d,shift2 = %d\n",shift1,shift2);
    return 0;
}