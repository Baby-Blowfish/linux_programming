#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_BUF 1024
#define CERT_FILE "server.crt"
#define KEY_FILE  "server.key"

typedef struct {
    SSL *ssl;
    char ip[INET_ADDRSTRLEN];
} thread_arg_t;

/* 함수 선언 */
void *clnt_connection(void *arg);
void write_log(const char *ip, const char *filename);
int send_file_response(SSL *ssl, const char *filename);
void send_error_response(SSL *ssl);

int main(int argc, char **argv)
{
    int ssock, csock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char ip_str[INET_ADDRSTRLEN];
    pthread_t thread;

    SSL_CTX *ctx;
    SSL *ssl;

    /* 포트 확인 */
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* OpenSSL 초기화 */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLS_server_method());

    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    /* 인증서와 개인키 로드 */
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0 ||
        !SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    /* 소켓 생성 및 설정 */
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    if (listen(ssock, 10) < 0) {
        perror("listen()");
        return -1;
    }

    printf("HTTPS Server started on port %s\n", argv[1]);

    while (1)
    {
        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
        if (csock < 0) {
            perror("accept()");
            continue;
        }

        inet_ntop(AF_INET, &cliaddr.sin_addr, ip_str, sizeof(ip_str));
        printf("Client connected: %s:%d\n", ip_str, ntohs(cliaddr.sin_port));

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, csock);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(csock);
            continue;
        }

        thread_arg_t *targ = malloc(sizeof(thread_arg_t));
        targ->ssl = ssl;
        strncpy(targ->ip, ip_str, INET_ADDRSTRLEN);

        pthread_create(&thread, NULL, clnt_connection, targ);
        pthread_detach(thread);
    }

    SSL_CTX_free(ctx);
    close(ssock);
    return 0;
}

void *clnt_connection(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    SSL *ssl = targ->ssl;
    char *ip = targ->ip;

    char buf[MAX_BUF], method[16], filename[256];
    int len;

    len = SSL_read(ssl, buf, sizeof(buf) - 1);
    if (len <= 0) {
        ERR_print_errors_fp(stderr);
        goto END;
    }

    buf[len] = '\0';
    printf("Request from %s:\n%s\n", ip, buf);

    // 파싱: "GET /index.html HTTP/1.1"
    sscanf(buf, "%s %s", method, filename);

    // '/' 제거
    if (filename[0] == '/')
        memmove(filename, filename + 1, strlen(filename));

    // 로그 기록
    write_log(ip, filename);

    if (strcasecmp(method, "GET") == 0) {
        send_file_response(ssl, filename);
    } else {
        send_error_response(ssl);
    }

END:
    SSL_shutdown(ssl);
    SSL_free(ssl);
    free(targ);
    return NULL;
}

/* 파일 전송 함수 */
int send_file_response(SSL *ssl, const char *filename)
{
    char buf[MAX_BUF];
    int fd = open(filename, O_RDONLY);
    int len;

    if (fd < 0) {
        send_error_response(ssl);
        return -1;
    }

    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    SSL_write(ssl, header, strlen(header));

    while ((len = read(fd, buf, sizeof(buf))) > 0) {
        SSL_write(ssl, buf, len);
    }

    close(fd);
    return 0;
}

/* 에러 응답 함수 */
void send_error_response(SSL *ssl)
{
    char header[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n";
    char body[] = "<html><body><h1>400 Bad Request</h1></body></html>";
    SSL_write(ssl, header, strlen(header));
    SSL_write(ssl, body, strlen(body));
}

/* 로그 기록 함수 */
void write_log(const char *ip, const char *filename)
{
    FILE *fp = fopen("access.log", "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] IP: %s Requested: %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            ip, filename);

    fclose(fp);
}
