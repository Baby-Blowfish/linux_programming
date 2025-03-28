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

#define TCP_PORT 5100   // ����� TCP ��Ʈ ��ȣ
#define MAX_CLIENTS 8   // �ִ� Ŭ���̾�Ʈ ��

// Signal �ڵ鷯 ����
static void sigHandlerParent(int);    // ���� �� signal �ڵ鷯 (�θ� ���μ����� ó��)
static void sigHandlerChild(int);     // Ŭ���̾�Ʈ �� signal �ڵ鷯 (�ڽ� ���μ����� ó��)

// ���� ����
char mesg[BUFSIZ];            // �޽��� ����, Ŭ���̾�Ʈ���� �ۼ��ŵǴ� ������ �����

// TCP ���� ����
int ssock;              // ���� ���� ��ũ����
int client_count = 0;    // ���� ���ӵ� Ŭ���̾�Ʈ ��
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

// Ŭ���̾�Ʈ ���� ����ü ����
typedef struct {
    int action;  // 1: ȸ������, 2: �α���, 3: �α׾ƿ�, 4: ä������
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char mesg[BUFSIZ];              // Ŭ���̾�Ʈ �޽��� ���� (Ŭ���̾�Ʈ���� ���ŵ� �����͸� ����)
    int child_to_parent[2];         // �ڽ� -> �θ�� �����͸� �����ϴ� ������
    int parent_to_child[2];         // �θ� -> �ڽ����� �����͸� �����ϴ� ������
    pid_t pid;                      // �ڽ� ���μ��� ID (Ŭ���̾�Ʈ ������ ó���ϴ� �ڽ� ���μ��� ID)
    int csock;                      // Ŭ���̾�Ʈ ���� ��ũ���� (Ŭ���̾�Ʈ�� ������ ����ϴ� ����)
    int client_id;                  // Ŭ���̾�Ʈ ���� ID (�������� �����ϱ� ���� ID)

} __attribute__((packed, aligned(4))) Client;  // 4����Ʈ�� ����

Client* clients[MAX_CLIENTS];  // �������� Ŭ���̾�Ʈ�� �����ϱ� ���� ������ �迭

// Ŭ���̾�Ʈ ���� �Լ� ����
void remove_client(int index); // Ŭ���̾�Ʈ�� ����� �� Ŭ���̾�Ʈ�� ��Ͽ��� �����ϴ� �Լ�



// longinout ���� �Լ�
void login(int i);
void logout(int i);
void chat_on(int i);
int childFlag = 1;

