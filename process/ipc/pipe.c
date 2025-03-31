#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid;
    int pfd[2];            // 파이프 디스크립터 [0]: read, [1]: write
    char line[BUFSIZ];     // 부모가 자식으로부터 읽을 데이터 버퍼
    int status;
    ssize_t n;

    // 파이프 생성 (익명 파이프)
    if(pipe(pfd) < 0)
    {
        perror("pipe()");
        return -1;
    }

    // 자식 프로세스 생성
    if((pid = fork()) < 0)
    {
        perror("fork()");
        return -1;
    }

    // 🔸 자식 프로세스
    else if(pid == 0)
    {
        printf("brother pfd0 = %d, pfd1 = %d\n", pfd[0], pfd[1]);

        close(pfd[0]);  // 자식은 읽기 안 함 → 읽기 디스크립터 닫기

        // stdout(1)을 파이프의 write 쪽으로 리디렉션
        dup2(pfd[1], 1);

        // execl로 date 명령 실행 → stdout이 파이프를 통해 부모로 전송됨
        execl("/bin/date", "date", NULL);

        // 이 코드는 execl 실패 시에만 실행됨
        write(pfd[1], "brother send to Parent about hello world\n",
              sizeof("brother send to Parent about hello world\n"));

        close(pfd[1]);
        _exit(127);  // exec 실패 종료
    }

    // 🔸 부모 프로세스
    else
    {
        printf("mother pfd0 = %d, pfd1 = %d\n", pfd[0], pfd[1]);

        close(pfd[1]);  // 부모는 쓰기 안 함 → 쓰기 디스크립터 닫기


        // 자식이 보낸 첫 메시지 읽기 (예: date 명령어 결과)
        while((n  = read(pfd[0], line, BUFSIZ))>0)
        {
            fwrite(line, 1, n, stdout);
        }

        close(pfd[0]);
        waitpid(pid, &status, 0); // 자식 종료 대기
    }

    return 0;
}

