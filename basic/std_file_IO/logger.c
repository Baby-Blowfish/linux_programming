#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAX_LOG_SIZE 1024 * 1024  // 1MB

// 로그 파일 이름 생성: log_YYYY-MM-DD[_n].log
void get_log_filename(char *filename, size_t size, int index) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date[16];
    strftime(date, sizeof(date), "%Y-%m-%d", t);
    if (index == 0)
        snprintf(filename, size, "log_%s.log", date);
    else
        snprintf(filename, size, "log_%s_%d.log", date, index);
}

// 로그 파일 크기 확인
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return 0;
}

// 파일 열기 (자동 회전 포함)
FILE *open_log_file() {
    char filename[64];
    int index = 0;

    while (1) {
        get_log_filename(filename, sizeof(filename), index);
        long size = get_file_size(filename);
        if (size < MAX_LOG_SIZE)
            break;
        index++;
    }

    FILE *fp = fopen(filename, "a");
    if (!fp) {
        perror("로그 파일 열기 실패");
        exit(1);
    }
    return fp;
}

// 로그 기록 함수
void write_log(const char *level, const char *format, ...) {
    FILE *log_fp = open_log_file();
    FILE *err_fp = fopen("log_error.log", "a");

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);

    char msg_buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(msg_buf, sizeof(msg_buf), format, args);
    va_end(args);

    // 로그 메시지 출력
    fprintf(log_fp, "[%s] [%s] %s\n", time_buf, level, msg_buf);
    fclose(log_fp);

    // 에러 레벨은 log_error.log에도 저장
    if (strcmp(level, "ERROR") == 0 && err_fp) {
        fprintf(err_fp, "[%s] %s\n", time_buf, msg_buf);
        fclose(err_fp);
    }
}



// int main() {
//     write_log("INFO", "프로그램이 시작되었습니다.");
//     write_log("WARN", "설정 파일이 없습니다. 기본값을 사용합니다.");
//     write_log("ERROR", "디바이스 초기화 실패: 코드 %d", -1);
//     return 0;
// }
//
