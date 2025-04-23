#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

// GPIO 레지스터를 표현하는 구조체. GPIO의 상태와 제어를 담당하는 레지스터 포함.
typedef struct {
    uint32_t status;  // GPIO 상태 레지스터
    uint32_t ctrl;    // GPIO 제어 레지스터
} GPIOregs;

// GPIO 레지스터를 쉽게 접근하기 위해 정의한 매크로
#define GPIO ((GPIOregs*)GPIOBase)

// RIO(입출력) 레지스터를 위한 구조체 정의. GPIO 핀의 입출력 제어와 관련된 레지스터 포함.
typedef struct {
    uint32_t Out;     // 출력 레지스터
    uint32_t OE;      // 출력 enable 레지스터
    uint32_t In;      // 입력 레지스터
    uint32_t InSync;  // 동기화된 입력 레지스터
} rioregs;

// RIO 레지스터에 접근하기 위한 매크로 정의
#define rio ((rioregs *)RIOBase)
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))  // XOR 연산을 위한 레지스터 블록
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))  // GPIO 핀을 설정하는 레지스터 블록
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))  // GPIO 핀을 클리어하는 레지스터 블록

int main(int argc, char **argv)
{
    // /dev/mem을 열어서 물리 메모리 접근을 위한 파일 디스크립터 생성
    int memfd = open("/dev/mem", O_RDWR | O_SYNC);

    // 물리 메모리 주소를 가상 메모리 주소에 매핑
    // 64MB 크기의 메모리를 매핑하며, 베이스 주소는 0x1f00000000으로 설정
    uint32_t *map = (uint32_t *)mmap(
        NULL,
        64 * 1024 * 1024, // 64MB 크기
        (PROT_READ | PROT_WRITE), // 읽기/쓰기 권한
        MAP_SHARED, // 프로세스 간 메모리 공유
        memfd,      // 메모리 파일 디스크립터
        0x1f00000000 // 물리 메모리 주소
    );

    // 메모리 매핑이 실패하면 오류 메시지를 출력하고 종료
    if (map == MAP_FAILED)
    {
        printf("mmap failed: %s\n", strerror(errno));
        return (-1);
    };
    
    // /dev/mem 파일 디스크립터 닫기
    close(memfd);

    // PERIBase는 매핑된 메모리의 시작 주소
    uint32_t *PERIBase = map;

    // GPIO 및 RIO 베이스 주소 설정
    uint32_t *GPIOBase = PERIBase + 0xD0000 / 4; // GPIO 베이스 주소는 0xD0000 오프셋에 있음
    uint32_t *RIOBase = PERIBase + 0xe0000 / 4;  // RIO 베이스 주소는 0xE0000 오프셋에 있음
    uint32_t *PADBase = PERIBase + 0xf0000 / 4;  // PAD 베이스 주소는 0xF0000 오프셋에 있음

    // PAD 레지스터에 대한 포인터 설정
    uint32_t *pad = PADBase + 1;   
    
    // 제어할 GPIO 핀 번호 및 함수 설정
    uint32_t pin = 18;  // GPIO 핀 18 사용
    uint32_t fn = 5;    // 해당 핀의 기능을 설정하는 값 (5는 해당 핀의 특별한 기능을 의미)

    // GPIO 핀을 해당 기능으로 설정
    GPIO[pin].ctrl = fn;

    // 패드 설정. 해당 핀의 패드에 대한 설정 값을 0x10으로 설정
    pad[pin] = 0x10;

    // RIO SET 레지스터에서 출력 enable (OE) 설정
    rioSET->OE = 0x01 << pin;  // 해당 핀을 출력 모드로 설정

    // RIO SET 레지스터에서 출력 (Out) 설정
    rioSET->Out = 0x01 << pin;  // 해당 핀에 논리 1 출력 (High 상태)

    // 무한 루프를 통해 핀의 출력을 반복적으로 토글
    for (;;)
    {
        sleep(1);  // 1초 대기
        rioXOR->Out = 0x01 << pin;  // 핀의 출력을 XOR 연산을 통해 토글 (High -> Low, Low -> High)
    	sleep(1);  // 다시 1초 대기
    }

    return (EXIT_SUCCESS);
}
