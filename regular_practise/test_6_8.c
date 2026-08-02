//实现牛顿切线法
#include <stdio.h>
#include <math.h>
#define eps 1e-6
int main()
{   
    double a,b,c,d;
    printf("请输入参数的值：\n");
    scanf("%lf %lf %lf %lf",&a,&b,&c,&d);
    double x0 = 1.0,x,f,fx;
    x=1;
    while (fabs(x-x0)>=eps)
    {   
        x = x0;
        f = ((a*x+b)*x+c)*x+d;
        fx = (3*x+2*b)*x+c;
        x=x0-f/fx;
    }
    printf("%lf",x);
    return 0;
}
