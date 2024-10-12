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
int client_count = 0;    // ����� Ŭ���̾�Ʈ ��

typedef struct
{
    int csock;                      // ���� ��ũ����
    pid_t pid;
    int mesgflag;
    int child_to_parent[2];         // �ڽ� -> �θ�� �����͸� �����ϴ� ������
    int parent_to_child[2];         // �θ� -> �ڽ����� �����͸� �����ϴ� ������
    char *id;
    char *pw;
    char mesg[BUFSIZ];
} Client;

Client clients[MAX_CLIENTS];

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
        clients[client_count].csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (clients[client_count].csock < 0) {
            perror("accept() :"); // ���� ���� ���� �� ���� ���
            continue; // ���� �߻� �� ���� Ŭ���̾�Ʈ ���� ó���� ����
        }

        // 6. Ŭ���̾�Ʈ�� IP �ּҸ� ���ڿ� �������� ��ȯ�Ͽ� ���
        memset(mesg, 0, BUFSIZ);  // �޽��� ���� �ʱ�ȭ
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ); // Ŭ���̾�Ʈ IP ���
        printf("Client %d is connected: %s\n",client_count, mesg);

        // �ڽ� <-> �θ� ������ ����
        if (pipe(clients[client_count].parent_to_child) < 0 || pipe(clients[client_count].child_to_parent) < 0) {
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
            close(clients[client_count].parent_to_child[1]);
            // �ڽ� -> �θ� �б� ������ �ݱ� (�ڽ��� ���ʿ��� ���� ����)
            close(clients[client_count].child_to_parent[0]);
            // �θ� ���� ����
            close(ssock);

            clients[client_count].pid = getppid();        // �ڽ����μ����� �θ��� pid ����

            do {

                memset(clients[client_count].mesg, 0, BUFSIZ);  // �޽��� ���� �ʱ�ȭ

                // Ŭ���̾�Ʈ�κ��� ������ ���� (read)
                if ((readBufferByte = read(clients[client_count].csock, clients[client_count].mesg, BUFSIZ)) <= 0) {
                    perror("csock read() :");   // ���� ���� �� ���� �޽��� ���
                    break;
                }

                printf("%d chiled Receved : %s",client_count,clients[client_count].mesg);

                clients[client_count].mesgflag = 1;             // 1 : ��ȿ�� �޽���.

                // �ڽ� -> �θ� �������� ������ ����
                if (write(clients[client_count].child_to_parent[1], &clients[client_count], sizeof(clients[client_count])) < 0) {
                    perror("child_to_parent pipe write()"); // ���� ���� �� ���� �޽��� ���
                }

                    // �θ� ���μ������� SIGUSR1 �ñ׳��� ����
                kill(clients[client_count].pid, SIGUSR1);


            } while (1); // "q" �Ǵ� "w" ���� �� ���� ����

            printf("Child %d process terminating\n",client_count);
            // �θ� -> �ڽ� �б� ������ ����
            close(clients[client_count].parent_to_child[0]);
            // �ڽ� -> �θ� ���� ������ ����
            close(clients[client_count].child_to_parent[1]);
            close(clients[client_count].csock);  // Ŭ���̾�Ʈ ���� �ݱ�
            exit(0);       // �ڽ� ���μ��� ����
        }
        else
        // �θ� -> �ڽ� �б� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(clients[client_count].parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(clients[client_count].child_to_parent[1]);

        clients[client_count].pid = pid;   // �θ����μ����� �ڽ��� pid ����

        client_count++;

    } while((client_count) < MAX_CLIENTS);

    printf("Parent process terminating\n");

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        // �θ� -> �ڽ� ���� ������ ����
        close(clients[i].parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ ����
        close(clients[i].child_to_parent[0]);
    }

    // ���� ���� �ݱ�
    close(ssock);

    return 0;
}



// SIGUSR1 �ñ׳� �ڵ鷯
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {

        for (int i = 0; i < client_count; i++) {     // � �ڽ� -> �θ� �б� ���������� �Դ��� Ȯ��

            memset(clients[i].mesg, 0, BUFSIZ);  // ���� �ʱ�ȭ
            clients[i].mesgflag = 0;             // mesgflag �ʱ�ȭ

            // Nonblock ó��
            int flags = fcntl(clients[i].child_to_parent[0], F_GETFL, 0);
            fcntl(clients[i].child_to_parent[0], F_SETFL, flags | O_NONBLOCK);

            // ����ŷ���� read() ȣ��
            ssize_t n = read(clients[i].child_to_parent[0], &clients[i], sizeof(clients[i]));

            if(clients[i].mesgflag > 0) // �������� ���� �����Ͱ� ��ȿ�� �޽������
            {

                for (int j = 0; j < client_count; j++)  // ���� Ŭ���̾�Ʈ ���� �ٸ� Ŭ���̾�Ʈ���� �޽����� �����ϱ�
                {

                    if(j != i)
                    {
                        write(clients[j].csock, clients[i].mesg, strlen(clients[i].mesg));
                        printf(" send to %d soket\n",j);
                    }

                }
            }

        }

    }
}

