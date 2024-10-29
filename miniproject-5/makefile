# 컴파일러와 플래그 설정
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

# 소스 파일 목록 정의
COMMON_SRCS = v4l2.c framebuffer.c common.c
SERVER_SRCS = server.c $(COMMON_SRCS)
CLIENT_SRCS = client.c $(COMMON_SRCS)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# 생성할 실행 파일 이름
TARGET1 = server
TARGET2 = client

# 기본 규칙: 모든 파일을 컴파일하여 실행 파일 생성
all: $(TARGET1) $(TARGET2)

# server 실행 파일 생성 규칙
$(TARGET1): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET1) $(SERVER_OBJS) $(LDFLAGS)

# client 실행 파일 생성 규칙
$(TARGET2): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET2) $(CLIENT_OBJS) $(LDFLAGS)

# 각 .c 파일을 .o로 컴파일하는 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 파일을 정리하는 규칙
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(TARGET1) $(TARGET2)

