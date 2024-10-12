#include <stdio.h>      // ǥ�� ����� �Լ� ��� (printf ��)
#include <unistd.h>     // ���н� �ý��� �Լ� ��� (fork, pipe, read, write, close ��)
#include <sys/wait.h>   // waitpid() �Լ� ���
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (strlen, memset ��)
#include <signal.h>     // �ñ׳� ó�� �Լ� ��� (signal, kill ��)
#include <stdlib.h>     // exit() �Լ� ���

static void sigHandler(int);    // �ñ׳� ó���� �ڵ鷯 ����

// ������ ����
int child_to_parent[2];     // �ڽ� -> �θ�� �����͸� �����ϴ� ������
int parent_to_child[2];     // �θ� -> �ڽ����� �����͸� �����ϴ� ������

// ���� ����
char buffer[BUFSIZ];        // �����͸� �ְ���� ����

int main() {

    // ���μ��� ����
    pid_t pid;
    int status; // �ڽ� ���μ��� ���� ���� ����

    int readBufferByte;     // ���� ����Ʈ ���� �����ϴ� ����

    // ������ ���� (�θ� -> �ڽ�, �ڽ� -> �θ� ���� ����)
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("pipe()");   // ������ ���� ���� �� ���� �޽��� ���
        return -1;          // ���α׷� ����
    }

    // �ñ׳� �ڵ鷯 ��� (SIGUSR1�� ���� �� ����� �ڵ鷯 ����)
    if (signal(SIGUSR1, sigHandler) == SIG_ERR) {
        perror("signal");   // �ñ׳� ��� ���� �� ���� �޽��� ���
        return -1;
    }

    // ���μ��� ���� (fork)
    if ((pid = fork()) < 0) {
        perror("fork()");   // ���μ��� ���� ���� �� ���� �޽��� ���
        return -1;
    }
    else if (pid == 0) // �ڽ� ���μ���
    {
        // �θ� -> �ڽ� ���� ������ �ݱ� (�ڽ��� ���ʿ��� ���� ����)
        close(parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ �ݱ� (�ڽ��� ���ʿ��� ���� ����)
        close(child_to_parent[0]);

        do {
            memset(buffer, 0, BUFSIZ);  // ���� �ʱ�ȭ

            // ǥ�� �Է����κ��� �����͸� �Է¹޾� �������� ����
            if ((readBufferByte = read(0, buffer, sizeof(buffer))) < 0) {
                perror("stdin read()"); // �Է� ���� �� ���� �޽��� ���
            }

            // �ڽ� -> �θ� �������� ������ ����
            if (write(child_to_parent[1], buffer, strlen(buffer)) <= 0) {
                perror("child_to_parent pipe write()"); // ���� ���� �� ���� �޽��� ���
            }

            // �θ� ���μ������� SIGUSR1 �ñ׳��� ����
            kill(getppid(), SIGUSR1);

            memset(buffer, 0, BUFSIZ);  // ���� �ʱ�ȭ
            // �θ� -> �ڽ� ���������� ������ �б�
            read(parent_to_child[0], buffer, sizeof(buffer));
            printf("Child received: %s\n", buffer); // ���� ������ ���

        } while (1);  // ���� �ݺ�

        // �θ� -> �ڽ� �б� ������ ����
        close(parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ ����
        close(child_to_parent[1]);

        _exit(0);  // �ڽ� ���μ��� ����
    }
    else // �θ� ���μ���
    {
        // �θ� -> �ڽ� �б� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ �ݱ� (�θ�� ���ʿ��� ���� ����)
        close(child_to_parent[1]);

        do {
            // ������ ����� ������ ���� ��� (���� �̿ϼ��� �κ�)
            // ������ ���� �����͸� ��� (�� �κ��� ���� ���� ������� �����ؾ� ��)
        } while (1);  // ���� �ݺ�

        // �θ� -> �ڽ� ���� ������ ����
        close(parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ ����
        close(child_to_parent[0]);

        // �ڽ� ���μ��� ���� ���
        waitpid(pid, &status, 0);
    }

    return 0;
}

// SIGUSR1 �ñ׳� �ڵ鷯
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // �ڽ� -> �θ� �б� ���������� ������ �б�
        memset(buffer, 0, BUFSIZ);  // ���� �ʱ�ȭ
        if (read(child_to_parent[0], buffer, sizeof(buffer)) > 0) {
            // �θ� -> �ڽ� ���� �������� �����͸� �ٽ� �� (����)
            write(parent_to_child[1], buffer, strlen(buffer));
            // Ŭ���̾�Ʈ�� ����Ǿ��� �� �ٸ� ���μ������� ������ �� �ְ� �ؾ� ��
        } else {
            perror("pipe read()"); // �б� ���� �� ���� �޽��� ���
        }
    }
}
