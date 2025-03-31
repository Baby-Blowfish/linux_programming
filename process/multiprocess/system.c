#include <sys/wait.h>   // waitpid()
#include <stdio.h>      // printf
#include <errno.h>      // errno, EINTR
#include <unistd.h>     // fork(), execl(), _exit()

// system 함수 사용자 구현 버전
int system(const char *cmd)
{
    pid_t pid;
    int status;

    // 1. fork로 자식 프로세스 생성
    if ((pid = fork()) < 0)
    {
        // fork 실패 시 status = -1로 반환
        status = -1;
    }
    else if (pid == 0)
    {
        // 자식 프로세스
        // "/bin/sh -c cmd" 실행: cmd를 셸을 통해 실행
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);

        // 만약 execl 실패 시, 127을 종료 코드로 반환
        _exit(127); // exec 실패했을 때 자식이 이상하게 실행되는 걸 방지
    }
    else
    {
        // 부모 프로세스: 자식 종료 대기
        while (waitpid(pid, &status, 0) < 0)
        {
            // EINTR: 시그널로 인해 waitpid가 중단된 경우 → 다시 시도
            if (errno != EINTR)
            {
                status = -1; // 다른 에러라면 -1 반환
                break;
            }
        }
    }

    return status; // 자식 종료 상태 반환
}


int main(int argc, char **argv, char **envp)
{
    // 1. 현재 환경 변수 출력
    while (*envp)
        printf("%s\n", *envp++);  // 예: PATH=/usr/bin 등

    // 2. 시스템 명령어 실행
    system("who");        // 현재 로그인 사용자 출력
    system("nocommand");  // 존재하지 않는 명령 → exec 실패 → 127 반환
    system("cal");        // 달력 출력

    return 0;
}

