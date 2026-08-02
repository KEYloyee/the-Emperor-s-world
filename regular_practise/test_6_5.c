#include <stdio.h>
int main()
{
    int a , b;
    printf("请输入两个整数：\n");
    scanf("%d %d",&a,&b);
    int shang = a / b;
    int yushu = a % b;
    printf("商是%d，余数是%d\n",shang,yushu);
    return 0;
}