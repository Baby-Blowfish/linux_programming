#include <stdio.h>      // 표준 입출력 함수 사용 (printf 등)
#include <unistd.h>     // 유닉스 시스템 함수 사용 (fork, pipe, read, write, close 등)
#include <sys/wait.h>   // waitpid() 함수 사용
#include <string.h>     // 문자열 처리 함수 사용 (strlen, memset 등)
#include <signal.h>     // 시그널 처리 함수 사용 (signal, kill 등)
#include <stdlib.h>     // exit() 함수 사용

static void sigHandler(int);    // 시그널 처리용 핸들러 선언

// 파이프 설정
int child_to_parent[2];     // 자식 -> 부모로 데이터를 전송하는 파이프
int parent_to_child[2];     // 부모 -> 자식으로 데이터를 전송하는 파이프

// 버퍼 설정
char buffer[BUFSIZ];        // 데이터를 주고받을 버퍼

int main() {

    // 프로세스 설정
    pid_t pid;
    int status; // 자식 프로세스 상태 저장 변수

    int readBufferByte;     // 읽은 바이트 수를 저장하는 변수

    // 파이프 생성 (부모 -> 자식, 자식 -> 부모 각각 생성)
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("pipe()");   // 파이프 생성 실패 시 오류 메시지 출력
        return -1;          // 프로그램 종료
    }

    // 시그널 핸들러 등록 (SIGUSR1을 받을 때 실행될 핸들러 설정)
    if (signal(SIGUSR1, sigHandler) == SIG_ERR) {
        perror("signal");   // 시그널 등록 실패 시 오류 메시지 출력
        return -1;
    }

    // 프로세스 생성 (fork)
    if ((pid = fork()) < 0) {
        perror("fork()");   // 프로세스 생성 실패 시 오류 메시지 출력
        return -1;
    }
    else if (pid == 0) // 자식 프로세스
    {
        // 부모 -> 자식 쓰기 파이프 닫기 (자식은 이쪽에서 쓰지 않음)
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 닫기 (자식은 이쪽에서 읽지 않음)
        close(child_to_parent[0]);

        do {
            memset(buffer, 0, BUFSIZ);  // 버퍼 초기화

            // 표준 입력으로부터 데이터를 입력받아 파이프로 전송
            if ((readBufferByte = read(0, buffer, sizeof(buffer))) < 0) {
                perror("stdin read()"); // 입력 실패 시 오류 메시지 출력
            }

            // 자식 -> 부모 파이프로 데이터 전송
            if (write(child_to_parent[1], buffer, strlen(buffer)) <= 0) {
                perror("child_to_parent pipe write()"); // 전송 실패 시 오류 메시지 출력
            }

            // 부모 프로세스에게 SIGUSR1 시그널을 보냄
            kill(getppid(), SIGUSR1);

            memset(buffer, 0, BUFSIZ);  // 버퍼 초기화
            // 부모 -> 자식 파이프에서 데이터 읽기
            read(parent_to_child[0], buffer, sizeof(buffer));
            printf("Child received: %s\n", buffer); // 읽은 데이터 출력

        } while (1);  // 무한 반복

        // 부모 -> 자식 읽기 파이프 종료
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 종료
        close(child_to_parent[1]);

        _exit(0);  // 자식 프로세스 종료
    }
    else // 부모 프로세스
    {
        // 부모 -> 자식 읽기 파이프 닫기 (부모는 이쪽에서 읽지 않음)
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 닫기 (부모는 이쪽에서 쓰지 않음)
        close(child_to_parent[1]);

        do {
            // 서버와 연결된 소켓의 응답 대기 (아직 미완성된 부분)
            // 응답을 받은 데이터를 출력 (이 부분은 추후 소켓 통신으로 변경해야 함)
        } while (1);  // 무한 반복

        // 부모 -> 자식 쓰기 파이프 종료
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 종료
        close(child_to_parent[0]);

        // 자식 프로세스 종료 대기
        waitpid(pid, &status, 0);
    }

    return 0;
}

// SIGUSR1 시그널 핸들러
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // 자식 -> 부모 읽기 파이프에서 데이터 읽기
        memset(buffer, 0, BUFSIZ);  // 버퍼 초기화
        if (read(child_to_parent[0], buffer, sizeof(buffer)) > 0) {
            // 부모 -> 자식 쓰기 파이프에 데이터를 다시 씀 (에코)
            write(parent_to_child[1], buffer, strlen(buffer));
            // 클라이언트가 연결되었을 때 다른 프로세스에게 전송할 수 있게 해야 함
        } else {
            perror("pipe read()"); // 읽기 실패 시 오류 메시지 출력
        }
    }
}
