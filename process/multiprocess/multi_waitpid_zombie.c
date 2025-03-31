#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_CHILDREN 3

int main() {
    pid_t pids[NUM_CHILDREN];  // 자식 PID 저장용
    int i;

    // 1. 자식 프로세스 생성
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // 자식 프로세스
            printf("[Child %d] PID = %d, Parent = %d\n", i, getpid(), getppid());
            sleep(2 + i);  // 각 자식은 서로 다른 시간 후 종료
            printf("[Child %d] 종료\n", i);
            exit(10 + i);  // 각각 다른 종료 코드
        } else if (pid > 0) {
            pids[i] = pid;  // 부모가 자식 PID 저장
        } else {
            perror("fork");
            exit(1);
        }
    }

    // 2. 부모가 자식 종료 감지 및 처리
    int status;
    pid_t terminated_pid;
    int children_left = NUM_CHILDREN;

    while (children_left > 0) {
        // 종료된 자식 중 하나를 기다림
        terminated_pid = waitpid(-1, &status, 0);

        if (terminated_pid > 0) {
            printf("[Parent] 자식 PID %d 종료 감지\n", terminated_pid);

            if (WIFEXITED(status)) {
                printf("[Parent] 자식의 종료 코드: %d\n", WEXITSTATUS(status));
            }

            children_left--;
        }
    }

    printf("[Parent] 모든 자식 종료 완료. 프로그램 종료.\n");
    return 0;
}

