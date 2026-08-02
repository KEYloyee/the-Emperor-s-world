#include <stdio.h>

int main(void) {
    unsigned long long n = 10;
    unsigned long long factorial = 1;

    for (unsigned long long i = 1; i <= n; ++i) {
        factorial *= i;
    }

    printf("10的阶乘是: %llu\n", factorial);
    return 0;
}
