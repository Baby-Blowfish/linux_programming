#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100
#define MAX_CLIENTS 8

// TCP ���� ����
extern int ssock;              // ���� ���� ��ũ����
extern int client_count;    // ���� ���ӵ� Ŭ���̾�Ʈ ��

// ���� ����
char mesg[BUFSIZ];            // �޽��� ����, Ŭ���̾�Ʈ���� �ۼ��ŵǴ� ������ �����

// Ŭ���̾�Ʈ ���� ����ü ����
typedef struct {
    int csock;                      // Ŭ���̾�Ʈ ���� ��ũ���� (Ŭ���̾�Ʈ�� ������ ����ϴ� ����)
    pid_t pid;                      // �ڽ� ���μ��� ID (Ŭ���̾�Ʈ ������ ó���ϴ� �ڽ� ���μ��� ID)
    int client_id;                  // Ŭ���̾�Ʈ ���� ID (�������� �����ϱ� ���� ID)
    int child_to_parent[2];         // �ڽ� -> �θ�� �����͸� �����ϴ� ������
    int parent_to_child[2];         // �θ� -> �ڽ����� �����͸� �����ϴ� ������
    char mesg[BUFSIZ];              // Ŭ���̾�Ʈ �޽��� ���� (Ŭ���̾�Ʈ���� ���ŵ� �����͸� ����)
} __attribute__((packed, aligned(4))) Client;  // 4����Ʈ�� ����

extern Client* clients[MAX_CLIENTS];  // �������� Ŭ���̾�Ʈ�� �����ϱ� ���� ������ �迭

// Ŭ���̾�Ʈ ���� �Լ� ����
void remove_client(int index); // Ŭ���̾�Ʈ�� ����� �� Ŭ���̾�Ʈ�� ��Ͽ��� �����ϴ� �Լ�
// Signal �ڵ鷯 ����
void sigHandlerParent(int signo);    // ���� �� signal �ڵ鷯 (�θ� ���μ����� ó��)
void sigHandlerChild(int signo);     // Ŭ���̾�Ʈ �� signal �ڵ鷯 (�ڽ� ���μ����� ó��)

#endif
