#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 실패");
        exit(1);
    }
    else if (pid == 0) {
        // 자식 프로세스: 사용자 정의 프로그램 실행
        printf("[Child] PID=%d, 사용자 프로그램 실행 시도\n", getpid());

        char *args[] = {"./my_child", NULL};  // 실행 파일 경로와 인자
        execvp(args[0], args);

        // exec 실패 시 여기 실행
        perror("[Child] exec 실패");
        exit(1);
    }
    else {
        // 부모 프로세스: 자식 대기
        printf("[Parent] 자식 PID=%d 실행됨, 대기 중...\n", pid);
        wait(NULL);
        printf("[Parent] 자식 종료 감지\n");
    }

    return 0;
}

