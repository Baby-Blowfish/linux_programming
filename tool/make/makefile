# ========= [기본 설정] =========
CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude -MMD -MP

# ========= [디렉토리 변수] =========
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# ========= [타겟 정의] =========
TARGETS = $(BIN_DIR)/app1 $(BIN_DIR)/app2

# ========= [소스 파일 정의] =========
APP1_SRCS = $(SRC_DIR)/main1.c $(SRC_DIR)/util1.c
APP2_SRCS = $(SRC_DIR)/main2.c $(SRC_DIR)/util2.c

# ========= [오브젝트 파일 정의] =========
# obj/main1.o, obj/util1.o 식으로 변환
APP1_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(APP1_SRCS))
APP2_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(APP2_SRCS))

# ========= [의존성 파일 정의] =========
APP1_DEPS = $(APP1_OBJS:.o=.d)
APP2_DEPS = $(APP2_OBJS:.o=.d)

OBJS = $(APP1_OBJS) $(APP2_OBJS)
DEPS = $(APP1_DEPS) $(APP2_DEPS)

# ========= [빌드 시작 전 디렉토리 생성] =========
# 디렉토리가 없을 경우 생성
.PHONY: all
all: $(BIN_DIR) $(OBJ_DIR) $(TARGETS)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# ========= [실행파일 생성 규칙] =========
$(BIN_DIR)/app1: $(APP1_OBJS)
	$(CC) -o $@ $^

$(BIN_DIR)/app2: $(APP2_OBJS)
	$(CC) -o $@ $^

# ========= [공통 컴파일 규칙: .c -> obj/*.o] =========
# src/main1.c → obj/main1.o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ========= [자동 의존성 포함 (.d 파일)] =========
# .d 파일이 있을 경우 include
-include $(DEPS)

# ========= [청소 규칙] =========
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

