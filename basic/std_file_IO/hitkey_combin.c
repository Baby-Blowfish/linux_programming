#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

// 기존 설정 백업용
struct termios oldt;

// 터미널 비표준 모드 설정
void set_raw_mode() {
    struct termios newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);       // 줄 단위 입력 제거, 에코 제거
    newt.c_iflag &= ~(IXON | ICRNL);        // 흐름 제어, 개행 처리 제거

    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

// 원래대로 복구
void restore_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// 입력 처리 함수
void handle_key(char *buf, int len) {
    if (len == 1) {
        unsigned char c = buf[0];

        switch (c) {
            case 'w': printf("앞으로 이동 🚶\n"); break;
            case 'a': printf("왼쪽 이동 ⬅️\n"); break;
            case 's': printf("뒤로 이동 🔙\n"); break;
            case 'd': printf("오른쪽 이동 ➡️\n"); break;
            case ' ': printf("정지 🛑\n"); break;
            case 'q': printf("종료 명령 🟥\n"); break;
            case 27:  printf("ESC 단독 입력 🧩\n"); break;
            default:  printf("일반 키 입력: %c\n", c); break;
        }
    }
    else if (len == 2 && buf[0] == 27) {
        printf("🟨 Alt + %c\n", buf[1]);
    }
    else if (len == 3 && buf[0] == 27 && buf[1] == '[') {
        switch (buf[2]) {
            case 'A': printf("↑ 방향키 입력\n"); break;
            case 'B': printf("↓ 방향키 입력\n"); break;
            case 'C': printf("→ 방향키 입력\n"); break;
            case 'D': printf("← 방향키 입력\n"); break;
            default:
                printf("알 수 없는 방향 시퀀스\n");
        }
    } else {
        printf("❔ 알 수 없는 입력 (%d bytes): ", len);
        for (int i = 0; i < len; ++i)
            printf("%02X ", (unsigned char)buf[i]);
        printf("\n");
    }
}

int main() {
    set_raw_mode();

    printf("🔵 키 입력 감지 시작 (q: 종료)\n");

    while (1) {
        char buf[8] = {0};
        int n = read(STDIN_FILENO, buf, sizeof(buf));

        if (n > 0) {
            if (n == 1 && buf[0] == 'q') break; // q 입력 시 종료
            handle_key(buf, n);
        }

        usleep(10000);  // 10ms 대기
    }

    restore_mode();
    printf("🟡 종료!\n");
    return 0;
}

