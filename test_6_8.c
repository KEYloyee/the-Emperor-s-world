//编写一个程序完成1到n的阶乘和
#include <stdio.h>
int jiecheng(int n)
{
    int i,chengji;
    chengji=1;
    for (i=1;i<=n;i++)
    {
        chengji*=i;
    }
    return chengji;
}
int main()
{
    int i,n;
    printf("请输入一个整数：\n");
    scanf("%d",&n);
    int sum = 0;
    for (i=1;i<=n;i++)
    {
        sum += jiecheng(i);
    }
    printf("从1到%d的阶乘和是%d",n,sum);
    return 0;
}