int main(int argc, char **argv) {

    // ���μ��� �� TCP ���� ���� ���� ����
    pid_t pid;  // �ڽ� ���μ��� ID�� ������ ����
    int status; // �ڽ� ���μ��� ���� ����
    int readBufferByte; // ���� ������ ũ�⸦ ������ ����

    // TCP ���� �ּ� ���� ���� ����
    socklen_t clen;      // Ŭ���̾�Ʈ �ּ� ����ü ũ��
    struct sockaddr_in servaddr, cliaddr;  // ���� �� Ŭ���̾�Ʈ �ּ� ����ü

    // Signal ���� (���� ������ SIGUSR1, Ŭ���̾�Ʈ ������ SIGUSR2 ��ȣ�� �޵��� ����)
    if (signal(SIGUSR1, sigHandlerParent) == SIG_ERR) {    // ������ SIGUSR1 ��ȣ�� ���� �� ó��
        perror("signal");
        return -1;
    }
    if (signal(SIGUSR2, sigHandlerChild) == SIG_ERR) {    // Ŭ���̾�Ʈ�� SIGUSR2 ��ȣ�� ���� �� ó��
        perror("signal");
        return -1;
    }

    // 1. ���� ���� ����
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  // TCP ���� ���� (IPv4, TCP ���)
        perror("socket() :");
        return -1;
    }

    // 2. ���� ���� �ּ� ����
    memset(&servaddr, 0, sizeof(servaddr));     // ���� �ּ� �ʱ�ȭ (��� �ʵ带 0���� �ʱ�ȭ)
    servaddr.sin_family = AF_INET;              // IPv4 �������� ���
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� IP �ּҿ��� ������ ��� (0.0.0.0)
    servaddr.sin_port = htons(TCP_PORT);        // ������ ��Ʈ ��ȣ�� ��� (5100�� ��Ʈ)

    // 3. ���ϰ� �ּҸ� ���ε� (���� ������ ������ �ּҿ� ����)
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");
        return -1;
    }

    // 4. Ŭ���̾�Ʈ ���� ��� (�ִ� 8���� Ŭ���̾�Ʈ ���� ���)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");
        return -1;
    }

    clen = sizeof(cliaddr); // Ŭ���̾�Ʈ �ּ� ����ü�� ũ�⸦ ����

    do {
        // 5. Ŭ���̾�Ʈ ���� ���� (accept ȣ��� Ŭ���̾�Ʈ ������ ���)
        memset(&cliaddr, 0, clen);  // Ŭ���̾�Ʈ �ּҸ� �ʱ�ȭ
        int new_sock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);  // Ŭ���̾�Ʈ ���� ����
        if (new_sock < 0) {
            perror("accept() :");
            continue;  // ������ �߻��ϸ� ���� Ŭ���̾�Ʈ ������ ���
        }

        // Ŭ���̾�Ʈ�� �ִ� ���� �������� ��� ó�� (�� �̻� Ŭ���̾�Ʈ�� �������� ����)
        if (client_count >= MAX_CLIENTS) {
            printf("Maximum clients connected.\n");
            close(new_sock);  // Ŭ���̾�Ʈ ���� �ݱ�
            continue;  // �� �̻� ó������ ����
        }

        // 6. ���ο� Ŭ���̾�Ʈ ����ü ���� �� �ʱ�ȭ (���� �Ҵ�)
        clients[client_count] = (Client *)malloc(sizeof(Client));  // Ŭ���̾�Ʈ ����ü �޸� �Ҵ�
        if (clients[client_count] == NULL) {  // �޸� �Ҵ� ���� ó��
            perror("malloc() :");
            return -1;
        }

        clients[client_count]->csock = new_sock;  // Ŭ���̾�Ʈ ������ ����ü�� ����

        // Ŭ���̾�Ʈ�� IP �ּҸ� ���
        memset(mesg, 0, BUFSIZ);  // �޽��� ���� �ʱ�ȭ
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);  // Ŭ���̾�Ʈ IP �ּҸ� ���ڿ��� ��ȯ
        printf("Client Socket %d is connected: %s\n", client_count, mesg);  // ����� Ŭ���̾�Ʈ ���� ���

        // 7. �θ�-�ڽ� �� ����� ���� ������ ���� (�θ�->�ڽ�, �ڽ�->�θ� �� ����� ���� ������)
        if (pipe(clients[client_count]->parent_to_child) < 0 || pipe(clients[client_count]->child_to_parent) < 0) {
            perror("pipe()");
            close(new_sock);  // ���� �ݱ�
            free(clients[client_count]);  // �޸� ����
            continue;  // ������ �߻������Ƿ� ���� Ŭ���̾�Ʈ ó���� �Ѿ
        }

        // 8. �ڽ� ���μ��� ���� (fork ȣ��)
        if ((pid = fork()) < 0) {  // fork ���� ó��
            perror("fork()");
            close(new_sock);  // ���� �ݱ�
            free(clients[client_count]);  // �޸� ����
            close(clients[client_count]->parent_to_child[0]);  // ������ �ݱ�
            close(clients[client_count]->parent_to_child[1]);
            close(clients[client_count]->child_to_parent[0]);
            close(clients[client_count]->child_to_parent[1]);
            continue;  // ������ �߻������Ƿ� ���� Ŭ���̾�Ʈ ó���� �Ѿ
        }
        else if (pid == 0) {  // �ڽ� ���μ��� ó��

            close(clients[client_count]->parent_to_child[1]);  // �θ�->�ڽ� ���� ������ �ݱ� (�ڽĿ��� ���� ����)
            close(clients[client_count]->child_to_parent[0]);  // �ڽ�->�θ� �б� ������ �ݱ� (�ڽĿ��� ���� ����)
            close(ssock);  // �ڽ��� ���� ������ ������� �����Ƿ� �ݱ�
            clients[client_count]->pid = getppid();  // �θ� ���μ����� PID�� ����
            clients[client_count]->client_id = client_count;  // Ŭ���̾�Ʈ ID ����

            do {
                    Client recvCli;

                    // Ŭ���̾�Ʈ ���Ͽ��� �޽��� �б�
                    if ((readBufferByte = read(clients[client_count]->csock, &recvCli, BUFSIZ)) > 0) {

                        clients[client_count]->action = recvCli.action;
                        strcpy(clients[client_count]->username, recvCli.username);
                        strcpy(clients[client_count]->password, recvCli.password);
                        strcpy(clients[client_count]->mesg, recvCli.mesg);

                        printf("%d child socket received : %s", clients[client_count]->client_id, clients[client_count]->mesg);  // �ڽ� ���μ����� �޽����� �޾��� �� ���

                        // �θ𿡰� ����ü ���� ���� (�ڽ�->�θ� �������� ������ ����)
                        if (write(clients[client_count]->child_to_parent[1], clients[client_count], sizeof(Client)) < 0) {
                            perror("child_to_parent pipe 1 write()");
                        }

                        // �θ𿡰� SIGUSR1 ��ȣ ������ (�θ� ���μ����� �����͸� ó���� �� �ֵ��� ��ȣ ����)
                        kill(clients[client_count]->pid, SIGUSR1);

                    }
                    else{
                        perror("Client csock read() :");
                        break;
                    }
                } while (childFlag);  // "/q" �Է� �� ���� (���� ��ȣ ó��)

            printf("Child %d process terminating\n", client_count);  // �ڽ� ���μ��� ���� �޽��� ���

            // �ڿ� ���� �� ���� (�������� ���� �ݱ�)
            close(clients[client_count]->parent_to_child[0]);
            close(clients[client_count]->child_to_parent[1]);
            close(clients[client_count]->csock);
            exit(0);  // �ڽ� ���μ��� ����

        } else {  // �θ� ���μ��� ó��
            close(clients[client_count]->parent_to_child[0]);  // �θ�->�ڽ� �б� ������ �ݱ� (�θ𿡼� ���� ����)
            close(clients[client_count]->child_to_parent[1]);  // �ڽ�->�θ� ���� ������ �ݱ� (�θ𿡼� ���� ����)

            clients[client_count]->pid = pid;  // �ڽ� ���μ��� ID ����
            client_count++;  // Ŭ���̾�Ʈ �� ���� (���ο� Ŭ���̾�Ʈ �߰�)
        }

    } while (1);  // ������ ��� ���� (���� ����)

    // ���� ���� ó�� (������ ����Ǿ��� ��)
    printf("Parent process terminating\n");

    for (int i = 0; i < client_count; i++) {  // ��� Ŭ���̾�Ʈ�� �ڿ� ����
        close(clients[i]->parent_to_child[1]);  // �θ�->�ڽ� ���� ������ �ݱ�
        close(clients[i]->child_to_parent[0]);  // �ڽ�->�θ� �б� ������ �ݱ�
    }
    close(ssock);   // ���� ���� �ݱ�
    return 0;  // ���α׷� ����
}

