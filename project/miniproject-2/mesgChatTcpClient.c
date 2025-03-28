#include <stdio.h>      // ǥ�� ����� �Լ� ���
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strncmp ��)
#include <unistd.h>     // ���н� ǥ�� �Լ� ��� (fork, pipe, close ��)
#include <stdlib.h>     // ǥ�� ���̺귯�� �Լ� ��� (exit ��)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // ���� �Լ� ��� (socket, connect, send, recv ��)
#include <arpa/inet.h>  // ���ͳ� �������� ���� �Լ� (inet_pton ��)

// Signal setting
static void sigHandler(int);    // �ñ׳� ó���� �ڵ鷯

// Pipe setting
int child_to_parent[2];         // �������� �������� ����

// Buffer setting
char buffer[BUFSIZ];            // �������� ���� ����

// TCP Socket setting
#define TCP_PORT 5100   // ������ ������ ��Ʈ ��ȣ
int ssock;       // ���� ��ũ����


int main( int argc, char **argv) {

    // Process setting
    pid_t pid;  // ���μ��� ID
    int status; // ���� �ڵ�

    // Buffer setting
    int readBufferByte;

    // TCP Socket setting
    struct sockaddr_in servaddr; // ���� �ּ� ���� ����ü


    //----------------------------------------------
    // ���α׷� ���� �� IP �ּҰ� �ԷµǾ����� Ȯ��
    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]); // ���� ���
        return -1;
    }

    // �ڽ� -> �θ�� ������ ������ ����
    if (pipe(child_to_parent) < 0) {
        perror("pipe()");
        return -1;
    }

    // �ñ׳� ó���� ���� �ڵ鷯 ���
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }

    // ���� ���� (AF_INET: IPv4, SOCK_STREAM: TCP ���, 0: �������� �ڵ� ����)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // ���� ���� ���� �� ���� ���
        return -1;
    }

    // ���� �ּ� ����ü �ʱ�ȭ �� ����
    memset(&servaddr, 0, sizeof(servaddr)); // ����ü �ʱ�ȭ
    servaddr.sin_family = AF_INET;          // IPv4 �ּ� ü�� ����
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr)); // IP �ּ� ��ȯ
    servaddr.sin_port = htons(TCP_PORT);    // ��Ʈ ��ȣ ����

    // ������ ���� �õ�
    if (connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect() :");   // ���� ���� �� ���� ���
        return -1;
    }




    //----------------------------------------------
    // �θ� ���μ����� �ڽ� ���μ����� �б�
    if((pid = fork()) < 0)
    {
      perror("fork()"); // ��ũ ���� �� ���� ���
      return -1;
    }
    else if(pid == 0) // // �ڽ� ���μ��� ( Ű����κ��� �޽����� �޴� ����)
    {
        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);
        // �ڽ� ���� ����
        close(ssock);                       // ���� ����

        do
        {
            memset(buffer, 0, BUFSIZ);  // buffer �ʱ�ȭ

            // Ű���� �Է� �ޱ�
            if((readBufferByte=read(0, buffer, sizeof(buffer))<0))
            {
                perror(" stdin read()");
            }

            // �ڽ� -> �θ� ���� ������
            if(write(child_to_parent[1],buffer,strlen(buffer))<=0)
            {
                perror("child_to_parent pipe write()");
            }

            // �θ𿡰� SIGUSR1 �ñ׳� ������
            kill(getppid(), SIGUSR1);


        } while (strncmp(buffer,"/q",2));    // �α��� �α׾ƿ� flag �ʿ�


        printf("child process terminating....\n");      // �ڽ� ���μ��� ���� �޽��� ���
        close(child_to_parent[1]);          // �ڽ� -> �θ� ���� ������ ����
        exit(0);                            // �ڽ� ���μ��� ����

    } else // �θ� ���μ��� (�����κ��� �޽����� �ް� ȭ�� ���)
    {
        // �ڽ� -> �θ� ���� ������ �ݱ�
        close(child_to_parent[1]);

        do
        {
            memset(buffer, 0, BUFSIZ); // ���� �ʱ�ȭ

            // server�� ����� soket�� ���� ���
            if ((readBufferByte = recv(ssock, buffer, BUFSIZ, 0)) < 0) {
                perror("parent soket read() :");   // ���� ���� �� ���� ���
                return -1;
            }
            // ���� ���� ������ ��� printf
            if((write(1, buffer, strlen(buffer))<0))
            {
                perror(" stdout write()");
            }

        } while (strncmp(buffer,"/q",2));

        printf("parent process terminating...\n");      // �ڽ� ���μ��� ���� �޽��� ���
        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);
        // �θ� ���� ����
        close(ssock);
        // �ڽ� ���μ��� ���� ���
        waitpid(pid, &status, 0);
    }

    return 0;
}

// SIGUSR1 �ñ׳� �ڵ鷯
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // ���������� ������ �б�
        memset(buffer, 0, BUFSIZ);
        if (read(child_to_parent[0], buffer, BUFSIZ) > 0) {
            // ������ �޽��� ���� (������ ���� ����)
            if (write(ssock, buffer, strlen(buffer)) <= 0) {
                perror("parent soket write()");    // ���� ���� �� ���� ���
            }
        } else {
            perror("pipe read()");
        }
    }
}