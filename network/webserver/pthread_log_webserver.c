#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_BUF 1024

/* 로그 기록용 구조체 */
typedef struct {
    int csock;
    char ip[INET_ADDRSTRLEN];
} thread_arg_t;

/* 함수 선언 */
static void *clnt_connection(void *arg);
int sendData(FILE *fp, char *ct, const char *filename);
void sendOk(FILE *fp);
void sendError(FILE *fp);
void write_log(const char *ip, const char *filename);

int main(int argc, char **argv)
{
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind()");
        return -1;
    }

    if (listen(ssock, 10) == -1)
    {
        perror("listen()");
        return -1;
    }

    printf("HTTP Server listening on port %s...\n", argv[1]);

    while (1)
    {
        int csock;
        char ip_str[INET_ADDRSTRLEN];

        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
        if (csock < 0)
        {
            perror("accept()");
            continue;
        }

        inet_ntop(AF_INET, &cliaddr.sin_addr, ip_str, sizeof(ip_str));
        printf("Client connected: %s:%d\n", ip_str, ntohs(cliaddr.sin_port));

        thread_arg_t *targ = malloc(sizeof(thread_arg_t));
        targ->csock = csock;
        strncpy(targ->ip, ip_str, INET_ADDRSTRLEN);

        pthread_create(&thread, NULL, clnt_connection, targ);
        pthread_detach(thread);
    }

    return 0;
}

void *clnt_connection(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    int csock = targ->csock;
    char *ip = targ->ip;

    FILE *clnt_read = fdopen(csock, "r");
    FILE *clnt_write = fdopen(dup(csock), "w");

    if (!clnt_read || !clnt_write)
    {
        perror("fdopen()");
        close(csock);
        free(targ);
        return NULL;
    }

    char reg_line[MAX_BUF], method[MAX_BUF], filename[MAX_BUF], *ret;

    fgets(reg_line, MAX_BUF, clnt_read); // 첫 줄 (Request Line)
    fputs(reg_line, stdout);

    ret = strtok(reg_line, " ");
    strcpy(method, ret ? ret : "");

    if (strcmp(method, "POST") == 0)
    {
        sendOk(clnt_write);
        goto END;
    }
    else if (strcmp(method, "GET") != 0)
    {
        sendError(clnt_write);
        goto END;
    }

    ret = strtok(NULL, " ");
    strcpy(filename, ret ? ret : "");

    // '/' 제거
    if (filename[0] == '/')
        memmove(filename, filename + 1, strlen(filename));

    // 헤더 skip
    while (fgets(reg_line, MAX_BUF, clnt_read) && strcmp(reg_line, "\r\n") != 0)
        ;

    // 로그 기록
    write_log(ip, filename);

    // 파일 전송
    sendData(clnt_write, "text/html", filename);

END:
    fclose(clnt_read);
    fclose(clnt_write);
    free(targ);
    return NULL;
}

/* 파일 응답 */
int sendData(FILE *fp, char *ct, const char *filename)
{
    char buf[MAX_BUF];
    int fd, len;

    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Server: SimpleServer/1.0\r\n");
    fprintf(fp, "Content-Type: %s\r\n", ct);
    fprintf(fp, "\r\n");

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        fprintf(fp, "<html><body><h1>404 Not Found</h1></body></html>");
        fflush(fp);
        return -1;
    }

    while ((len = read(fd, buf, sizeof(buf))) > 0)
    {
        fwrite(buf, 1, len, fp);
    }

    close(fd);
    fflush(fp);
    return 0;
}

/* POST 응답 */
void sendOk(FILE *fp)
{
    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Server: SimpleServer/1.0\r\n\r\n");
    fflush(fp);
}

/* 오류 응답 */
void sendError(FILE *fp)
{
    fprintf(fp, "HTTP/1.1 400 Bad Request\r\n");
    fprintf(fp, "Server: SimpleServer/1.0\r\n");
    fprintf(fp, "Content-Type: text/html\r\n");
    fprintf(fp, "\r\n");
    fprintf(fp, "<html><body><h1>400 Bad Request</h1></body></html>");
    fflush(fp);
}

/* 요청 로그 작성 */
void write_log(const char *ip, const char *filename)
{
    FILE *logfp = fopen("access.log", "a");
    if (!logfp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(logfp, "[%04d-%02d-%02d %02d:%02d:%02d] IP: %s Requested: %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            ip, filename);

    fclose(logfp);
}
