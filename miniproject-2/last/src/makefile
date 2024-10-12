# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2

# Server target executable
# SERVER_TARGET = chat_server
SERVER_TARGET = chat_server

# Client target executable
CLIENT_TARGET = chat_client
CLIENT_TARGET = chat_client

# Object files for server
SERVER_OBJS = server_main.o server_functions.o
SERVER_OBJS = server_main.o server_functions.o

# Object files for client
CLIENT_OBJS = client_main.o client_functions.o
CLIENT_OBJS = client_main.o client_functions.o

# Default target: Compile both server and client
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Rule to link server object files into the server executable
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

# Rule to link client object files into the client executable
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

# Compile server main file
server_main.o: server_main.c server_functions.h
	$(CC) $(CFLAGS) -c server_main.c
server_main.o: server_main.c server_functions.h
	$(CC) $(CFLAGS) -c server_main.c

# Compile server functions file
server_functions.o: server_functions.c server_functions.h
	$(CC) $(CFLAGS) -c server_functions.c
server_functions.o: server_functions.c server_functions.h
	$(CC) $(CFLAGS) -c server_functions.c

# Compile client main file
client_main.o: client_main.c client_functions.h
	$(CC) $(CFLAGS) -c client_main.c
client_main.o: client_main.c client_functions.h
	$(CC) $(CFLAGS) -c client_main.c

# Compile client functions file
client_functions.o: client_functions.c client_functions.h
	$(CC) $(CFLAGS) -c client_functions.c
client_functions.o: client_functions.c client_functions.h
	$(CC) $(CFLAGS) -c client_functions.c

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)

# .PHONY to avoid conflicts with files named clean or all
.PHONY: clean all
