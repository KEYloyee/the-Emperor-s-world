#include <stdio.h>
#include <math.h>
int main()
{
    if(fabs(1.0/3.0*3.0-1.0)<1e-6)
    {
        printf("Yes\n");
    }
    else
    {
        printf("No\n");
    }
    return 0;
}