#include <stdio.h>

int add(int a, int b)
{
    int result = a +  b;
    return result;
}

int main(void)
{
    int x  = 3,  y = 5 , z =  0;
    z =  add(x,y);
    printf("%d\n",z);

    return 0;
}


