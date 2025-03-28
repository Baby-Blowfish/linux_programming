#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>   // waitpid()
#include <string.h>     // strlen(), strsignal()
#include <signal.h>     // signal()
#include <stdlib.h>     // exit()

static void sigHandler(int);    // �ñ׳� ó���� �ڵ鷯

// Pipe setting
int child_to_parent[2];         // �������� �������� ����

// Buffer setting
char buffer[BUFSIZ];            // �������� ���� ����

int main() {

    // Process setting
    pid_t pid;
    int status; // �ڽ� ���μ��� ����

    // Buffer setting
    int readBufferByte;

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

    // ���μ��� ��ũ
    if((pid = fork()) < 0)
    {
      perror("fork()");
      return -1;
    }
    else if(pid == 0) // �ڽ� ���μ���
    {
        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);

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


        } while (1);

        // �ڽ� -> �θ� ���� ������ ����
        close(child_to_parent[1]);
    } else // �θ� ���μ���
    {
        // �ڽ� -> �θ� ���� ������ �ݱ�
        close(child_to_parent[1]);

        do
        {
            // server�� ����� soket�� ���� ���
            // ���� ���� ������ ��� printf
        } while (1);


        // �ڽ� -> �θ� �б� ������
            read(child_to_parent[0], buffer, sizeof(buffer));
            printf("Parent received: %s\n", buffer);


        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);

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
        if (read(child_to_parent[0], buffer, sizeof(buffer)) > 0) {
            printf("Parent received: %s", buffer);
        } else {
            perror("pipe read()");
        }
    }
}