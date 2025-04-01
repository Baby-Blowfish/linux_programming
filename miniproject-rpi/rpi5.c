#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <wiringPi.h>
#include <softTone.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <linux/input.h>

#define SW   9          // 스위치 핀 번호
#define LED  8          // LED 핀 번호
#define CDS  0          // 조도 센서(CDS) 핀 번호
#define SPKR 6          // 스피커 핀 번호
#define TOTAL 32        // 연주할 음표의 총 개수

/* 공유 자원 구조체 정의 */
typedef struct {
    pthread_mutex_t cds_lock;  // CDS 센서에 대한 뮤텍스
    pthread_mutex_t sw_lock;   // 스위치에 대한 뮤텍스
    bool is_run;               // 프로그램 종료 플래그
    bool f_led;                // LED 상태 플래그 (true: 켜짐, false: 꺼짐)
    bool f_alram;              // 알람 상태 플래그 (true: 알람 울림, false: 알람 중지)
} SharedData;

/* 키보드 입력 확인을 위한 함수 선언 */
int kbhit(void);

/* 학교종 연주를 위한 계이름 배열 */
int notes[] = {
  391, 391, 440, 440, 391, 391, 329, 329,
  391, 391, 329, 329, 293, 293, 293, 0,
  391, 391, 440, 440, 391, 391, 329, 329,
  391, 329, 293, 329, 261, 261, 261, 0
};

/* 조도 센서 값을 읽고 LED를 제어하는 스레드 함수 */
void *cdsFunction(void *arg)
{
    SharedData *data = (SharedData *)arg;

    pinMode(CDS, INPUT);   // CDS 핀을 입력 모드로 설정
    pinMode(LED, OUTPUT);  // LED 핀을 출력 모드로 설정

    while (data->is_run) {
        // 뮤텍스를 사용하여 임계 구역 설정
        if (pthread_mutex_trylock(&data->cds_lock) != EBUSY) {
            // CDS 센서 값이 LOW일 때 (빛이 없을 때)
            if (digitalRead(CDS) == LOW) {
                data->f_led = false;          // LED 상태 플래그를 false로 설정
                digitalWrite(LED, HIGH);      // LED 켬
                printf("No light, LED flag : false \n");
            } else {                          // CDS 센서 값이 HIGH일 때 (빛이 있을 때)
                data->f_led = true;           // LED 상태 플래그를 true로 설정
                digitalWrite(LED, LOW);       // LED 끔
                printf("There is light, LED flag : true \n");
            }

            pthread_mutex_unlock(&data->cds_lock);  // 임계 구역 해제
        }
        delay(1000);  // 1초 대기
    }
    return NULL;
}

/* 스위치 입력을 감지하고 알람 플래그를 제어하는 스레드 함수 */
void *switchFunction(void *arg)
{
    SharedData *data = (SharedData *)arg;

    pinMode(SW, INPUT);  // 스위치 핀을 입력 모드로 설정

    while (data->is_run) {
        // 뮤텍스를 사용하여 임계 구역 설정
        if (pthread_mutex_trylock(&data->sw_lock) != EBUSY) {
            // 스위치가 눌렸을 때 (LOW)
            if (digitalRead(SW) == LOW) {
                delay(300);               // 디바운싱 처리 (스위치의 떨림을 방지)
                data->f_alram = !data->f_alram;  // 알람 상태 토글
            }

            printf("Alram flag : %s \n", data->f_alram ? "true" : "false");
            pthread_mutex_unlock(&data->sw_lock);  // 임계 구역 해제
        }
        delay(1000);  // 1초 대기
    }
    return NULL;
}

/* 알람 플래그에 따라 음악을 재생하는 스레드 함수 */
void *musicPlayFunction(void *arg)
{
    SharedData *data = (SharedData *)arg;
    int i;
    softToneCreate(SPKR);  // 스피커 핀에서 소프트 톤을 생성

    while (data->is_run) {
        for (i = 0; i < TOTAL; ++i) {
            pthread_mutex_lock(&data->sw_lock);  // 스위치에 대한 뮤텍스 잠금
            if (data->f_alram && data->f_led) {
                softToneWrite(SPKR, notes[i]);  // 알람 플래그와 LED 플래그가 true일 때 음표 재생
            } else {
                softToneWrite(SPKR, 0);         // 조건을 만족하지 않으면 스피커를 끔
            }
            pthread_mutex_unlock(&data->sw_lock);  // 뮤텍스 해제
            delay(280);   // 음표 간 대기 시간
	    
        }
    }
    return NULL;
}

/* 메인 함수 */
int main(int argc, char **argv)
{
    int i;
    wiringPiSetup();  // wiringPi 라이브러리 초기화

    // 공유 자원 초기화
    SharedData data = {
        .is_run = true,
        .f_led = false,
        .f_alram = false
    };
    pthread_mutex_init(&data.cds_lock, NULL);  // 뮤텍스 초기화
    pthread_mutex_init(&data.sw_lock, NULL);

    pthread_t ptCds, ptSwitch, ptMusic;
    // 스레드 생성 및 실행
    pthread_create(&ptCds, NULL, cdsFunction, (void *)&data);
    pthread_create(&ptSwitch, NULL, switchFunction, (void *)&data);
    pthread_create(&ptMusic, NULL, musicPlayFunction, (void *)&data);

    printf("q : Quit\n");
    for (i = 0; data.is_run; i++) {
        if (kbhit()) {  // 키보드 입력이 있는지 확인
            switch (getchar()) {  // 입력된 문자를 읽음
            case 'q':  // 'q'가 입력되면 프로그램 종료
                data.is_run = false;
                goto END;
            };
        }
        delay(100);  // 100ms 대기
    }

END:
    printf("Good Bye!\n");

    pthread_join(ptCds, NULL);     // cdsFunction 스레드 종료 대기
    pthread_join(ptSwitch, NULL);  // switchFunction 스레드 종료 대기
    pthread_join(ptMusic, NULL);   // musicPlayFunction 스레드 종료 대기

    pthread_mutex_destroy(&data.cds_lock);  // 뮤텍스 삭제
    pthread_mutex_destroy(&data.sw_lock);

    return 0;
}

/* 키보드 입력을 확인하는 함수 */
int kbhit(void)
{
    struct termios oldt, newt;    /* 터미널에 대한 구조체 */
    int ch, oldf;

    tcgetattr(0, &oldt);          /* 현재 터미널에 설정된 정보를 가져온다. */
    newt = oldt;
    newt.c_lflag &= ~(ICANON);    /* 정규 모드 입력과 에코를 해제한다. */
    tcsetattr(0, TCSANOW, &newt); /* 새 값으로 터미널을 설정한다. */
    oldf = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, oldf | O_NONBLOCK);  /* 입력을 논블로킹 모드로 설정한다. */

    ch = getchar();

    tcsetattr(0, TCSANOW, &oldt); /* 기존의 값으로 터미널의 속성을 바로 적용한다. */
    fcntl(0, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);        /* 읽은 문자를 입력 스트림으로 되돌린다. */
        return 1;
    }

    return 0;
}
