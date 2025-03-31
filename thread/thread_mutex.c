#include <stdio.h>          // printf 등 표준 입출력 함수
#include <pthread.h>        // POSIX 스레드 관련 헤더

int g_var = 1;              // 공유 자원: 전역 정수 변수
pthread_mutex_t mid;        // 뮤텍스 객체 선언

// 증가 함수, 감소 함수 선언
void *inc_function(void*);
void *dec_function(void*);

int main(int argc, char**argv)
{
    pthread_t ptInc, ptDec;  // 스레드 ID

    // 뮤텍스 초기화 (기본 속성 사용)
    pthread_mutex_init(&mid, NULL);

    // 스레드 생성: 하나는 증가 함수, 하나는 감소 함수 실행
    pthread_create(&ptInc, NULL, inc_function, NULL);
    pthread_create(&ptDec, NULL, dec_function, NULL);

    // 생성된 두 스레드의 종료를 기다림
    pthread_join(ptInc, NULL);
    pthread_join(ptDec, NULL);

    // 뮤텍스 파괴 (더 이상 사용하지 않으므로 해제)
    pthread_mutex_destroy(&mid);

    return 0;
}

// 전역 변수 증가 함수 (임계 영역 보호)
void *inc_function(void* arg)
{
    pthread_mutex_lock(&mid);   // 뮤텍스 잠금
    printf("Inc : %d < Before\n", g_var);
    g_var++;                    // 공유 자원 수정
    printf("Inc : %d > After\n", g_var);
    pthread_mutex_unlock(&mid); // 뮤텍스 잠금 해제

    return NULL;
}

// 전역 변수 감소 함수 (임계 영역 보호)
void *dec_function(void* arg)
{
    pthread_mutex_lock(&mid);   // 뮤텍스 잠금
    printf("Dec : %d < Before\n", g_var);
    g_var--;                    // 공유 자원 수정
    printf("Dec : %d > After\n", g_var);
    pthread_mutex_unlock(&mid); // 뮤텍스 잠금 해제

    return NULL;
}

