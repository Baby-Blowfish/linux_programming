#include <stdio.h>      // ǥ�� ����� �Լ� ���
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strncmp ��)
#include <unistd.h>     // ���н� ǥ�� �Լ� ��� (fork, pipe, close ��)
#include <stdlib.h>     // ǥ�� ���̺귯�� �Լ� ��� (exit ��)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // ���� �Լ� ��� (socket, connect, send, recv ��)
#include <arpa/inet.h>  // ���ͳ� �������� ���� �Լ� (inet_pton ��)



// Signal setting
static void sigHandlerParent(int);    // ���� �� signal �ڵ鷯 (�θ� ���μ����� ó��)
static void sigHandlerChild(int);     // Ŭ���̾�Ʈ �� signal �ڵ鷯 (�ڽ� ���μ����� ó��)

// TCP Socket setting
#define TCP_PORT 5100   // ������ ������ ��Ʈ ��ȣ
int ssock;       // ���� ��ũ����

// Ŭ���̾�Ʈ ���� ����ü ����
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

typedef struct {
    int action; 
    // 1: ȸ������, 2: �α��� ��û, 3: �α׾ƿ� ��û, 4: ä������
    // 5 : ������ �α��ε� ������ ����, 6 : ��й�ȣ Ʋ�� 7 :�α��� ����
    // 9 : ä�� on, 10 : logout, chat off
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char mesg[BUFSIZ];              // Ŭ���̾�Ʈ �޽��� ���� (Ŭ���̾�Ʈ���� ���ŵ� �����͸� ����)
    int child_to_parent[2];         // �ڽ� -> �θ�� �����͸� �����ϴ� ������
    int parent_to_child[2];         // �θ� -> �ڽ����� �����͸� �����ϴ� ������
    pid_t pid;                      // �ڽ� ���μ��� ID (Ŭ���̾�Ʈ ������ ó���ϴ� �ڽ� ���μ��� ID)

    //----- Ŭ���̾�Ʈ������ �Ʒ��� ���� ������� ����!
    int csock;                      // Ŭ���̾�Ʈ ���� ��ũ���� (Ŭ���̾�Ʈ�� ������ ����ϴ� ����)
    int client_id;                  // Ŭ���̾�Ʈ ���� ID (�������� �����ϱ� ���� ID)
} __attribute__((packed, aligned(4))) Client;  // 4����Ʈ�� ����

Client client;  // ���������� ����



//  �Լ� ����
void showMenu();
void loginrequest(int action);
void logoutrequest(int action);
int chatOn_flag;    //  1 : ä�� ����       , 0 : ä�� ����
int childFlag = 1;      //  1 : �ڽ����μ��� ����, 0 : �ڽ����μ��� ����

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

    // Signal ���� (���� ������ SIGUSR1, Ŭ���̾�Ʈ ������ SIGUSR2 ��ȣ�� �޵��� ����)
    if (signal(SIGUSR1, sigHandlerParent) == SIG_ERR) {    // ������ SIGUSR1 ��ȣ�� ���� �� ó��
        perror("signal");
        return -1;
    }
    if (signal(SIGUSR2, sigHandlerChild) == SIG_ERR) {    // Ŭ���̾�Ʈ�� SIGUSR2 ��ȣ�� ���� �� ó��
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


// 7. �θ�-�ڽ� �� ����� ���� ������ ���� (�θ�->�ڽ�, �ڽ�->�θ� �� ����� ���� ������)
    if (pipe(client.parent_to_child) < 0 || pipe(client.child_to_parent) < 0) {
        perror("pipe()");
        close(ssock);  // ���� �ݱ�
        return -1;
    }


    //----------------------------------------------
    // �θ� ���μ����� �ڽ� ���μ����� �б�
    if((pid = fork()) < 0)
    {
        perror("fork()"); // ��ũ ���� �� ���� ���
        close(ssock);  // ���� �ݱ�
        close(client.parent_to_child[0]);  // ������ �ݱ�
        close(client.parent_to_child[1]);
        close(client.child_to_parent[0]);
        close(client.child_to_parent[1]);
        return -1;
    }
    else if(pid == 0) // �ڽ� ���μ���: �޴� �Է��� �ް� �̸� �θ𿡰� ����
    {
        close(client.parent_to_child[1]);  // �θ�->�ڽ� ���� ������ �ݱ� (�ڽĿ��� ���� ����)
        close(client.child_to_parent[0]);  // �ڽ�->�θ� �б� ������ �ݱ� (�ڽĿ��� ���� ����)
        close(ssock);                       // ���� ����
        client.pid = getppid();  // �θ� ���μ����� PID�� ����

       int choice;

        do
        {
            if(chatOn_flag == 1)    // ä���� �������� ���
            {
                // Ű���� �Է� �ޱ�
                if((readBufferByte=read(0, client.mesg, sizeof(BUFSIZ))<0))
                {
                    perror(" stdin read()");
                }

                if(!strncmp(client.mesg,"/q",2))    // ä�� ����
                {
                    chatOn_flag == 0;
                    client.action = 4;
                }
                // �ڽ� -> �θ� ���� ������
                if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
                {
                    perror("1 child_to_parent pipe write()");
                }

                // �θ𿡰� SIGUSR1 �ñ׳� ������
                kill(client.pid, SIGUSR1);

            }
            else
            {
                showMenu();  // �޴��� �����ֱ�

                if (scanf("%d", &choice) != 1) {  // �Է� ���� ó��
                    printf("Invalid input. Please enter a number.\n");
                    while (getchar() != '\n');  // �߸��� �Է��� ���� ��� ���۸� ���
                    continue;
                }

                switch (choice) {
                    // 1: ȸ������, 2: �α���, 3: �α׾ƿ�(���α׷� ����)
                    case 1:
                        //
                        break;
                    case 2:     // �α��� ��û
                        loginrequest(2);
                        break;
                    case 3:
                        logoutrequest(3);
                        childFlag = 0;
                        break;
                    default:
                        printf("Invalid option.\n");
                        break;
                }

            }

        } while (childFlag);    // �α��� �α׾ƿ� flag �ʿ�


        printf("child process terminating....\n");      // �ڽ� // �ڿ� ���� �� ���� (�������� ���� �ݱ�)
        close(client.parent_to_child[0]);
        close(client.child_to_parent[1]);
        close(ssock);
        exit(0);                            // �ڽ� ���μ��� ����

    } else // �θ� ���μ��� (�����κ��� �޽����� �ް� ȭ�� ���)
    {
        close(client.parent_to_child[0]);  // �θ�->�ڽ� �б� ������ �ݱ� (�θ𿡼� ���� ����)
        close(client.child_to_parent[1]);  // �ڽ�->�θ� ���� ������ �ݱ� (�θ𿡼� ���� ����)
        client.pid = pid;  // �ڽ� ���μ����� PID�� ����
        Client recvCli;

        do
        {
            // server�� ����� soket�� ���� ���
            if ((readBufferByte = recv(ssock, &recvCli, sizeof(Client), 0)) < 0) {
                perror("parent soket read() :");   // ���� ���� �� ���� ���
                return -1;
            }
            if((write(1, recvCli.mesg, strlen(recvCli.mesg))<0))
            {
                perror(" stdout write()");
            }

           write(client.parent_to_child[1],&recvCli,sizeof(Client));
            kill(client.pid, SIGUSR2);  // SIGUSR2 ��ȣ�� ���ŵ� ������ ����



        } while (strncmp(recvCli.mesg,"/q",2));

        printf("parent process terminating...\n");      // �ڽ� ���μ��� ���� �޽��� ���
        close(client.parent_to_child[1]);  // �θ�->�ڽ� ���� ������ �ݱ�
        close(client.child_to_parent[0]);  // �ڽ�->�θ� �б� ������ �ݱ�
        // �θ� ���� ����
        close(ssock);
        // �ڽ� ���μ��� ���� ���
        waitpid(pid, &status, 0);
    }

    return 0;
}

