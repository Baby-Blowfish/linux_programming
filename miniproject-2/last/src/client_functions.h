#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#include <stdio.h>      // ǥ�� ����� �Լ� ���
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strncmp ��)
#include <unistd.h>     // ���н� ǥ�� �Լ� ��� (fork, pipe, close ��)
#include <stdlib.h>     // ǥ�� ���̺귯�� �Լ� ��� (exit ��)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // ���� �Լ� ��� (socket, connect, send, recv ��)
#include <arpa/inet.h>  // ���ͳ� �������� ���� �Լ� (inet_pton ��)



// Signal setting
void sigHandler(int signo);

// Pipe setting
int child_to_parent[2];         // �������� �������� ����

// Buffer setting
char buffer[BUFSIZ];            // �������� ���� ����

// TCP Socket setting
#define TCP_PORT 5100   // ������ ������ ��Ʈ ��ȣ
int ssock;       // ���� ��ũ����


#endif // CLIENT_FUNCTIONS_H
