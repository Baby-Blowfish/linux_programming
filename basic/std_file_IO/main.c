#include "logger.h"

int main() {
    LOG_INFO("서버가 시작되었습니다");
    LOG_WARN("메모리 사용량 90%% 초과");
    LOG_ERROR("포트 %d를 열 수 없습니다", 8080);

    for (int i = 0; i < 10000; i++) {
        LOG_INFO("자동 로그 #%d", i);
    }

    return 0;
}

