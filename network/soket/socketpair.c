/**
 * @file socketpair_ipc.c
 * @brief socketpair()를 이용한 부모-자식 IPC 예제
 * @details AF_LOCAL 소켓을 통해 부모 → 자식 간 데이터 전송을 구현한 코드입니다.
 * @author
 * Kim Hyo Jin
 */

#include <stdio.h>          // printf, perror
#include <unistd.h>         // fork, write, read, close
#include <string.h>         // strlen
#include <sys/wait.h>       // wait, WEXITSTATUS
#include <sys/socket.h>     // socketpair

int main(int argc, char **argv)
{
    int ret, sock_fd[2];            // 소켓 쌍을 저장할 배열
    int status;                     // 자식 종료 상태 저장 변수
    char buf[] = "hello world";     // 전송할 데이터
    char line[BUFSIZ];              // 수신할 버퍼
    pid_t pid;

    // 1. socketpair() 생성 - 양방향 통신 가능한 소켓 쌍 생성
    ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sock_fd);
    if (ret == -1)
    {
        perror("socketpair() : ");
        return -1;
    }

    // 디버깅용 출력
    printf("socket 1 : %d\n", sock_fd[0]);
    printf("socket 2 : %d\n", sock_fd[1]);

    // 2. fork()를 통해 부모-자식 프로세스 분기
    if ((pid = fork()) < 0)
    {
        perror("fork() :");
    }
    else if (pid == 0)
    {
        // === 자식 프로세스 ===
        // sock_fd[0]을 사용해 부모에게 데이터 전송
        write(sock_fd[0], buf, strlen(buf) + 1);
        printf("Child Process - Data Sent : %s\n", buf);

        // 소켓 닫기
        close(sock_fd[0]);
    }
    else
    {
        // === 부모 프로세스 ===
        wait(&status);  // 자식 종료 대기

        // sock_fd[1]을 사용해 자식이 보낸 데이터 수신
        read(sock_fd[1], line, BUFSIZ);
        printf("Parent Process - Data Read : %s\n", line);

        // 소켓 닫기
        close(sock_fd[1]);

        // 자식 프로세스 종료 상태 출력
        printf("Child Exit Status : %d\n", WEXITSTATUS(status));
    }

    return 0;
}

