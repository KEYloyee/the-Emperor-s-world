#include <stdio.h>
struct student
{
    char name[10];
    int age ;
    char tele[20];
};
void print(struct student* ps)
{
    printf("%s %d %s",ps->name,ps->age,ps->tele);
}
int main ()
{
    struct student s1 = {"Mike",10,"19862202987"};
    print(&s1);
    return 0;   
}