#include <stdio.h>
#include <unistd.h>

// 전역 변수 (데이터 영역)
static int g_var = 1;

// 전역 문자열 (데이터 영역)
char str[] = "PID";

int main(int argc, char **argv)
{
    int var;
    pid_t pid;

    var = 92;  // 지역 변수 (스택 영역)

    // 자식 프로세스 생성
    if ((pid = fork()) < 0)
    {
        perror("[ERROR] : fork()");
    }
    else if (pid == 0)  // 자식 프로세스
    {
        g_var++;      // 자식에서 전역 변수 변경
        var++;        // 자식에서 지역 변수 변경

        // 부모 PID 확인
        printf("Parent %s from Child Process(%d) : %d\n", str, getpid(), getppid());
    }
    else  // 부모 프로세스
    {
        printf("Child %s from Parent Process(%d) : %d\n", str, getpid(), pid);
        sleep(1);  // 자식보다 늦게 출력하기 위해 잠시 대기
    }

    // 부모와 자식 각각에서 현재 PID, 변수 상태 출력
    printf("pid = %d, Global var = %d, var = %d\n", getpid(), g_var, var);

    return 0;
}

