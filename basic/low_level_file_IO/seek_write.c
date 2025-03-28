#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    char buf[20] = {0};
    off_t offset;
    int num = 3;

    int fd = open("seekfile.txt",O_RDWR|O_CREAT,0644);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }


    write(fd,"ABCDEFGH12345678", 16);

     // 2. SEEK_SET: 파일 시작 기준으로 4바이트 이동
    offset = lseek(fd, 4, SEEK_SET);
    printf("[SEEK_SET] offset: %ld\n", offset);
    write(fd, "X", 1);  // → A B C D X F G H ...
    write(fd,"987", 2);  // → A B C D X F G H ...

    // 3. SEEK_CUR: 현재 위치에서 5바이트 앞으로 이동
    offset = lseek(fd, 5, SEEK_CUR);
    printf("[SEEK_CUR] offset: %ld\n", offset);
    write(fd, "Y", 1);  // → ... Y ...

    // 4. SEEK_END: 파일 끝에서 -3바이트 뒤로 이동
    offset = lseek(fd, -3, SEEK_END);
    printf("[SEEK_END] offset: %ld\n", offset);
    write(fd, "Z", 1);  // → 마지막에서 3바이트 전 위치에 Z 삽입

    // 5. 전체 내용 읽기 확인
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, 16);
    buf[16] = '\0';
    printf("[파일 최종 내용] %s\n", buf);

    close(fd);
    return 0;
}


