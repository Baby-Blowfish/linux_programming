# 컴파일러와 플래그 설정
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

# 소스 파일 목록과 객체 파일 목록 정의
SRCS = main.c v4l2.c framebuffer.c common.c
OBJS = $(SRCS:.c=.o)

# 생성할 실행 파일 이름
TARGET = my_program

# 기본 규칙: 모든 파일을 컴파일하여 실행 파일 생성
all: $(TARGET)

# 실행 파일 생성 규칙
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 각 .c 파일을 .o로 컴파일하는 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 파일을 정리하는 규칙
clean:
	rm -f $(OBJS) $(TARGET)
