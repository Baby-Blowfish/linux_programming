// my_child.c - 자식 프로세스 역할을 할 사용자 정의 프로그램
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("[my_child] 사용자 정의 프로그램 실행 중 (PID=%d)\n", getpid());
    sleep(1);
    printf("[my_child] 작업 완료 후 종료\n");
    return 0;
}

