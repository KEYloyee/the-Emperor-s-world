#include <stdio.h>
#include <math.h>
int isprime(int m)
{
  if (m <=1) return 0;
  else if (m == 2) return 1;
  else 
  {
    for (int i = 2;i*i<=m;i++ )
    {
        if (m % i ==0) return 0;
    }
    return 1;
  }

}
int main ()    
{   
    int n;
    printf ("请输入一个自然数：\n");
    scanf("%d",&n);
    int a = isprime(n);
    printf("%d",a);
    return 0;
}
//0表示非素数，1表示素数