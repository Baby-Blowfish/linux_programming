#include <stdio.h>      // ǥ�� ����� �Լ� ��� (printf ��)
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strncmp ��)
#include <unistd.h>     // ���н� ǥ�� �Լ� ��� (close ��)
#include <sys/socket.h> // ���� �Լ� ��� (socket, bind, listen, accept ��)
#include <arpa/inet.h>  // ���ͳ� �������� ���� �Լ� ��� (inet_ntop ��)

#define TCP_PORT 5100   // ������ ����� TCP ��Ʈ ��ȣ

int main(int argc, char **argv) {


    // Socket setting
    int ssock;          // ���� ���� ��ũ����
    socklen_t clen;      // Ŭ���̾�Ʈ �ּ� ����ü ũ��
    struct sockaddr_in servaddr, cliaddr;  // ���� �� Ŭ���̾�Ʈ �ּ� ����ü

    // Buffer setting
    char mesg[BUFSIZ];   // �޽��� ����

    // 1. ���� ���� ���� (AF_INET: IPv4, SOCK_STREAM: TCP, 0: �������� �ڵ� ����)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // ���� ���� ���� �� ���� ���
        return -1;
    }

    // 2. ���� �ּ� ����ü ����
    memset(&servaddr, 0, sizeof(servaddr)); // ����ü�� 0���� �ʱ�ȭ
    servaddr.sin_family = AF_INET;          // IPv4 �ּ� ü�� ����
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� IP �ּҿ��� ���� ���
    servaddr.sin_port = htons(TCP_PORT);    // ��Ʈ ��ȣ ���� (ȣ��Ʈ -> ��Ʈ��ũ ����Ʈ ����)

    // 3. ���� ������ ������ �ּҿ� ��Ʈ�� ���ε�
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");       // ���ε� ���� �� ���� ���
        return -1;
    }

    // 4. ���� ��� ť ���� (�ִ� 8���� Ŭ���̾�Ʈ ��� ����)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");    // ��� ���� ���� ���� �� ���� ���
        return -1;
    }

    clen = sizeof(cliaddr); // Ŭ���̾�Ʈ �ּ� ����ü�� ũ�⸦ �ʱ�ȭ

    // 5. Ŭ���̾�Ʈ ���� ���� (accept)
    int n, csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
    if (csock < 0) {
        perror("accept() :"); // ���� ���� ���� �� ���� ���
        //continue; // ���� �߻� �� ���� Ŭ���̾�Ʈ ���� ó���� ����
    }

    // 6. Ŭ���̾�Ʈ�� IP �ּҸ� ���ڿ� �������� ��ȯ�Ͽ� ���
    inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
    printf("Client is connected: %s\n", mesg);

    do {

        memset(mesg, 0, BUFSIZ); // �޽��� ���� �ʱ�ȭ
        // 7. Ŭ���̾�Ʈ�κ��� �޽��� ���� (read)
        if ((n = read(csock, mesg, BUFSIZ)) < 0) {
            perror("read() :");   // �޽��� ���� ���� �� ���� ���
            break;
        }

        // 8. ������ �޽����� ��� (null ���ڷ� �� ó��)
        mesg[n] = '\0';           // ������ �޽����� ���� null ���� �߰�
        printf("Received data: %s", mesg);

        // 9. Ŭ���̾�Ʈ�� �޽��� ���� (���� �޽����� �ٽ� Ŭ���̾�Ʈ�� ����)
        if (write(csock, mesg, n) <= 0) {
            perror("write()");    // �޽��� ���� ���� �� ���� ���
        }

    } while (strncmp(mesg, "q", 1) != 0); // �޽����� 'q'�� �����ϸ� ���� ����

    // 10. Ŭ���̾�Ʈ���� ���� ����
    close(csock);             // Ŭ���̾�Ʈ ���� ����
    // 11. ���� ���� ����
    close(ssock);                 // ���� ���� ����
    return 0;
}
