#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>

// 터미널 비표준 모드 설정
void enable_raw_mode(struct termios *saved)
{
    struct termios newt;

    // 현재 설정 저장
    tcgetattr(STDIN_FILENO, saved);
    newt = *saved;

    // 캐노니컬, 에코 제거
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 논블로킹 모드로 변경
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
}

// 터미널 복구
void disable_raw_mode(const struct termios *saved)
{
    tcsetattr(STDIN_FILENO, TCSANOW, saved);
}

// 키 입력 처리
void handle_input(char *buf, int len)
{
    if (len == 1) {
        switch (buf[0]) {
            case 'w': printf("앞으로 이동 🚶\n"); break;
            case 'a': printf("왼쪽으로 이동 ⬅️\n"); break;
            case 's': printf("뒤로 이동 🔙\n"); break;
            case 'd': printf("오른쪽으로 이동 ➡️\n"); break;
            case ' ': printf("정지 🛑\n"); break;
            case 27:  printf("ESC 입력됨 🧩\n"); break;
        }
    } else if (len == 3 && buf[0] == 27 && buf[1] == '[') {
        switch (buf[2]) {
            case 'A': printf("↑ 방향키 입력\n"); break;
            case 'B': printf("↓ 방향키 입력\n"); break;
            case 'C': printf("→ 방향키 입력\n"); break;
            case 'D': printf("← 방향키 입력\n"); break;
        }
    }
}

int main()
{
    struct termios original_termios;
    char buf[4];
    int n;

    enable_raw_mode(&original_termios);

    printf("🔵 실시간 키 매핑 시작 (q: 종료)\n");

    while (1) {
        n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            if (buf[0] == 'q') break;
            handle_input(buf, n);
        }
        usleep(100000);
    }

    disable_raw_mode(&original_termios);
    printf("🟡 프로그램 종료!\n");
    return 0;
}

