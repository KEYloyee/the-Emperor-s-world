#include <stdio.h>





int main ()
{
    for (int i ;i<=10;i++)
    {
        printf("%d\n",i);
        int* p = &i;
        printf("%p\n",p);
    }
    const char* s = "wode";
    printf("%s", s);

    return 0;
}