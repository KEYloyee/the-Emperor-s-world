#include <stdio.h>
    int fee(int x)//计算出租车分段计费的函数
    {
        if (x<3)
        {
            return 10;
        }
        else if (x>=3 && x<=8)
        {
            int y =10+(int)(x-3+1)*2.0;
            return y;
        }
        else
        {
            int z = 10+5*2.0+(int)(x-8+1)*2.4;
            return z;
        }
    }
    int main ()
    {
        int d ;
        printf("请输入出租车行驶的距离（单位：公里）：\n");
        scanf("%d",&d); 
        int fee_d=fee(d);
        printf("出租车行驶%d公里的费用是%.2f元\n",d,(float)fee_d);
        return 0;
    }
