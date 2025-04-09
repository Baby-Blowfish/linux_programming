# Linux Thread Programming Examples

이 프로젝트는 Linux 시스템에서의 스레드 프로그래밍, 동기화, 시그널 처리에 대한 예제들을 포함하고 있습니다.

## 파일 설명

### C 스레드 프로그래밍
- `thread.c`: POSIX 스레드 기본 예제
  - pthread_create()로 스레드 생성
  - pthread_join()으로 스레드 종료 대기
  - 세마포어를 사용한 동기화
  - 공유 자원(cnt)에 대한 접근 제어
  - 네임드 세마포어(sem_open) 사용

- `thread_mutex.c`: 뮤텍스를 사용한 동기화
  - pthread_mutex_init()으로 뮤텍스 초기화
  - pthread_mutex_lock()/unlock()으로 임계 영역 보호
  - 공유 자원(g_var)에 대한 안전한 접근
  - 뮤텍스 파괴(pthread_mutex_destroy)

- `thread_signal.c`: 스레드 시그널 처리 기본
  - pthread_sigmask()로 시그널 마스킹
  - sigemptyset()/sigaddset()으로 시그널 집합 설정
  - 스레드별 시그널 처리
  - kill()로 프로세스에 시그널 전송

- `thread_signal_handling.c`: 고급 시그널 처리
  - 전용 시그널 핸들러 스레드 구현
  - sigwait()으로 시그널 대기
  - SIGINT, SIGTERM 등 다양한 시그널 처리
  - 스레드 간 시그널 전달 메커니즘

### C++ 스레드 프로그래밍
- `cppthread.cpp`: C++11 스레드 기본 예제
  - std::thread 클래스 사용
  - 스레드 생성 및 실행
  - join()/detach() 메서드로 스레드 관리
  - 람다 함수와 스레드

- `cppthreadmutex.cpp`: C++11 뮤텍스 사용
  - std::mutex 클래스로 동기화
  - lock()/unlock() 메서드로 임계 영역 보호
  - RAII 스타일의 뮤텍스 관리
  - 여러 스레드의 동시 실행

## 주요 개념

### 스레드 생성 및 관리
- pthread_create(): POSIX 스레드 생성
- pthread_join(): 스레드 종료 대기
- pthread_detach(): 스레드 분리
- std::thread: C++11 스레드 클래스

### 동기화 메커니즘
- 뮤텍스(Mutex)
  - pthread_mutex_t: POSIX 뮤텍스
  - std::mutex: C++11 뮤텍스
  - 상호 배제 구현
  - 데드락 방지

- 세마포어(Semaphore)
  - sem_t: POSIX 세마포어
  - sem_open(): 네임드 세마포어
  - P/V 연산 (wait/post)
  - 리소스 카운팅

### 시그널 처리
- 시그널 마스킹
  - sigemptyset()/sigaddset()
  - pthread_sigmask()
  - 스레드별 시그널 처리

- 시그널 핸들링
  - sigwait()으로 시그널 대기
  - 전용 시그널 핸들러 스레드
  - 프로세스/스레드 간 시그널 전달

### C++11 스레드 기능
- std::thread 클래스
  - 생성자로 스레드 시작
  - join()/detach() 메서드
  - move 시맨틱스 지원

- std::mutex 클래스
  - lock()/unlock() 메서드
  - RAII 스타일 락 관리
  - std::lock_guard 사용

## 참고사항
- 대부분의 예제는 Linux 환경에서 실행되어야 합니다.
- POSIX 스레드 예제는 -pthread 옵션으로 컴파일해야 합니다.
- C++11 스레드 예제는 -std=c++11 이상의 옵션으로 컴파일해야 합니다.
- 시그널 처리 예제는 터미널에서 Ctrl+C로 테스트할 수 있습니다.
- 동기화 예제는 여러 번 실행하여 결과를 비교해볼 수 있습니다. 