#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define LOG_FILE "app.log"

// 로그 한 줄 작성 함수
void write_log(const char *message) {
    FILE *fp = fopen(LOG_FILE, "a");  // append 모드
    if (fp == NULL) {
        fprintf(stderr, "로그 파일 열기 실패!\n");
        return;
    }

    // 현재 시간 얻기
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t == NULL) {
        fprintf(stderr, "시간 정보 오류\n");
        fclose(fp);
        return;
    }

    // 로그 형식: [YYYY-MM-DD HH:MM:SS] 메시지
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);
    fprintf(fp, "[%s] %s\n", time_buf, message);

    fclose(fp);
}

int main() {
    char input[256];

    printf("저장할 로그 메시지를 입력하세요 (빈 줄 입력 시 종료):\n");

    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);

        if (strcmp(input, "\n") == 0)
            break;

        // 개행 제거
        input[strcspn(input, "\n")] = '\0';
        write_log(input);
    }

    printf("✅ 로그가 '%s' 파일에 저장되었습니다.\n", LOG_FILE);
    return 0;
}

