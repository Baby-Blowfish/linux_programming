#include "server_functions.h"

int ssock;              // 서버 소켓 디스크립터
int client_count = 0;    // 현재 접속된 클라이언트 수
Client* clients[MAX_CLIENTS];

// 서버 측 SIGUSR1 처리 함수 (부모 프로세스가 자식 프로세스의 메시지를 처리하는 함수)
void sigHandlerParent(int signo) {
    if (signo == SIGUSR1) {  // SIGUSR1 신호를 받은 경우
        for (int i = 0; i < client_count; i++) {  // 현재 연결된 클라이언트 중 신호를 보낸 클라이언트를 찾음

            // 스택에 구조체 선언 (포인터 대신 구조체를 직접 선언하여 데이터를 수신)
            Client recvCli;

            // 파이프에서 데이터 읽기 (논블로킹 모드로 설정)
            int flags = fcntl(clients[i]->child_to_parent[0], F_GETFL, 0);
            fcntl(clients[i]->child_to_parent[0], F_SETFL, flags | O_NONBLOCK);

            // 논블로킹 read: 파이프에서 데이터 읽기
            ssize_t n = read(clients[i]->child_to_parent[0], &recvCli, sizeof(Client));

            if (n < 0) {  // 읽기 오류 처리
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 파이프에 읽을 데이터가 없음을 의미 (비동기 작업에서는 흔한 상황)
                } else {
                    // 그 외의 read 에러 처리
                }
            } else if (n == 0) {  // EOF: 파이프가 닫힘 (더 이상 데이터가 없음)
                printf("Pipe closed (EOF).\n");
            } else {  // 성공적으로 데이터를 읽은 경우
                printf("Parent read Data from %d Client pipe: %s", recvCli.client_id, recvCli.mesg);  // 부모가 자식으로부터 받은 메시지 출력

                if (!strncmp(recvCli.mesg, "/q", 2)) {  // "/q" 메시지인 경우 클라이언트 종료 처리

                    // 종료됨을 알림 (다른 클라이언트에게 알림)
                    for (int j = 0; j < client_count; j++) {
                        if (j != i) {  // 다른 클라이언트에게만 알림
                            char off[] = "  is terminating...\n";
                            off[0] = '0' + i;  // 종료된 클라이언트 번호를 알림
                            write(clients[j]->csock, off, strlen(off));  // 메시지를 다른 클라이언트에 전송

                            write(clients[j]->parent_to_child[1], &recvCli, sizeof(Client));  // 다른 자식 프로세스에게 종료를 알림

                            if (j > i)  // 종료된 자식 프로세스보다 뒤에 생성된 프로세스들의 클라이언트 ID 갱신
                                clients[j]->client_id--;
                            kill(clients[j]->pid, SIGUSR2);  // SIGUSR2 신호로 갱신된 정보를 전달
                        }
                    }

                    write(clients[i]->csock, recvCli.mesg, strlen(recvCli.mesg));  // 종료된 클라이언트에 종료 메시지 전송
                    remove_client(i);  // 클라이언트를 목록에서 제거
                    break;
                } else {  // 다른 메시지를 처리하는 경우
                    // 다른 클라이언트에게 메시지를 브로드캐스트 (모든 클라이언트에게 전송)
                    for (int j = 0; j < client_count; j++) {
                        if (j != i) {  // 자기 자신을 제외한 클라이언트들에게 전송
                            if (write(clients[j]->csock, recvCli.mesg, strlen(recvCli.mesg)) > 0) {  // 메시지 전송 성공
                                printf("Parent send Data to %d Client pipe: success\n", j);
                            }
                            else {  // 메시지 전송 실패 처리
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

// 자식 측 SIGUSR2 처리 함수 (부모 프로세스가 자식 프로세스에게 전달하는 신호를 처리)
void sigHandlerChild(int signo) {
    if (signo == SIGUSR2) {  // SIGUSR2 신호를 받은 경우

        // 스택에 구조체 선언 (포인터 대신 구조체를 직접 선언하여 데이터를 수신)
        Client recvCli;

        // 부모에서 자식으로 전달된 메시지 읽기
        if (read(clients[client_count]->parent_to_child[0], &recvCli, sizeof(Client)) > 0) {  // 부모->자식 파이프에서 데이터 읽기
            if (recvCli.client_id < clients[client_count]->client_id) {  // 클라이언트 ID가 변경되었으면
                clients[client_count]->client_id--;  // 클라이언트 ID를 감소시킴
            }
        } else {
            perror("pipe read()");  // 읽기 실패 시 오류 메시지 출력
        }
    }
}

// 클라이언트 목록에서 종료된 클라이언트를 제거하는 함수
void remove_client(int index) {
    if (clients[index] != NULL) {
        close(clients[index]->csock);                   // 클라이언트와 연결된 소켓 닫기
        close(clients[index]->child_to_parent[0]);      // 자식->부모 파이프 닫기
        close(clients[index]->parent_to_child[1]);      // 부모->자식 파이프 닫기

        printf("Child process %d removed\n", index);  // 클라이언트 제거 메시지 출력

        free(clients[index]);  // 동적으로 할당된 클라이언트 메모리 해제
        clients[index] = NULL;  // 포인터 초기화

        client_count--;  // 총 클라이언트 수 감소

        // 클라이언트 목록을 한칸씩 당기기 (클라이언트 목록에서 빈자리를 없앰)
        for (int i = index; i < client_count; i++) {
            clients[i] = clients[i + 1];
        }

        clients[client_count] = NULL;  // 마지막 클라이언트 포인터를 초기화
    }
}