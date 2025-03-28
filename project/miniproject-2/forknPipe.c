#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


int main() {
    int parent_to_child[2]; // 부모 -> 자식으로 보내는 파이프
    int child_to_parent[2]; // 자식 -> 부모로 보내는 파이프
    pid_t pid;
    char parent_msg[] = "Hello from parent!";
    char child_msg[] = "Hello from child!";
    char buffer[128];
    int status;

    // 두 개의 파이프 생성
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("pipe()");
        return -1;
    }

    // 프로세스 포크
    if((pid = fork()) < 0)
    {
      perror("fork()");
      return -1;
    }
    else if(pid == 0) {  // 자식 프로세스
        // 부모 -> 자식 쓰기 파이프 닫기
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);


        // 부모 -> 자식 읽기 파이프
        read(parent_to_child[0], buffer, sizeof(buffer));
        printf("Child received: %s\n", buffer);

        // 자식 -> 부모 쓰기 파이프
        write(child_to_parent[1], child_msg, strlen(child_msg) + 1);



        // 부모 -> 자식 읽기 파이프 종료
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 종료
        close(child_to_parent[1]);

        _exit(0);  // 자식 프로세스 종료
    } else {  // 부모 프로세스
        // 부모 -> 자식 읽기 파이프 닫기
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 닫기
        close(child_to_parent[1]);


        // 부모 -> 자식 쓰기 파이프
        write(parent_to_child[1], parent_msg, strlen(parent_msg) + 1);

        // 자식 -> 부모 읽기 파이프
        read(child_to_parent[0], buffer, sizeof(buffer));
        printf("Parent received: %s\n", buffer);


        // 부모 -> 자식 쓰기 파이프 닫기
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);

        // 자식 프로세스 종료 대기
        waitpid(pid, &status, 0);
    }

    return 0;
}
