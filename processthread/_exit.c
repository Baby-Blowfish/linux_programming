#include<stdio.h>
#include <unistd.h>
int main(int argc, char **argv)
{
    printf("hello world");
    //fflush(stdout);
    _exit(0);

}
