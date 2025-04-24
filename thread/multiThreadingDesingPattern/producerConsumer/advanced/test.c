#include "producer_consumer.h"  // 공유 버퍼 구조체와 스레드 함수 선언 포함
#include <pthread.h>            // pthread_t, pthread_create 등 POSIX 스레드 사용
#include <stdlib.h>             // 일반 유틸 함수들 (예: malloc, exit)
#include <stdio.h>              // printf, fprintf 등 표준 입출력
#include <unistd.h>             // pipe(), read(), write(), sleep() 등 시스템 호출
#include <sys/epoll.h>          // epoll API 사용 (epoll_create1, epoll_ctl, epoll_wait)
#include <signal.h>             // signal 처리 (sigaction)
#include <fcntl.h>              // fcntl()로 fd를 non-blocking으로 설정하기 위해 사용

int sig_pipe_fd[2];             // pipe file descriptor: [0] 읽기용, [1] 쓰기용

// ──────────────────────────────────────────────────────────────
// Signal handler: Ctrl+C (SIGINT) 수신 시 pipe에 1바이트 씀
// 이 write는 async-signal-safe 함수이며 epoll에서 감지 가능
void handle_sigint(int signo) {
    char c = 'x';
    write(sig_pipe_fd[1], &c, 1);  // signal-safe: epoll 감지용 바이트 쓰기
}

// SIGINT 발생 시 handle_sigint() 실행하도록 설정
void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;     // 핸들러 함수 지정
    sigemptyset(&sa.sa_mask);         // 추가 블록 시그널 없음
    sa.sa_flags = 0;                  // 특별한 플래그 없음
    sigaction(SIGINT, &sa, NULL);     // SIGINT (Ctrl+C) 등록
}

int main() {
    shared_data_t shared;
    queue_init(&shared);             // 버퍼, mutex, cond 초기화

    // ────── Pipe 생성 및 Non-block 설정 ──────
    pipe(sig_pipe_fd);                              // pipe 생성: signal → main 알림용
    fcntl(sig_pipe_fd[0], F_SETFL, O_NONBLOCK);     // read end non-blocking
    fcntl(sig_pipe_fd[1], F_SETFL, O_NONBLOCK);     // write end non-blocking

    // ────── Signal 핸들러 등록 ──────
    setup_signal_handler();                         // Ctrl+C 감지용 핸들러 등록

    // ────── epoll 등록: pipe[0]을 감시 ──────
    int epfd = epoll_create1(0);                    // epoll 인스턴스 생성
    struct epoll_event ev;
    ev.events = EPOLLIN;                            // 읽기 이벤트 감지 설정
    ev.data.fd = sig_pipe_fd[0];                    // pipe read end 등록
    epoll_ctl(epfd, EPOLL_CTL_ADD, sig_pipe_fd[0], &ev);

    // ────── Producer, Consumer 스레드 시작 ──────
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    thread_arg_t args[NUM_PRODUCERS + NUM_CONSUMERS];

    // 각 producer 생성
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].thread_id = i;
        args[i].shared = &shared;
        pthread_create(&producers[i], NULL, producer, &args[i]);
    }

    // 각 consumer 생성
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        args[NUM_PRODUCERS + i].thread_id = i;
        args[NUM_PRODUCERS + i].shared = &shared;
        pthread_create(&consumers[i], NULL, consumer, &args[NUM_PRODUCERS + i]);
    }

    // ────── epoll 대기: Ctrl+C 수신 시까지 blocking ──────
    struct epoll_event events[1];                   // 감지된 이벤트 저장용
    printf("🧭 Waiting for Ctrl+C (SIGINT)...\n");
    epoll_wait(epfd, events, 1, -1);                // 무한 대기: signal 발생 시 unblock

    // ────── signal 감지 후 graceful shutdown 트리거 ──────
    char dummy;
    read(sig_pipe_fd[0], &dummy, 1);                // pipe 안의 dummy 바이트 소비
    printf("🛑 SIGINT received. Setting done=1...\n");

    pthread_mutex_lock(&shared.mutex);              // shutdown 안전 동기화를 위해 lock
    shared.done = 1;                                // 모든 스레드 종료 플래그 설정
    pthread_cond_broadcast(&shared.cond_not_empty); // 대기 중인 consumer 깨우기
    pthread_cond_broadcast(&shared.cond_not_full);  // 대기 중인 producer 깨우기
    pthread_mutex_unlock(&shared.mutex);            // unlock

    // ────── 모든 스레드 종료 대기 ──────
    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(producers[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(consumers[i], NULL);

    printf("✅ All threads exited cleanly.\n");
    return 0;
}
