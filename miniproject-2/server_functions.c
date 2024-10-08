#include "server_functions.h"

int ssock;              // ���� ���� ��ũ����
int client_count = 0;    // ���� ���ӵ� Ŭ���̾�Ʈ ��
Client* clients[MAX_CLIENTS];

// ���� �� SIGUSR1 ó�� �Լ� (�θ� ���μ����� �ڽ� ���μ����� �޽����� ó���ϴ� �Լ�)
void sigHandlerParent(int signo) {
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

                if (!strncmp(recvCli.mesg, "/q", 2)) {  // "/q" �޽����� ��� Ŭ���̾�Ʈ ���� ó��

                    // ������� �˸� (�ٸ� Ŭ���̾�Ʈ���� �˸�)
                    for (int j = 0; j < client_count; j++) {
                        if (j != i) {  // �ٸ� Ŭ���̾�Ʈ���Ը� �˸�
                            char off[] = "  is terminating...\n";
                            off[0] = '0' + i;  // ����� Ŭ���̾�Ʈ ��ȣ�� �˸�
                            write(clients[j]->csock, off, strlen(off));  // �޽����� �ٸ� Ŭ���̾�Ʈ�� ����

                            write(clients[j]->parent_to_child[1], &recvCli, sizeof(Client));  // �ٸ� �ڽ� ���μ������� ���Ḧ �˸�

                            if (j > i)  // ����� �ڽ� ���μ������� �ڿ� ������ ���μ������� Ŭ���̾�Ʈ ID ����
                                clients[j]->client_id--;
                            kill(clients[j]->pid, SIGUSR2);  // SIGUSR2 ��ȣ�� ���ŵ� ������ ����
                        }
                    }

                    write(clients[i]->csock, recvCli.mesg, strlen(recvCli.mesg));  // ����� Ŭ���̾�Ʈ�� ���� �޽��� ����
                    remove_client(i);  // Ŭ���̾�Ʈ�� ��Ͽ��� ����
                    break;
                } else {  // �ٸ� �޽����� ó���ϴ� ���
                    // �ٸ� Ŭ���̾�Ʈ���� �޽����� ��ε�ĳ��Ʈ (��� Ŭ���̾�Ʈ���� ����)
                    for (int j = 0; j < client_count; j++) {
                        if (j != i) {  // �ڱ� �ڽ��� ������ Ŭ���̾�Ʈ�鿡�� ����
                            if (write(clients[j]->csock, recvCli.mesg, strlen(recvCli.mesg)) > 0) {  // �޽��� ���� ����
                                printf("Parent send Data to %d Client pipe: success\n", j);
                            }
                            else {  // �޽��� ���� ���� ó��
                                printf("Parent send Data to %d Client pipe: fail\n", j);
                                perror("write()");
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

// �ڽ� �� SIGUSR2 ó�� �Լ� (�θ� ���μ����� �ڽ� ���μ������� �����ϴ� ��ȣ�� ó��)
void sigHandlerChild(int signo) {
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