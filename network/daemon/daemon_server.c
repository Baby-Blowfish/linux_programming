/*
 * 데몬 웹 서버 구현
 * 데몬 프로세스로 실행되는 웹 서버입니다.
 * 멀티스레드로 클라이언트 요청을 처리하며,
 * 접속 로그를 기록합니다.
 */

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
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define MAX_BUF 1024
#define MAX_PATH 2048  // 전체 경로를 위한 더 큰 버퍼
#define DOCUMENT_ROOT "/home/hyojin/study/linux_programming/network/daemon"  // 문서 루트 디렉토리 추가

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
    struct sigaction sa;
    struct rlimit rl;
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;
    pid_t pid;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* 데몬 프로세스 초기화 */
    umask(0);
    
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        perror("getrlimit");
        return -1;
    }

    if((pid=fork())<0)
    {
        perror("fork()");
        return -1;
    }
    else if(pid != 0)
    {
        return 0;
    }

    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL)< 0)
    {
        perror("sigaction() : Can't ignore SIGHUP");
        return -1;
    }

    if(chdir("/")<0)
    {
        perror("cd()");
        return -1;
    }

    if(rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }

    for(int i=0; i < rl.rlim_max; i++)
    {
        close(i);
    }

    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);

    openlog("daemon_webserver", LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    /* 웹 서버 소켓 초기화 */
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        syslog(LOG_ERR, "socket() failed");
        return -1;
    }

    /* SO_REUSEADDR 옵션 설정 - 이미 사용 중인 포트 재사용 가능 */
    int opt = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        syslog(LOG_ERR, "setsockopt() failed");
        close(ssock);
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        syslog(LOG_ERR, "bind() failed");
        close(ssock);
        return -1;
    }

    if (listen(ssock, 10) == -1)
    {
        syslog(LOG_ERR, "listen() failed");
        close(ssock);
        return -1;
    }

    syslog(LOG_INFO, "HTTP Server started on port %s", argv[1]);

    while (1)
    {
        int csock;
        char ip_str[INET_ADDRSTRLEN];

        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
        if (csock < 0)
        {
            syslog(LOG_ERR, "accept() failed");
            continue;
        }

        inet_ntop(AF_INET, &cliaddr.sin_addr, ip_str, sizeof(ip_str));
        syslog(LOG_INFO, "Client connected: %s:%d", ip_str, ntohs(cliaddr.sin_port));

        thread_arg_t *targ = malloc(sizeof(thread_arg_t));
        if (!targ) {
            syslog(LOG_ERR, "malloc() failed");
            close(csock);
            continue;
        }
        
        targ->csock = csock;
        strncpy(targ->ip, ip_str, INET_ADDRSTRLEN);

        pthread_create(&thread, NULL, clnt_connection, targ);
        pthread_detach(thread);
    }

    /* 프로그램 종료 시 소켓 정리 */
    close(ssock);
    closelog();
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
        syslog(LOG_ERR, "fdopen() failed for client %s", ip);
        close(csock);
        free(targ);
        return NULL;
    }

    char reg_line[MAX_BUF], method[MAX_BUF], filename[MAX_BUF], full_path[MAX_PATH], *ret;

    fgets(reg_line, MAX_BUF, clnt_read);
    syslog(LOG_INFO, "Request from %s: %s", ip, reg_line);

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

    // 기본 문서 설정
    if (strlen(filename) == 0 || strcmp(filename, "/") == 0)
    {
        strcpy(filename, "index.html");
    }

    // 경로 길이 검사
    size_t root_len = strlen(DOCUMENT_ROOT);
    size_t file_len = strlen(filename);
    if (root_len + file_len + 2 > MAX_PATH) {  // +2 for '/' and null terminator
        syslog(LOG_ERR, "Path too long: %s/%s", DOCUMENT_ROOT, filename);
        sendError(clnt_write);
        goto END;
    }

    // 전체 경로 생성
    snprintf(full_path, MAX_PATH, "%s/%s", DOCUMENT_ROOT, filename);

    // 헤더 skip
    while (fgets(reg_line, MAX_BUF, clnt_read) && strcmp(reg_line, "\r\n") != 0)
        ;

    write_log(ip, filename);
    sendData(clnt_write, "text/html", full_path);

END:
    fclose(clnt_read);
    fclose(clnt_write);
    free(targ);
    return NULL;
}

int sendData(FILE *fp, char *ct, const char *filename)
{
    char buf[MAX_BUF];
    int fd, len;

    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Server: DaemonServer/1.0\r\n");
    fprintf(fp, "Content-Type: %s\r\n", ct);
    fprintf(fp, "\r\n");

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        syslog(LOG_ERR, "File not found: %s", filename);
        fprintf(fp, "<html><body><h1>404 Not Found</h1><p>The requested file %s was not found on this server.</p></body></html>", filename);
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

void sendOk(FILE *fp)
{
    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Server: DaemonServer/1.0\r\n\r\n");
    fflush(fp);
}

void sendError(FILE *fp)
{
    fprintf(fp, "HTTP/1.1 400 Bad Request\r\n");
    fprintf(fp, "Server: DaemonServer/1.0\r\n");
    fprintf(fp, "Content-Type: text/html\r\n");
    fprintf(fp, "\r\n");
    fprintf(fp, "<html><body><h1>400 Bad Request</h1></body></html>");
    fflush(fp);
}

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
