#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


int main() {
    int parent_to_child[2]; // �θ� -> �ڽ����� ������ ������
    int child_to_parent[2]; // �ڽ� -> �θ�� ������ ������
    pid_t pid;
    char parent_msg[] = "Hello from parent!";
    char child_msg[] = "Hello from child!";
    char buffer[128];
    int status;

    // �� ���� ������ ����
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("pipe()");
        return -1;
    }

    // ���μ��� ��ũ
    if((pid = fork()) < 0)
    {
      perror("fork()");
      return -1;
    }
    else if(pid == 0) {  // �ڽ� ���μ���
        // �θ� -> �ڽ� ���� ������ �ݱ�
        close(parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);


        // �θ� -> �ڽ� �б� ������
        read(parent_to_child[0], buffer, sizeof(buffer));
        printf("Child received: %s\n", buffer);

        // �ڽ� -> �θ� ���� ������
        write(child_to_parent[1], child_msg, strlen(child_msg) + 1);



        // �θ� -> �ڽ� �б� ������ ����
        close(parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ ����
        close(child_to_parent[1]);

        _exit(0);  // �ڽ� ���μ��� ����
    } else {  // �θ� ���μ���
        // �θ� -> �ڽ� �б� ������ �ݱ�
        close(parent_to_child[0]);
        // �ڽ� -> �θ� ���� ������ �ݱ�
        close(child_to_parent[1]);


        // �θ� -> �ڽ� ���� ������
        write(parent_to_child[1], parent_msg, strlen(parent_msg) + 1);

        // �ڽ� -> �θ� �б� ������
        read(child_to_parent[0], buffer, sizeof(buffer));
        printf("Parent received: %s\n", buffer);


        // �θ� -> �ڽ� ���� ������ �ݱ�
        close(parent_to_child[1]);
        // �ڽ� -> �θ� �б� ������ �ݱ�
        close(child_to_parent[0]);

        // �ڽ� ���μ��� ���� ���
        waitpid(pid, &status, 0);
    }

    return 0;
}
