#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

// sa_sigaction 기반 핸들러 선언
void advancedHandler(int sig, siginfo_t *info, void *context);

int main()
{
    struct sigaction sa;

    // 핸들러 설정 (sa_sigaction 사용)
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = advancedHandler;    // 고급 핸들러 함수 지정
    sa.sa_flags = SA_SIGINFO;             // 중요: SA_SIGINFO 설정 필요
    sigemptyset(&sa.sa_mask);

    // SIGUSR1과 SIGINT에 등록
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    printf("PID: %d - SIGUSR1 또는 SIGINT를 보내보세요.\n", getpid());

    while (1) pause(); // 시그널을 받을 때까지 대기
    return 0;
}

// sa_sigaction 기반 시그널 핸들러 정의
void advancedHandler(int sig, siginfo_t *info, void *context)
{
    printf("\n[시그널 핸들링]\n");
    printf("시그널 번호   : %d (%s)\n", sig, strsignal(sig));
    printf("보낸 프로세스 : PID = %d\n", info->si_pid);
    printf("UID           : %d\n", info->si_uid);
    printf("코드          : %d (0=kill, 1=POSIX 타이머 등)\n", info->si_code);

    if (sig == SIGINT) {
        printf("프로그램 종료됨 (SIGINT)\n");
        exit(0);
    }
}

