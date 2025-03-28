#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// 키 입력 감지 함수
int kbhit(void) {
    struct termios oldt, newt;
    int ch, oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);       // 비표준 모드, 입력 감춤
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// 즉시 한 글자 읽기 (버퍼링 안 됨)
char getch(void) {
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);       // 비표준 입력 모드
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main() {
    printf("키 매핑 인터페이스 시작\n");
    printf("[w:앞 s:뒤 a:좌 d:우 SPACE:정지 q:종료]\n");

    while (1) {
        if (kbhit()) {
            char c = getch();

            switch (c) {
                case 'w': printf("앞으로 이동 🚶\n"); break;
                case 's': printf("뒤로 이동 🔙\n"); break;
                case 'a': printf("왼쪽 이동 ⬅️\n"); break;
                case 'd': printf("오른쪽 이동 ➡️\n"); break;
                case ' ': printf("정지 🛑\n"); break;
                case 'q': printf("종료합니다 👋\n"); return 0;
                default:  printf("알 수 없는 키: %c\n", c); break;
            }
        }

        usleep(50000); // 50ms 대기
    }

    return 0;
}

