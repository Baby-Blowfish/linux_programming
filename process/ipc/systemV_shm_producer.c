#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define BUF_SIZE 5

#define SEM_MUTEX 0
#define SEM_EMPTY 1
#define SEM_FULL  2

// 공유 메모리 구조체
typedef struct {
    int buffer[BUF_SIZE];
    int in;
    int out;
} shm_t;

// P 연산
void sem_P(int semid, int semnum) {
    struct sembuf p = {semnum, -1, SEM_UNDO};
    if (semop(semid, &p, 1) == -1) {
        perror("sem_P");
        exit(1);
    }
}

// V 연산
void sem_V(int semid, int semnum) {
    struct sembuf v = {semnum, 1, SEM_UNDO};
    if (semop(semid, &v, 1) == -1) {
        perror("sem_V");
        exit(1);
    }
}

int main() {
    int shmid, semid;
    shm_t *shm;

    // 1. 공유 메모리 접근
    shmid = shmget(SHM_KEY, sizeof(shm_t), 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    shm = (shm_t *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // 2. 세마포어 접근
    semid = semget(SEM_KEY, 3, 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    srand(time(NULL));

    // 3. 데이터 5개 생산
    for (int i = 0; i < 5; ++i) {
        int data = rand() % 100;

        sem_P(semid, SEM_EMPTY);  // 빈 슬롯 확보
        sem_P(semid, SEM_MUTEX);  // 임계영역 진입

        // 생산
        shm->buffer[shm->in] = data;
        printf("🟢 Producer: Produced %d at index %d\n", data, shm->in);
        shm->in = (shm->in + 1) % BUF_SIZE;

        sem_V(semid, SEM_MUTEX);  // 임계영역 해제
        sem_V(semid, SEM_FULL);   // 소비 가능 데이터 알림

        sleep(1);  // 생산 간격
    }

    return 0;
}

