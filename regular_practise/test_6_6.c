#include <stdio.h>
#include <math.h>
int main()
{
    printf("请输入方程参数：\n");
    float a,b,c,x1,x2;
    scanf("%f %f %f",&a,&b,&c);
    if (b*b - 4*a*c < 0) 
    {
        printf("方程无实数根\n");
        return 0;
    }
    x1 = (-b + sqrt(b*b - 4*a*c)) / (2*a);
    x2 = (-b - sqrt(b*b - 4*a*c)) / (2*a);
    printf("方程的两个根是：%f 和 %f\n",x1,x2);
    printf("判别式的绝对值是：%.4f\n", fabs(b*b - 4*a*c));
    return 0;
}