#include <stdio.h>        // printf, perror
#include <fcntl.h>        // open
#include <unistd.h>       // read, close, unlink
#include <sys/stat.h>     // mkfifo

#define FIFOFILE "fifo"

int main(int argc, char **argv)
{
    int n, fd;
    char buf[BUFSIZ];

    // 기존 FIFO 파일 삭제 (중복 생성 방지)
    unlink(FIFOFILE);

    // FIFO 파일 생성 (0666: 읽기/쓰기 허용)
    if (mkfifo(FIFOFILE, 0666) < 0)
    {
        perror("mkfifo()");
        return -1;
    }

    // FIFO 파일을 읽기 전용으로 open (블로킹: 쓰는 쪽 연결될 때까지 대기)
    if ((fd = open(FIFOFILE, O_RDONLY)) < 0)
    {
        perror("open()");
        return -1;
    }

    // 표준 출력으로 FIFO 데이터 출력
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        fwrite(buf, 1, n, stdout);

    // 사용 종료
    close(fd);
    unlink(FIFOFILE);
    return 0;
}

