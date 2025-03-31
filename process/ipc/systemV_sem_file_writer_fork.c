#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define SEM_KEY 1234
#define NUM_PROC 5

// P 연산: 자원 획득
void sem_P(int semid) {
    struct sembuf p = {0, -1, SEM_UNDO};
    semop(semid, &p, 1);
}

// V 연산: 자원 반환
void sem_V(int semid) {
    struct sembuf v = {0, 1, SEM_UNDO};
    semop(semid, &v, 1);
}

int main() {
    // 1. 세마포어 생성
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    // 2. 세마포어 초기값 설정 (1개 자원 보유 상태)
    union semun {
        int val;
    } arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);

    // 3. 자식 프로세스 생성
    for (int i = 0; i < NUM_PROC; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // 자식 프로세스
            sem_P(semid);  // 자원 요청 (잠금)

            FILE *fp = fopen("log.txt", "a");
            if (fp) {
                fprintf(fp, "Child PID: %d | Time: %ld\n", getpid(), time(NULL));
                fclose(fp);
            }

            printf("✔️ 자식 %d: 로그 기록 완료\n", getpid());
            sem_V(semid);  // 자원 해제
            exit(0);
        }
    }

    // 자식 종료 감지
    int status, exited = 0;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        printf("🔙 종료된 자식: PID=%d, status=%d\n", wpid, status);
        exited++;
    }

    printf("총 %d개의 자식이 종료됨\n", exited);

    // 5. 세마포어 제거
    semctl(semid, 0, IPC_RMID);
    printf("🗑️ 세마포어 제거 완료\n");

    return 0;
}

