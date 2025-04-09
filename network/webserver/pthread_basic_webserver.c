#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>

/*Thread function*/
static void *clnt_connection(void *arg);
int sendData(FILE* fp, char *ct, char *filename);
void sendOk(FILE* fp);
void sendError(FILE* fp);

int main(int argc, char**argv)
{
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    /* get server port */
    if(argc != 2)
    {
        printf("Usage : %s <port>\n",argv[0]);
    }

    /* make server socket */
    if((ssock=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("server soket()");
        return -1;
    }

    /* register server socket,  bind & listen function*/
    memset(&servaddr,0,sizeof(servaddr));   // init servaddr zero
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = (argc != 2) ? htons(8000) : htons(atoi(argv[1]));
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind()");
        return -1;
    }

    if(listen(ssock, 10) == -1)
    {
        perror("listen()");
        return -1;
    }
    
    while(1)
    {
        char mesg[BUFSIZ];
        int csock;

        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr*)&cliaddr,&len);

        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client IP : %s:%d\n",mesg,ntohs(cliaddr.sin_port));

        pthread_create(&thread, NULL, clnt_connection, &csock);
    }

    return 0;
}

void *clnt_connection(void *arg)
{
    /* call by address : csock */
    int csock = *((int*)arg);
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[BUFSIZ], type[BUFSIZ];
    char filename[BUFSIZ], *ret, portName[BUFSIZ];

    /* fd to *FILE */
    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    /* HTML is line text, so use fgets() */
    fgets(reg_line, BUFSIZ, clnt_read);
    fputs(reg_line, stdout);

    /* mathod token separate */
    ret = strtok(reg_line, " ");
    strcpy(method, (ret != NULL) ? ret : "");
    /* if post method send "ok", if not get method send "error"  */
    if(strcmp(method, "POST") == 0)
    {
        sendOk(clnt_write);
        goto END;
    }
    else if(strcmp(method, "GET") != 0)
    {
        sendError(clnt_write);
        goto END;
    }
    
    /* filename token separte */
    ret = strtok(NULL," ");
    strcpy(filename,(ret != NULL) ? ret : "");
    /* remove '/' from filename */
    if(filename[0] == '/')
    {
        for(int i = 0, j = 0; i < BUFSIZ; i++)
        {
			j++;
            filename[i] = filename[j];
            if(filename[j] == '\0') break;
        }
    }
	
	/* separate token "HTTP/1/1" */
    ret = strtok(NULL,"\r\n");

    /* next line get */
	fgets(reg_line, BUFSIZ, clnt_read);
    //fputs(reg_line, stdout);
	
	/* separate token "Host" */
	ret = strtok(reg_line, ": ");
	//printf("%s: ",ret);

	/* separate token "localhost" */
    ret = strtok(NULL,": ");
	printf("%s:",ret);

	/* separate token "8000" */
    ret = strtok(NULL,": ");
	strcpy(portName,ret);
    fputs(portName,stdout);
	

	do
    {
        /* read message header and print */
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line,stdout);
        strcpy(reg_buf, reg_line);
        char *str = strchr(reg_buf, ':');
    }while(strncmp(reg_line, "\r\n", 2));

    sendData(clnt_write, type, filename);

END:
    fclose(clnt_read);
    fclose(clnt_write);

    pthread_exit(0);

    return (void*)NULL;

}

int sendData(FILE* fp, char *ct, char *filename)
{
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[] = "Content-Tpye:text/html\r\n";
    char end[] = "\r\n";
    char buf[BUFSIZ];
    int fd, len;


    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filename, O_RDWR);
    do
    {
        len = read(fd, buf, BUFSIZ);
        fputs(buf, fp);
    }while(len == BUFSIZ);

    close(fd);

    return 0;
}

void sendOk(FILE* fp)
{

    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n\r\n";
    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}

void sendError(FILE* fp)
{
    char protocol[] = "HTTP/1.1 400 Bad\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_len[] = "Content-Length:1024\r\n";
    char cnt_type[] = "Content-Tpye:text/html\r\n";
    char end[] = "\r\n";

    char content1[] = "<html><head><title>BAD Connection</title></head>";
    char content2[] = "<body><font size = +5>Bad Request</font></body></html>";

    printf("send_error\n");
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);
    fputs(content1, fp);
    fputs(content2, fp);
 
    fflush(fp);
}
