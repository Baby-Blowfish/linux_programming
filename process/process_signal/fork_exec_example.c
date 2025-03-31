#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 실패");
        return 1;
    } else if (pid == 0) {
        // 자식 프로세스
        printf("[Child] PID=%d. ls 실행\n", getpid());

        // execvp: 현재 환경에서 "ls -l -a" 실행
        char *args[] = {"ls", "-l", "-a", NULL};
        execvp("ls", args);  // 성공 시 여기 아래는 실행되지 않음

        // exec 실패 시
        perror("execvp 실패");
        exit(1);
    } else {
        // 부모 프로세스
        printf("[Parent] PID=%d. 자식 PID=%d 대기 중...\n", getpid(), pid);
        wait(NULL);  // 자식 종료 대기
        printf("[Parent] 자식 종료됨. 부모도 종료.\n");
    }

    return 0;
}

