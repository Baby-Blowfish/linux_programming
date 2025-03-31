#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

int main() {
    int p2c[2], c2p[2]; // [0] = read, [1] = write
    pid_t pid;
    char buf[BUF_SIZE];

    // 파이프 생성: 부모 → 자식
    if (pipe(p2c) == -1) {
        perror("pipe p2c");
        exit(1);
    }

    // 파이프 생성: 자식 → 부모
    if (pipe(c2p) == -1) {
        perror("pipe c2p");
        exit(1);
    }

    // fork로 자식 생성
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }

    // 🔸 자식 프로세스
    if (pid == 0) {
        close(p2c[1]);  // 자식은 부모로부터 읽기만
        close(c2p[0]);  // 자식은 부모에게 쓰기만

        while (1) {
            // 1. 부모로부터 명령 읽기
            int n = read(p2c[0], buf, BUF_SIZE);
            if (n <= 0) break;

            buf[n] = '\0'; // 문자열 종료
            printf("[Child] 받은 명령: %s", buf);

            // 2. popen으로 명령 실행하고 결과 읽기
            FILE *fp = popen(buf, "r");
            if (!fp) {
                char *err = "명령 실행 실패\n";
                write(c2p[1], err, strlen(err));
                continue;
            }

            // 3. 명령 실행 결과 → 부모에게 전송
            while (fgets(buf, BUF_SIZE, fp)) {
                write(c2p[1], buf, strlen(buf));
            }

            pclose(fp);
        }

        close(p2c[0]);
        close(c2p[1]);
        _exit(0);
    }

    // 🔸 부모 프로세스
    else {
        close(p2c[0]);  // 부모는 자식에게 쓰기만
        close(c2p[1]);  // 부모는 자식으로부터 읽기만

        while (1) {
            printf("\n[Parent] 명령 입력 (종료: exit): ");
            fflush(stdout);

            // 사용자 입력
            if (!fgets(buf, BUF_SIZE, stdin)) break;

            // 종료 명령
            if (strncmp(buf, "exit", 4) == 0) break;

            // 자식에게 명령어 전송
            write(p2c[1], buf, strlen(buf));

            // 자식이 보낸 실행 결과 수신
            printf("[Parent] 자식 응답:\n");
            int n;
            while ((n = read(c2p[0], buf, BUF_SIZE)) > 0) {
                write(STDOUT_FILENO, buf, n);
                // break 조건: 현재는 전체 읽기 → timeout 처리 필요시 수정
                if (n < BUF_SIZE) break;
            }
        }

        // 파이프 닫고 자식 종료 대기
        close(p2c[1]);
        close(c2p[0]);
        wait(NULL);
        printf("[Parent] 종료.\n");
    }

    return 0;
}

