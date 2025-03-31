#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // 자식 프로세스
        printf("Child (PID=%d, PPID=%d)\n", getpid(), getppid());

        // 자식 종료
        // exit(0);    // 표준 종료
        _exit(0);      // 즉시 종료 (버퍼 플러시 X)
    } else {
        // 부모 프로세스
        printf("Parent (PID=%d), child PID = %d\n", getpid(), pid);
        sleep(1); // 자식 종료 대기
    }
    return 0;
}

