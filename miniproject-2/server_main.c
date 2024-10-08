
#include "server_functions.h"

int main(int argc, char **argv) {

    // ���μ��� �� TCP ���� ���� ���� ����
    pid_t pid;  // �ڽ� ���μ��� ID�� ������ ����
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
                memset(clients[client_count]->mesg, 0, BUFSIZ); // �޽��� ���� �ʱ�ȭ

                // Ŭ���̾�Ʈ ���Ͽ��� �޽��� �б�
                if ((readBufferByte = read(clients[client_count]->csock, clients[client_count]->mesg, BUFSIZ)) <= 0) {
                    perror("Client csock read() :");
                    break;
                }

                printf("%d child socket received : %s", clients[client_count]->client_id, clients[client_count]->mesg);  // �ڽ� ���μ����� �޽����� �޾��� �� ���

                // �θ𿡰� �޽��� ���� (�ڽ�->�θ� �������� ������ ����)
                if (write(clients[client_count]->child_to_parent[1], clients[client_count], sizeof(Client)) < 0) {
                    perror("child_to_parent pipe 1 write()");
                }

                // �θ𿡰� SIGUSR1 ��ȣ ������ (�θ� ���μ����� �����͸� ó���� �� �ֵ��� ��ȣ ����)
                kill(clients[client_count]->pid, SIGUSR1);

            } while (strncmp(clients[client_count]->mesg,"/q",2));  // "/q" �Է� �� ���� (���� ��ȣ ó��)

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


