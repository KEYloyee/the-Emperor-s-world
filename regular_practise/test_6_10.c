#include <stdio.h>
int main()
{   
    int arr[10]={0};
    int* p =arr;
    for (int i = 0;i<=9;i++)
    {
        *p = i;
        p++;
        printf("%d\n",arr[i]);
    }
    return 0;
}