#include <stdio.h>
struct student
{
    char name[10];
    int age ;
    char tele[20];
    
};
int main ()
{
    struct student s1 = {"Mike",10,"19862202987"};
    printf("%s %d %s",s1.name,s1.age,s1.tele);
    return 0;   
}