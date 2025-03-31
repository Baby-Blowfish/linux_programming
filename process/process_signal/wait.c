#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // 자식 프로세스
        printf("자식 프로세스 실행 중 (PID=%d)\n", getpid());
        sleep(2);
        printf("자식 종료\n");
        exit(42);  // 종료 코드 42로 종료
    } else {
        // 부모 프로세스
        int status;
        pid_t child_pid = wait(&status);  // 자식 종료 대기
        printf("부모 프로세스: 자식 PID %d 종료 감지\n", child_pid);

        if (WIFEXITED(status)) {
            printf("자식의 종료 코드: %d\n", WEXITSTATUS(status));
        }
    }

    return 0;
}

