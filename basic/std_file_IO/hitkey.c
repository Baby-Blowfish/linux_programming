#include <stdio.h>
#include <unistd.h>     // read, usleep
#include <termios.h>    // struct termios, tcgetattr, tcsetattr
#include <fcntl.h>      // fcntl, O_NONBLOCK

int main() {
    struct termios oldt, newt;
    int i = 0;
    char c;

    // 1. 현재 터미널 설정 백업
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 2. 비표준 모드 (ICANON 해제), 에코 해제
    newt.c_lflag &= ~(ICANON | ECHO);

    // 3. 터미널 설정 적용
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 4. stdin을 논블로킹으로 설정
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

    // 5. 메인 루프
    while (1) {
        // 실시간 키 입력 감지
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 'q') break;
            printf("\n입력한 키: %c\n", c);
        }

        // 한 줄 덮어쓰기 출력
        printf("\rCount: %d", i++);
        fflush(stdout);

        usleep(100000); // 0.1초
    }

    // 6. 터미널 원상복구
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\nGood Bye!\n");
    return 0;
}

