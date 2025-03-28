#include <stdio.h>
#include <unistd.h>

int main()
{
    char buffer[BUFSIZ];


    setvbuf(stdout, buffer, _IOFBF, BUFSIZ);
    printf("[완전버퍼링]");
    sleep(2);
    fflush(stdout);
    printf("flush 후 출력됨\n");

    setvbuf(stdout, buffer, _IOLBF, BUFSIZ);
    printf("[줄 단위 버퍼링]\n");
    sleep(2);

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[무 버퍼링]\n");
    sleep(2);


    return 0;
}
