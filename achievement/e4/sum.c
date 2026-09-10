#include <stdio.h>

int main() {
    long long sum = 0;
    long long n = 1000000000;  
    for (long long i = 1; i <= n; i++) {
        sum += i;
    }
    printf("sum = %lld\n", sum);
    return 0;
}