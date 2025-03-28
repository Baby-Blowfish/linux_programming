#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("줄 버퍼링 테스트 (개행  없음):");
    sleep(2);

    printf("계속 출력중");
    sleep(2);

    printf("fflush 사용");
    fflush(stdout);
    sleep(2);

    printf("줄 버퍼링 테스트 (개행 있음): \n");

    return 0;
}
























