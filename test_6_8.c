//编写一个程序完成1到n的阶乘和
#include <stdio.h>
#include <stdio.h>
int main()
{
    int n;
    printf("请输入一个整数：\n");
    scanf("%d",&n);
    int i,sum;
    for (i=1;i<=n;i++)
    {
        int j,i_;
        for (j=1;j<=i;j++)
        {
            i_ *= j;
        }
        sum += i_;
    } 
    printf("从1到%d的阶乘和是%d",n,sum);
    return 0;
}
