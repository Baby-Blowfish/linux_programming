#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SHM_KEY 0x1234        // 공유 메모리를 위한 고유 키
#define SEM_KEY 0x5678        // 세마포어 집합을 위한 고유 키
#define BUF_SIZE 5            // 버퍼 크기 정의 (생산-소비 버퍼의 슬롯 수)

// 🔸 세마포어 번호 정의 (세마포어 집합 내 인덱스)
#define SEM_MUTEX 0           // 임계영역 보호용 세마포어
#define SEM_EMPTY 1           // 빈 공간 추적용 세마포어
#define SEM_FULL  2           // 채워진 공간 추적용 세마포어

// 🔸 공유 메모리 데이터 구조 정의
typedef struct {
    int buffer[BUF_SIZE];  // 생산자-소비자가 공유할 순환 큐
    int in;                // 생산자가 데이터를 쓸 위치
    int out;               // 소비자가 데이터를 읽을 위치
} shm_t;

int main() {
    int shmid, semid;
    shm_t *shm;

    // 🔹 1. 공유 메모리 생성 또는 접근
    shmid = shmget(SHM_KEY, sizeof(shm_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // 🔹 2. 공유 메모리를 프로세스 주소 공간에 붙이기
    shm = (shm_t *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // 🔹 3. in/out 초기화 (생산자/소비자의 순환 큐 인덱스)
    shm->in = shm->out = 0;

    // 🔹 4. 세마포어 집합 생성 (총 3개 세마포어 포함)
    // → semid는 세마포어 집합의 식별자이며, 인덱스로 각각 접근
    semid = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    // 🔹 5. 세마포어 초기값 설정
    union semun {
        int val;  // 세마포어 값을 설정할 때 사용하는 union
    } arg;

    // ✅ 세마포어 [0] = mutex: 임계영역 보호용 → 1
    // → 한 번에 하나의 프로세스만 공유 메모리에 접근 가능
    arg.val = 1;
    semctl(semid, SEM_MUTEX, SETVAL, arg);

    // ✅ 세마포어 [1] = empty: 비어 있는 슬롯 수 → BUF_SIZE
    // → 처음엔 모든 슬롯이 비어 있으므로 BUF_SIZE만큼 가능
    arg.val = BUF_SIZE;
    semctl(semid, SEM_EMPTY, SETVAL, arg);

    // ✅ 세마포어 [2] = full: 채워진 슬롯 수 → 0
    // → 생산 전이므로 소비 가능한 데이터는 없음
    arg.val = 0;
    semctl(semid, SEM_FULL, SETVAL, arg);

    // 5. Producer fork + exec
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./producer", "producer", NULL);
        perror("execl producer");
        exit(1);
    }

    // 6. Consumer fork + exec
    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./consumer", "consumer", NULL);
        perror("execl consumer");
        exit(1);
    }

    // 7. 부모는 두 자식 기다림
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("✅ Producer & Consumer 종료. 자원 정리 시작...\n");

    // 8. 공유 자원 정리
    shmdt(shm);                              // detach
    shmctl(shmid, IPC_RMID, NULL);           // remove shm
    semctl(semid, 0, IPC_RMID);              // remove semaphores

    printf("🧹 공유 자원 정리 완료\n");
    return 0;


}

