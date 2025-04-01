# Makefile for TCP Server

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pthread

# Source files
SRCS = main.c tcp_server.c video_capture.c

# header files
HDRS = tcp_server.h video_capture.h

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = tcp_server

# Default rule to build the target
all: $(TARGET)

# Rule to link object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)
