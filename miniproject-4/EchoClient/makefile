# Makefile

CC = gcc
CFLAGS = -Wall -pthread
TARGET = chat_program
OBJ = main.o function.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

main.o: main.c function.h
	$(CC) $(CFLAGS) -c main.c

function.o: function.c function.h
	$(CC) $(CFLAGS) -c function.c

clean:
	rm -f $(OBJ) $(TARGET)