// (�θ� ���μ����� �ڽ� ���μ����� �޽����� ó���ϴ� �Լ�)
static void sigHandlerParent(int signo) {
    if (signo == SIGUSR1) {

        Client recvCli;

        // ���������� ������ �б�
        if (read(client.child_to_parent[0], &recvCli, sizeof(Client)) > 0) {
            // ������ �޽��� ���� (������ ���� ����)
            if (write(ssock, &recvCli, sizeof(Client)) <= 0) {
                perror("parent soket write()");    // ���� ���� �� ���� ���
            }
        } else {
            perror("pipe read()");
        }
    }
}

// (�ڽ� ���μ����� �θ� ���μ����� �޽����� ó���ϴ� �Լ�)
static void sigHandlerChild(int signo) {
    if (signo == SIGUSR2) {

        Client recvCli;

        // �θ𿡼� �ڽ����� ���޵� �޽��� �б�
        if (read(client.parent_to_child[0], &recvCli, sizeof(Client)) > 0) {  // �θ�->�ڽ� ���������� ������ �б�

            switch (recvCli.action)
            {
                // 1: ȸ������, 2: �α���, 3: �α׾ƿ�(Ŭ���̾�Ʈ ����), 4: ä�� ����
                case 5:
                case 6:     // �α��� ��û
                    chatOn_flag = 0;
                    break;
                case 7:
                    chatOn_flag = 1;
                    break;
                default:
                    printf("Invalid option.\n");
                    break;
            }
        } else {
            perror("pipe read()");  // �б� ���� �� ���� �޽��� ���
        }
    }
}

// �޴��� ����ϰ� ������� ���ÿ� ���� �Լ��� ����
void showMenu() {

    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("                  VEDA Talk                  \n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("  1. Register                                \n");
    printf("  2. Log In                                  \n");
    printf("  3. Log Out                                 \n");
    printf("  4. exit Chat                               \n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" What do you wanna do? ");

}


// ������ ������ ������ �Լ�
void loginrequest(int action) {

    client.action = action;

    printf("Enter username : ");
    scanf("%s", client.username);
    printf("Enter password : ");
    scanf("%s", client.password);

    // �ڽ� -> �θ� ���� ������
    if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
    {
        perror("2 child_to_parent pipe write()");
    }

    // �θ𿡰� SIGUSR1 �ñ׳� ������
    kill(client.pid, SIGUSR1);

    while (getchar() != '\n');  // '\n'�� ���� ������ �Է��� �����մϴ�

    // �޽����� ����մϴ�
    printf("If you go back or start chatting, please press Enter!\n");

    // ���� Ű�� ���� ������ ��ٸ��ϴ�
    getchar();  // ���Ͱ� ������ ����

}

// ������ ������ ������ �Լ�
void logoutrequest(int action) {

    client.action = action;

    strcpy(client.mesg,"/q");
    client.action = 3;
    // �ڽ� -> �θ� ���� ������
    if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
    {
        perror("3 child_to_parent pipe write()");
    }

    // �θ𿡰� SIGUSR1 �ñ׳� ������
    kill(client.pid, SIGUSR1);

    while (getchar() != '\n');  // '\n'�� ���� ������ �Է��� �����մϴ�

    // �޽����� ����մϴ�
    printf("If you want next action, please press Enter!\n");

    // ���� Ű�� ���� ������ ��ٸ��ϴ�
    getchar();  // ���Ͱ� ������ ����

}
