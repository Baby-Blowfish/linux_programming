#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // 자식 프로세스
        printf("자식 실행 중 (PID=%d)\n", getpid());
        sleep(3);
        exit(7);  // 자식 종료 코드
    } else {
        // 부모 프로세스
        int status;
        pid_t result;

        do {
            result = waitpid(pid, &status, WNOHANG);  // 비블로킹 대기
            if (result == 0) {
                printf("자식 아직 종료 안 됨. 기다리는 중...\n");
                sleep(1);
            }
        } while (result == 0);

        printf("자식 종료됨. PID: %d\n", result);
        if (WIFEXITED(status)) {
            printf("자식 종료 코드: %d\n", WEXITSTATUS(status));
        }
    }

    return 0;
}

