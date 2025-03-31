#include<stdio.h>       // 표준 입출력 함수들 (printf 등)
#include<signal.h>      // 시그널 관련 함수 및 상수 정의
#include<stdlib.h>      // 표준 라이브러리 함수 (exit, abort 등)
#include<string.h>      // 문자열 처리 함수 (strsignal)
#include<unistd.h>      // POSIX 함수들 (pause, alarm 등)

// 시그널 집합을 출력하는 함수 선언
static void printSigset(sigset_t *set);

// 시그널 핸들러 함수 선언
static void sigHandler(int);

int main(int argc, char **argv)
{
    sigset_t pset;

    // 시그널 집합을 비움 (초기화)
    sigemptyset(&pset);

    // SIGQUIT와 SIGRTMIN을 시그널 집합에 추가
    sigaddset(&pset, SIGQUIT);
    sigaddset(&pset, SIGRTMIN);

    // 위에서 설정한 시그널 집합을 현재 프로세스의 마스크에 블록으로 설정
    // → SIGQUIT, SIGRTMIN은 현재 차단(block) 상태
    sigprocmask(SIG_BLOCK, &pset, NULL);

    // 차단된 시그널 목록 출력
    printSigset(&pset);

    // 시그널 핸들러 등록: SIGINT (Ctrl+C)
    if(signal(SIGINT, sigHandler) == SIG_ERR)
    {
        perror("signal() : SIGINT");
        return -1;
    }

    // 사용자 정의 시그널 1번 (SIGUSR1)
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal() : SIGUSR1");
        return -1;
    }

    // 사용자 정의 시그널 2번 (SIGUSR2)
    if(signal(SIGUSR2, sigHandler) == SIG_ERR)
    {
        perror("signal() : SIGUSR2");
        return -1;
    }

    // SIGALRM 시그널 등록
    if(signal(SIGALRM, sigHandler) == SIG_ERR)
    {
        perror("signal() : SIGALRM");
        return -1;
    }

    // 5초 후 SIGALRM 시그널 발생 예약
    alarm(5);

    // SIGABRT 시그널 등록
    if(signal(SIGABRT, sigHandler) == SIG_ERR)
    {
        perror("signal() : SIGABRT");
        return -1;
    }

    // SIGABRT 시그널을 강제로 발생시킴 (핸들러가 호출될 것)
    //abort();

    // 프로세스 그룹에 SIGINT 시그널 전송 (Ctrl+C 효과)
    //kill(0, SIGINT);

    // SIGPIPE 시그널을 무시하도록 설정 (파이프 오류 무시)
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("signal() : SIGPIPE");
        return -1;
    }

    // 시그널 대기 (시그널이 올 때까지 프로세스 블로킹)
    while(1) pause();

    return 0;
}

static void sigHandler(int signo)
{
    if(signo == SIGINT)
    {
        // Ctrl+C 또는 kill로 들어온 SIGINT 처리
        printf("SIGINT is catched : %d\n", signo);
        exit(0);  // 프로그램 종료
    }
    else if(signo == SIGUSR1)
    {
        printf("SIGUSR1! is catched\n");
    }
    else if(signo == SIGUSR2)
    {
        printf("SIGUSR2! is catched\n");
    }
    else if(signo == SIGALRM)
    {
        // alarm()에 의해 발생한 SIGALRM 처리
        printf("SIGALRM! is catched\n");
    }
    else if(signo == SIGABRT)
    {
        // abort() 호출 시 발생하는 시그널
        printf("SIGABRT! is catched\n");
    }
    else if(signo == SIGQUIT)
    {
        // SIGQUIT 시그널이 들어왔을 경우
        printf("SIGQUIT! is catched\n");

        // SIGINT 시그널을 차단 해제 (unblock)하는 예제
        sigset_t uset;
        sigemptyset(&uset);
        sigaddset(&uset, SIGINT);
        sigprocmask(SIG_UNBLOCK, &uset, NULL);
    }
    else
    {
        // 등록되지 않은 기타 시그널 처리
        fprintf(stderr, "Catched signal : %s\n", strsignal(signo));
    }
}


static void printSigset(sigset_t *set)
{
    int i;

    for(i = 1; i < NSIG; ++i)
    {
        printf((sigismember(set, i)) ? "1" : "0");
    }
    putchar('\n');
}

