#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main() {
    FILE *fp;

    // 존재하지 않는 파일 열기 시도 → 에러 발생
    fp = fopen("없는파일.txt", "r");
    if (fp == NULL) {
        // perror 사용: 시스템 오류 메시지를 자동으로 붙여 출력
        perror("파일 열기 실패");

        // fprintf로 stderr에 직접 출력
        fprintf(stderr, "오류 코드: %d (%s)\n", errno, strerror(errno));
    }

    // 정상 출력 테스트
    printf("stdout: 이 메시지는 줄 버퍼링입니다.");
    // 개행 없이 sleep → 출력 안 될 수 있음
    sleep(2);
    printf(" → 줄 끝\n");

    // stderr는 개행 없어도 바로 출력됨
    fprintf(stderr, "stderr: 이 메시지는 즉시 출력됩니다.\n");

    return 0;
}

