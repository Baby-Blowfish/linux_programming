#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

#define SHM_KEY 0x12345 // 공유 메모리 식별을 위한 키 값

int main(int argc, char**argv)
{
    int i, pid, shmid;
    int *cVal;                  // 공유 메모리에서 사용할 정수형 포인터
    void *shmmem = (void*)0;    // 공유 메모리를 연결할 주소

    // 🔸 자식 프로세스 코드
    if((pid = fork()) == 0)
    {
        // 1. 공유 메모리 생성 (없으면 생성, 있으면 접근)
        shmid = shmget((key_t)SHM_KEY, sizeof(int), 0666 | IPC_CREAT);
        if(shmid == -1)
        {
            perror("shmget : ");
            return -1;
        }

        // 2. 공유 메모리를 현재 프로세스 주소 공간에 attach
        shmmem = shmat(shmid, (void *)0, 0);
        if(shmmem == (void*) -1)
        {
            perror("shmmem : ");
            return -1;
        }

        cVal = (int *)shmmem;   // void* → int*로 변환
        *cVal = 1;              // 초기 값 설정

        // 3. 값 증가하면서 출력
        for(i = 0; i < 3; i++)
        {
            *cVal += 1;         // 공유 변수 값 증가
            printf("Child(%d) : %d\n", i, *cVal);
            sleep(1);
        }
    }

    // 🔸 부모 프로세스 코드
    else if(pid > 0)
    {
        // 1. 자식과 동일한 키로 공유 메모리 접근
        shmid = shmget((key_t)SHM_KEY, sizeof(int), 0666 | IPC_CREAT);
        if(shmid == -1)
        {
            perror("shmget : ");
            return -1;
        }

        // 2. attach
        shmmem = shmat(shmid, (void *)0, 0);
        if(shmmem == (void*) -1)
        {
            perror("shmmem : ");
            return -1;
        }

        cVal = (int *)shmmem;

        // 3. 자식이 변경한 값을 읽기
        for(i = 0; i < 3; i++)
        {
            sleep(1);  // 자식이 먼저 값을 바꾸도록 약간의 delay
            printf("Parent(%d) : %d\n", i, *cVal);
        }
    }

    // 🔚 공유 메모리 제거 (부모가 수행)
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}