// ���� �� SIGUSR1 ó�� �Լ� (�θ� ���μ����� �ڽ� ���μ����� �޽����� ó���ϴ� �Լ�)
static void sigHandlerParent(int signo) {
    if (signo == SIGUSR1) {  // SIGUSR1 ��ȣ�� ���� ���
        for (int i = 0; i < client_count; i++) {  // ���� ����� Ŭ���̾�Ʈ �� ��ȣ�� ���� Ŭ���̾�Ʈ�� ã��

            // ���ÿ� ����ü ���� (������ ��� ����ü�� ���� �����Ͽ� �����͸� ����)
            Client recvCli;

            // ���������� ������ �б� (����ŷ ���� ����)
            int flags = fcntl(clients[i]->child_to_parent[0], F_GETFL, 0);
            fcntl(clients[i]->child_to_parent[0], F_SETFL, flags | O_NONBLOCK);

            // ����ŷ read: ���������� ������ �б�
            ssize_t n = read(clients[i]->child_to_parent[0], &recvCli, sizeof(Client));

            if (n < 0) {  // �б� ���� ó��
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // �������� ���� �����Ͱ� ������ �ǹ� (�񵿱� �۾������� ���� ��Ȳ)
                } else {
                    // �� ���� read ���� ó��
                }
            } else if (n == 0) {  // EOF: �������� ���� (�� �̻� �����Ͱ� ����)
                    printf("Pipe closed (EOF).\n");
            } else {  // ���������� �����͸� ���� ���
                printf("Parent read Data from %d Client pipe: %s", recvCli.client_id, recvCli.mesg);  // �θ� �ڽ����κ��� ���� �޽��� ���

                // ������ ������Ʈ
                clients[i]->action = recvCli.action;
                strcpy(clients[i]->username, recvCli.username);
                strcpy(clients[i]->password, recvCli.password);
                strcpy(clients[i]->mesg, recvCli.mesg);

                // 1: ȸ������, 2: �α��� ��û, 3: �α׾ƿ� ��û, 4: ä�� off ��û
                // 5 : ������ �α��ε� ������ ����, 6 : ��й�ȣ Ʋ�� 7 :�α��� ����
                // 9 : ä�� on, 10 : logout, chat off
                switch (clients[i]->action) {
                    // 1: ȸ������, 2: �α���, 3: �α׾ƿ�, 4: ä������
                    case 1:
                        //recvNamepw(1);
                        break;
                    case 2:     // �α��� ��û
                        login(i);
                        break;
                    case 3:     // �α� �ƿ� ��û
                        logout(i);
                        printf("Exiting chat.\n");
                        childFlag = 0;
                        break;
                    case 5:
                        chat_on(i);
                    default:
                        printf("Invalid option.\n");
                        break;
                }
                
            }
        }
    }
}

