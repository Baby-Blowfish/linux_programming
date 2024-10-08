#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 100  // Maximum buffer size for messages
#define MAX_CLNT 256  // Maximum number of clients that can connect

// Function to display an error message and terminate the program
void error_handling(char *msg);

// Initializes the server socket and binds it to the given port
// Returns the server socket descriptor
int init_server(int port); 

// Global variables to track connected clients
extern int clnt_cnt;                     // Number of currently connected clients
extern int clnt_socks[MAX_CLNT];          // Array to store client socket descriptors
extern pthread_mutex_t mutx;              // Mutex to protect shared resources

#endif // TCP_SERVER_H
