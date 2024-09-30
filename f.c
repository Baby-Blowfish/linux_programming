#include<stdio.h>
#include<unistd.h>

int main(void)
{
    int a;
    // close(0)  // std 0 close, not act scaf()   
    scanf("%d",&a);     // fd 0
    
    printf("hello");    // fd 1
    perror("error");    // fd 2
 
    return 0;
}
