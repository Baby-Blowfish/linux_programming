#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#define FIFOFILE "fifo"

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    int fd;
    char buf[1024];

    // 🔁 파일 존재할 때까지 루프
    while (access(FIFOFILE, F_OK) == -1) {
        printf("[Client] FIFO 파일이 아직 없음. 서버를 기다리는 중...\n");
        sleep(1);
    }

    // 🔁 읽는 쪽 있을 때까지 재시도 루프
    while (1) {
        fd = open(FIFOFILE, O_WRONLY | O_NONBLOCK);
        if (fd == -1) {
            if (errno == ENXIO) {
                printf("[Client] 서버가 아직 연결되지 않음. 재시도 중...\n");
                sleep(1);
                continue;
            } else {
                perror("open");
                exit(1);
            }
        }
        break;
    }

    printf("[Client] 연결됨. 메시지를 입력하세요 (Ctrl+D로 종료):\n");

    while (fgets(buf, sizeof(buf), stdin)) {
    ssize_t n = write(fd, buf, strlen(buf));
    if (n == -1) {
        if (errno == EPIPE) {
            fprintf(stderr, "[Client] 서버가 종료되어 write 실패 (EPIPE).\n");
            break;
        } else {
            perror("write");
        }
    }
}
    close(fd);
    return 0;
}

