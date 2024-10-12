#include <stdio.h>      // ǥ�� ����� �Լ� ���
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strncmp ��)
#include <unistd.h>     // ���н� �ý��� ȣ�� �Լ� ��� (fork, pipe, read, write ��)
#include <stdlib.h>     // ǥ�� ���̺귯�� �Լ� ��� (exit ��)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // ���� �Լ� ��� (socket, bind, listen, accept ��)
#include <arpa/inet.h>  // ���ͳ� �������� ���� �Լ� ��� (inet_ntop ��)
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100   // ������ ����� TCP ��Ʈ ����
#define MAX_CLIENTS 8   // �ִ� Ŭ���̾�Ʈ ��

// Signal setting
static void sigHandler(int);    // �ñ׳� ó���� �ڵ鷯

// Buffer setting
char mesg[BUFSIZ];            // �������� ���� ����

// TCP Socket setting
int ssock;              // ���� ���� ��ũ����
int csock;

int child_to_parent[2];         // �ڽ� -> �θ�� �����͸� �����ϴ� ������
int parent_to_child[2];         // �θ� -> �ڽ����� �����͸� �����ϴ� ������

int main(int argc, char **argv)
{
    // Process setting
    pid_t pid;  // ���μ��� ID
    int status; // ���� �ڵ�

     // Buffer setting
    int readBufferByte;

    // TCP Socket Setting
    socklen_t clen;      // Ŭ���̾�Ʈ �ּ� ����ü ũ��
    struct sockaddr_in servaddr, cliaddr;  // ���� �� Ŭ���̾�Ʈ �ּ� ����ü





//------------------------------------------------------------------------------------------------------

    // �ñ׳� ó���� ���� �ڵ鷯 ���
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }
    // 1. ���� ���� ���� (AF_INET: IPv4, SOCK_STREAM: TCP, 0: �������� �ڵ� ����)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // ���� ���� ���� �� ���� �޽��� ���
        return -1;
    }

    // 2. ���� �ּ� ���� �� �ʱ�ȭ
    memset(&servaddr, 0, sizeof(servaddr));     // ���� �ּ� ����ü �ʱ�ȭ
    servaddr.sin_family = AF_INET;              // IPv4 �ּ� ü�� ����
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� IP �ּҿ��� ���� ���
    servaddr.sin_port = htons(TCP_PORT);        // ���� ��Ʈ ��ȣ ����

    // 3. ���� ������ ������ �ּҿ� ��Ʈ�� ���ε�
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");       // ���ε� ���� �� ���� �޽��� ���
        return -1;
    }

    // 4. ���� ��� (��⿭ �ִ� ũ��: 8)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");    // ��� ���� ��ȯ ���� �� ���� �޽��� ���
        return -1;
    }

    clen = sizeof(cliaddr); // Ŭ���̾�Ʈ �ּ� ����ü ũ�� �ʱ�ȭ




//----------------------------------------------------------------------------------------------------
    // ������ Ŭ���̾�Ʈ ������ �ݺ������� ó��
    do
    {
        // 5. Ŭ���̾�Ʈ ���� ���� (accept)
        memset(&cliaddr, 0, clen); // ����ü �ʱ�ȭ
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (csock < 0) {
            perror("accept() :"); // ���� ���� ���� �� ���� ���
            continue; // ���� �߻� �� ���� Ŭ���̾�Ʈ ���� ó���� ����
        }

        // 6. Ŭ���̾�Ʈ�� IP �ּҸ� ���ڿ� �������� ��ȯ�Ͽ� ���
        memset(mesg, 0, BUFSIZ);  // �޽��� ���� �ʱ�ȭ
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ); // Ŭ���̾�Ʈ IP ���
        printf("Client %d is connected: %s\n",csock, mesg);

        // �ڽ� <-> �θ� ������ ����
        if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
            perror("pipe()");
            return -1;
        }

        // 7. ���μ��� ��ũ (�ڽ� ���μ��� ����)
        if ((pid = fork()) < 0) {
            perror("fork()");     // ���μ��� ���� ���� �� ���� �޽��� ���
            return -1;
        }
        else if (pid == 0) // �ڽ� ���μ��� (Ŭ���̾�Ʈ�κ��� �޽��� ����)
        {
            // �θ� -> �ڽ� ���� ������ �ݱ� (�ڽ��� ���ʿ��� ���� ����)
            close(parent_to_child[1]);
            // �ڽ� -> �θ� �б� ������ �ݱ� (�ڽ��� ���ʿ��� ���� ����)
            close(child_to_parent[0]);
            // �θ� ���� ����
            close(ssock);


            do {
                memset(mesg, 0, BUFSIZ);  // �޽��� ���� �ʱ�ȭ

                // Ŭ���̾�Ʈ�κ��� ������ ���� (read)
                if ((readBufferByte = read(csock, mesg, BUFSIZ)) <= 0) {
                    perror("csock read() :");   // ���� ���� �� ���� �޽��� ���
                    break;
                }

                printf("chiled Receved : %s",mesg);

                // �ڽ� -> �θ� �������� ������ ����
                if (write(child_to_parent[1], mesg, strlen(mesg)) < 0) {
                    perror("child_to_parent pipe write()"); // ���� ���� �� ���� �޽��� ���
                }

                // �θ� ���μ������� SIGUSR1 �ñ׳��� ����
                kill(getppid(), SIGUSR1);

            } while (1); // "q" �Ǵ� "w" ���� �� ���� ����

            printf("Child %d process terminating\n",csock);
            // �θ� -> �ڽ� �б� ������ ����
            close(parent_to_child[0]);
            // �ڽ� -> �θ� ���� ������ ����
            close(child_to_parent[1]);
            close(csock);  // Ŭ���̾�Ʈ ���� �ݱ�
            exit(0);       // �ڽ� ���μ��� ����
        }
        else
        // �θ� -> �ڽ� �б� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(child_to_parent[1]);



    } while(1);

    printf("Parent process terminating\n");

        // �θ� -> �ڽ� ���� ������ ����
        close(parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ ����
        close(child_to_parent[0]);

    // ���� ���� �ݱ�
    close(ssock);

    return 0;
}

#include <fcntl.h>
#include <errno.h>

// SIGUSR1 �ñ׳� �ڵ鷯
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        //printf("recevied Signal\n");
        // �ڽ� -> �θ� �б� ���������� �ε����� �޽��� �б�
        memset(mesg, 0, BUFSIZ);  // ���� �ʱ�ȭ

        if ((read(child_to_parent[0], mesg, sizeof(mesg))) > 0)
        {
            if (send(csock, mesg, BUFSIZ, MSG_DONTWAIT) < 0)
            {
                perror("parent socket send()");    // ���� ���� �� ���� ���
            }
        }
        else perror("parent pipe read()");    // ���� ���� �� ���� ���
    }
}