// �ڽ� �� SIGUSR2 ó�� �Լ� (�θ� ���μ����� �ڽ� ���μ������� �����ϴ� ��ȣ�� ó��)
static void sigHandlerChild(int signo) {
    if (signo == SIGUSR2) {  // SIGUSR2 ��ȣ�� ���� ���

        // ���ÿ� ����ü ���� (������ ��� ����ü�� ���� �����Ͽ� �����͸� ����)
        Client recvCli;

        // �θ𿡼� �ڽ����� ���޵� �޽��� �б�
        if (read(clients[client_count]->parent_to_child[0], &recvCli, sizeof(Client)) > 0) {  // �θ�->�ڽ� ���������� ������ �б�
            if (recvCli.client_id < clients[client_count]->client_id) {  // Ŭ���̾�Ʈ ID�� ����Ǿ�����
                clients[client_count]->client_id--;  // Ŭ���̾�Ʈ ID�� ���ҽ�Ŵ
            }
        } else {
            perror("pipe read()");  // �б� ���� �� ���� �޽��� ���
        }
    }
}

// Ŭ���̾�Ʈ ��Ͽ��� ����� Ŭ���̾�Ʈ�� �����ϴ� �Լ�
void remove_client(int index) {
    if (clients[index] != NULL) {
        close(clients[index]->csock);                   // Ŭ���̾�Ʈ�� ����� ���� �ݱ�
        close(clients[index]->child_to_parent[0]);      // �ڽ�->�θ� ������ �ݱ�
        close(clients[index]->parent_to_child[1]);      // �θ�->�ڽ� ������ �ݱ�

        printf("Child process %d removed\n", index);  // Ŭ���̾�Ʈ ���� �޽��� ���

        free(clients[index]);  // �������� �Ҵ�� Ŭ���̾�Ʈ �޸� ����
        clients[index] = NULL;  // ������ �ʱ�ȭ

        client_count--;  // �� Ŭ���̾�Ʈ �� ����

        // Ŭ���̾�Ʈ ����� ��ĭ�� ���� (Ŭ���̾�Ʈ ��Ͽ��� ���ڸ��� ����)
        for (int i = index; i < client_count; i++) {
            clients[i] = clients[i + 1];
        }

        clients[client_count] = NULL;  // ������ Ŭ���̾�Ʈ �����͸� �ʱ�ȭ
    }
}

