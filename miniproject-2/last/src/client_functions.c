#include "client_functions.h"
// SIGUSR1 �ñ׳� �ڵ鷯
void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // ���������� ������ �б�
        memset(buffer, 0, BUFSIZ);
        if (read(child_to_parent[0], buffer, BUFSIZ) > 0) {
            // ������ �޽��� ���� (������ ���� ����)
            if (write(ssock, buffer, strlen(buffer)) <= 0) {
                perror("parent soket write()");    // ���� ���� �� ���� ���
            }
        } else {
            perror("pipe read()");
        }
    }
}