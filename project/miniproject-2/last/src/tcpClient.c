#include <stdio.h>      // �⺻ ����� �Լ� ��� (printf, fgets ��)
#include <unistd.h>     // UNIX ǥ�� �Լ� ��� (close ��)
#include <string.h>     // ���ڿ� ó�� �Լ� ��� (memset, strlen ��)

#include <sys/socket.h> // ���� ���α׷����� ���� ���̺귯��
#include <arpa/inet.h>  // ���ͳ� �������� �ּ� ��ȯ �Լ� (inet_pton ��)

#define TCP_PORT 5100   // ������ ����� TCP ��Ʈ ��ȣ ����

int main(int argc, char** argv)
{
    int ssock;                         // Ŭ���̾�Ʈ ���� ��ũ����
    struct sockaddr_in servaddr;       // ���� �ּ� ������ ��� ����ü
    char mesg[BUFSIZ];                 // �޽��� ���� (Ŭ���̾�Ʈ -> ����)

    // ���α׷� ���� �� IP �ּҰ� �ԷµǾ����� Ȯ��
    if (argc < 2)
    {
        printf("Usage : %s IP_ADDRESS\n", argv[0]); // ���� ���
        return -1;
    }

    // ���� ���� (AF_INET: IPv4, SOCK_STREAM: TCP ���, 0: �������� �ڵ� ����)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() :");          // ���� ���� ���� �� ���� ���
        return -1;
    }

    // ���� �ּ� ����ü �ʱ�ȭ
    memset(&servaddr, 0, sizeof(servaddr)); // ����ü�� ��� �ʵ带 0���� �ʱ�ȭ
    servaddr.sin_family = AF_INET;          // IPv4 �ּ� ü�� ����

    // ����ڰ� �Է��� IP �ּҸ� ��Ʈ��ũ ����Ʈ ������ ��ȯ�Ͽ� ����ü�� ����
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TCP_PORT);    // ��Ʈ ��ȣ ���� (ȣ��Ʈ ����Ʈ ������ ��Ʈ��ũ ������ ��ȯ)

    // ������ ���� �õ�
    if (connect(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect() :");         // ���� ���� �� ���� ���
        return -1;
    }

    // ǥ�� �Է����κ��� �޽��� �Է� �ޱ� (����ڰ� �Է��� �޽���)
    fgets(mesg, BUFSIZ, stdin);

    // �Է¹��� �޽����� ������ ���� (������ ���� ����)
    if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0)
    {
        perror("send() :");            // �޽��� ���� ���� �� ���� ���
        return -1;
    }

    // �����κ��� ������ ���� �� ����� �޽��� ���� �ʱ�ȭ
    memset(mesg, 0, BUFSIZ);

    // �����κ��� ������ ����
    if (recv(ssock, mesg, BUFSIZ, 0) <= 0)
    {
        perror("recv() :");            // ���� ���� ���� �� ���� ���
        return -1;
    }

    // ������ �����͸� ���
    printf("Received data : %s ", mesg);

    // ��� ���� �� ���� �ݱ�
    close(ssock);

    return 0;
}