// �α��� �Լ�
void login(int i) {

    for(int x =0; x < client_count; x++)
    {
        if(x != i)  // �ش� Ŭ���̾�Ʈ ���� �ٸ� Ŭ���̾�Ʈ�� ��
        {
            // ������ Ŭ���̾�Ʈ �̸��� ��ϵǾ� �ִ� ���
            if(!strcmp(clients[x]->username,clients[i]->username))
            {
                clients[i]->action = 5;     //  ������ ������ ������ ��Ÿ��
                break;
            }
            else if(!strcmp(clients[x]->password,clients[i]->password))
            {
                clients[i]->action = 7;    // �α��� �Ǿ����� ��Ÿ��
                break;
            }
            else
            {
                clients[i]->action = 6;    // ��й�ȣ Ʋ��
                break;
            }
        }
    }

    if(clients[i]->action == 5)         //  ������ ������ ������ ��Ÿ��
    {
        sprintf(clients[i]->mesg, "%d Already logged in \n", i);
        if (write(clients[i]->csock, clients[i], sizeof(Client)) > 0) {  // �޽��� ���� ����
        printf("Parent send Data to %d Client pipe: success\n", i);
        }
        else {  // �޽��� ���� ���� ó��
            printf("Parent send Data to %d Client pipe: fail\n", i);
            perror("write()");
        }
    }
    else if(clients[i]->action == 6)    // ��й�ȣ Ʋ��
    {
        sprintf(clients[i]->mesg, "%d Invalid username or password. ",i);
        if (write(clients[i]->csock, clients[i], sizeof(Client)) > 0) {  // �޽��� ���� ����
        printf("Parent send Data to %d Client pipe: success\n", i);
        }
        else {  // �޽��� ���� ���� ó��
            printf("Parent send Data to %d Client pipe: fail\n", i);
            perror("write()");
        }
    }
    else if(clients[i]->action == 7)    // �α��� �Ǿ����� ��Ÿ��
    {
        // �ٸ� Ŭ���̾�Ʈ���� �޽����� ��ε�ĳ��Ʈ (��� Ŭ���̾�Ʈ���� ����)
        sprintf(clients[i]->mesg, "%d is chat on ", i);
        printf("%d is login",i);
        for (int j = 0; j < client_count; j++) {

            if (write(clients[j]->csock, clients[i], sizeof(Client)) > 0) {  // �޽��� ���� ����
                printf("Parent send Data to %d Client pipe: success\n", j);
            }
            else {  // �޽��� ���� ���� ó��
                printf("Parent send Data to %d Client pipe: fail\n", j);
                perror("write()");
            }

        }
    }

}

// �α׾ƿ� �Լ�
void logout(int i)
{

    if (!strncmp(clients[i]->mesg, "/q", 2))   // "/q" �޽����� ��� Ŭ���̾�Ʈ ���� ó��

    // ������� �˸� (�ٸ� Ŭ���̾�Ʈ���� �˸�)
    for (int j = 0; j < client_count; j++) {
        if (j != i) {  // �ٸ� Ŭ���̾�Ʈ���Ը� �˸�
            sprintf(clients[i]->mesg, "%d is terminating... \n", i);
            write(clients[j]->csock, clients[i], sizeof(Client));  // �޽����� �ٸ� Ŭ���̾�Ʈ�� ����

            write(clients[j]->parent_to_child[1], clients[i], sizeof(Client));  // �ٸ� �ڽ� ���μ������� ���Ḧ �˸�

            if (j > i)  // ����� �ڽ� ���μ������� �ڿ� ������ ���μ������� Ŭ���̾�Ʈ ID ����
                clients[j]->client_id--;
            kill(clients[j]->pid, SIGUSR2);  // SIGUSR2 ��ȣ�� ���ŵ� ������ ����
        }
    }

    write(clients[i]->csock, clients[i], sizeof(Client));  // ����� Ŭ���̾�Ʈ�� ���� �޽��� ����
    remove_client(i);  // Ŭ���̾�Ʈ�� ��Ͽ��� ����
}

void chat_on(int i)
{
    // �ٸ� Ŭ���̾�Ʈ���� �޽����� ��ε�ĳ��Ʈ (��� Ŭ���̾�Ʈ���� ����)
    for (int j = 0; j < client_count; j++) {
        if (j != i) {  // �ڱ� �ڽ��� ������ Ŭ���̾�Ʈ�鿡�� ����
            if (write(clients[j]->csock, clients[i], strlen(clients[i]->mesg)) > 0) {  // �޽��� ���� ����
                printf("Parent send Data to %d Client pipe: success\n", j);
            }
            else {  // �޽��� ���� ���� ó��
                printf("Parent send Data to %d Client pipe: fail\n", j);
                perror("write()");
            }
        }
    }
}
