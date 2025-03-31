#include <stdio.h>       // 표준 입출력 함수
#include <unistd.h>      // usleep 등 유닉스 표준 함수
#include <fcntl.h>       // 파일 및 리소스 제어 관련 플래그
#include <pthread.h>     // POSIX 스레드 관련 헤더
#include <semaphore.h>   // POSIX 세마포어 사용을 위한 헤더
#include <stdbool.h>     // bool 타입 사용을 위한 헤더

// 전역 변수 선언
sem_t *sem;              // POSIX 네임드 세마포어 포인터
static int cnt = 7;      // 공유 자원 (카운터)
static bool isRun = true; // 루프 제어 변수

// 세마포어 P 연산 (wait) + cnt 감소
void p()
{
    sem_wait(sem);   // 세마포어 자원 요청 (잠금)
    cnt--;           // 공유 자원 감소 (주의: cnt를 이중 감소함 → 아래에서 수정 필요)
}

// 세마포어 V 연산 (post) + cnt 증가
void v()
{
    sem_post(sem);   // 세마포어 자원 반환 (잠금 해제)
    cnt++;           // 공유 자원 증가 (주의: 이중 증가됨)
}

// cnt를 증가시키는 스레드 함수
void *pthreadV(void *arg)
{
    while (isRun)
    {
        usleep(200000);   // 200ms 대기
        v();              // 세마포어 증가 및 cnt 증가
        cnt++;            // 이중 증가 (의도인지 확인 필요)
        printf("increase : %d\n", cnt);
        fflush(stdout);   // 출력 버퍼 강제 출력
    }
    return NULL;
}

// cnt를 감소시키는 스레드 함수
void *pthreadP(void *arg)
{
    while (isRun)
    {
        p();              // 세마포어 감소 및 cnt 감소
        cnt--;            // 이중 감소 (의도인지 확인 필요)
        printf("decrease : %d\n", cnt);
        fflush(stdout);

        // cnt가 0 이하가 되면 루프 종료
        if (cnt <= 0)
            isRun = false;

        usleep(100000);   // 100ms 대기
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t ptV, ptP;  // 스레드 ID 변수

    const char* name = "posix_sem"; // 세마포어 이름 (네임드 세마포어)
    unsigned int value = cnt;       // 초기 세마포어 값

    // 네임드 세마포어 생성 (파일 시스템 기반)
    sem = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, value);

    // 스레드 생성: 증가 스레드
    if (pthread_create(&ptV, NULL, pthreadV, NULL) != 0)
    {
        perror("ptV :");
        return -1;
    }

    // 스레드 생성: 감소 스레드
    if (pthread_create(&ptP, NULL, pthreadP, NULL) != 0)
    {
        perror("ptP :");
        return -1;
    }

    // 스레드 종료 대기
    pthread_join(ptV, NULL);
    pthread_join(ptP, NULL);

    // 세마포어 닫기 및 제거
    sem_close(sem);               // 세마포어 핸들 닫기
    printf("sem_destroy() : %d\n", sem_destroy(sem)); // 메모리 제거 시도
    sem_unlink(name);             // 네임드 세마포어 파일 제거

    return 0;
}

