
#ifndef LOGGER_H
#define LOGGER_H

void write_log(const char *level, const char *format, ...);

// 간편 매크로: 로그 레벨별 호출
#define LOG_INFO(...)  write_log("INFO",  __VA_ARGS__)
#define LOG_WARN(...)  write_log("WARN",  __VA_ARGS__)
#define LOG_ERROR(...) write_log("ERROR", __VA_ARGS__)

#endif

