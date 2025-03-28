#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>   // waitpid()
#include <string.h>     // strlen(), strsignal()
#include <signal.h>     // signal()
#include <stdlib.h>     // exit()

static void sigHandler(int);    // 시그널 처리용 핸들러

// Pipe setting
int child_to_parent[2];         // 파이프를 전역으로 선언

// Buffer setting
char buffer[BUFSIZ];            // 전역으로 버퍼 선언

int main() {

    // Process setting
    pid_t pid;
    int status; // 자식 프로세스 상태

    // Buffer setting
    int readBufferByte;

    // 자식 -> 부모로 보내는 파이프 생성
    if (pipe(child_to_parent) < 0) {
        perror("pipe()");
        return -1;
    }

    // 시그널 처리를 위한 핸들러 등록
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }

    // 프로세스 포크
    if((pid = fork()) < 0)
    {
      perror("fork()");
      return -1;
    }
    else if(pid == 0) // 자식 프로세스
    {
        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);

        do
        {
            memset(buffer, 0, BUFSIZ);  // buffer 초기화

            // 키보드 입력 받기
            if((readBufferByte=read(0, buffer, sizeof(buffer))<0))
            {
                perror(" stdin read()");
            }

            // 자식 -> 부모 쓰기 파이프
            if(write(child_to_parent[1],buffer,strlen(buffer))<=0)
            {
                perror("child_to_parent pipe write()");
            }

            // 부모에게 SIGUSR1 시그널 보내기
            kill(getppid(), SIGUSR1);


        } while (1);

        // 자식 -> 부모 쓰기 파이프 종료
        close(child_to_parent[1]);
    } else // 부모 프로세스
    {
        // 자식 -> 부모 쓰기 파이프 닫기
        close(child_to_parent[1]);

        do
        {
            // server와 연결된 soket의 응답 대기
            // 응답 받은 데이터 출력 printf
        } while (1);


        // 자식 -> 부모 읽기 파이프
            read(child_to_parent[0], buffer, sizeof(buffer));
            printf("Parent received: %s\n", buffer);


        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);

        // 자식 프로세스 종료 대기
        waitpid(pid, &status, 0);
    }

    return 0;
}

// SIGUSR1 시그널 핸들러
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // 파이프에서 데이터 읽기
        memset(buffer, 0, BUFSIZ);
        if (read(child_to_parent[0], buffer, sizeof(buffer)) > 0) {
            printf("Parent received: %s", buffer);
        } else {
            perror("pipe read()");
        }
    }
}