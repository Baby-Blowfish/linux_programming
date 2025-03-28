#include <stdio.h>

void funcC(int z) {
    int x = 10;
    printf("In funcC: z = %d, x = %d\n", z, x);
}

void funcB(int y) {
    funcC(y + 1);
}

void funcA() {
    int n = 5;
    funcB(n);
}

int main() {
    funcA();
    return 0;
}

