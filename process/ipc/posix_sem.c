#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>     // POSIX 세마포어 함수들 포함
#include <fcntl.h>         // O_CREAT 등 플래그 정의

static sem_t *sem;         // 전역 세마포어 포인터 (named 세마포어용)

// 세마포어 P 연산 (sem_wait)
static void p() {
  sem_wait(sem);           // 세마포어 값이 0이면 블로킹됨, 1 이상이면 1 감소
  printf("p\n");           // 실행된 순서 확인용 출력
}

// 세마포어 V 연산 (sem_post)
static void v() {
  sem_post(sem);           // 세마포어 값 1 증가, 대기 중인 프로세스 깨움
  printf("v\n");
}

int main(int argc, char **argv) {
  pid_t pid;
  int status;

  const char* name = "posix_sem";    // 세마포어 이름 ("/" 없이도 내부적으로 /dev/shm/sem.posix_sem 생성됨)
  unsigned int value = 0;            // 초기 세마포어 값 0 → 자식은 대기 상태로 시작

  // 1. 세마포어 생성 (없으면 생성, 있으면 열기)
  sem = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, value);
  if (sem == SEM_FAILED) {
    perror("sem_open");
    return -1;
  }

  // 2. fork로 자식 생성
  if ((pid = fork()) < 0) {
    perror("fork()");
    return -1;
  }

  // 3. 자식 프로세스: sem_wait() 10번 호출
  else if (pid == 0) {
    for (int i = 0; i < 10; i++) {
      p();                // sem_wait(): 부모가 올려줄 때까지 대기
    }
    exit(127);            // 종료 코드 명시
  }

  // 4. 부모 프로세스: 1초 간격으로 sem_post() 10번 호출
  else {
    for (int i = 0; i < 10; i++) {
      sleep(1);           // 매초마다 한 번씩 v()
      v();                // sem_post(): 세마포어 값 증가 → 자식 진행 가능
    }

    waitpid(pid, &status, 0); // 자식 종료 대기
  }

  // 5. 세마포어 정리
  sem_close(sem);         // 포인터 닫기
  sem_unlink(name);       // 세마포어 이름 제거 (/dev/shm에서 제거됨)

  return 0;
}

