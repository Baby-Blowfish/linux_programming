#include "client_functions.h"
// SIGUSR1 시그널 핸들러
void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // 파이프에서 데이터 읽기
        memset(buffer, 0, BUFSIZ);
        if (read(child_to_parent[0], buffer, BUFSIZ) > 0) {
            // 서버로 메시지 전송 (비차단 모드로 전송)
            if (write(ssock, buffer, strlen(buffer)) <= 0) {
                perror("parent soket write()");    // 전송 실패 시 오류 출력
            }
        } else {
            perror("pipe read()");
        }
    }
}