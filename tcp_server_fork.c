#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <wait.h>

#define TCP_PORT 5100

int main(int argc, char**argv)
{
    pid_t pid;
    int pfd[2];
    int ssock,sitatus;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    char mesg[BUFSIZ];
    if((ssock = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket() :");
        return -1;
    }


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
    {
        perror("bind() :");
        return -1;
    }

    if((listen(ssock, 8) < 0))
    {
        perror("listen() : ");
        return -1;
    }

    clen = sizeof(cliaddr);

    do
    {
        int n, csock=accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is connected: %s\n",mesg);

        if(pipe(pfd) <0 )
        {
            perror("pipe() : ");
            return -1;
        }
        if((pid=fork())<0)
        {
            perror("fork()");
            return -1;
        }
        else if(pid == 0)
        {
            do
            {
                memset(mesg, 0, BUFSIZ);
                if((n=read(csock, mesg, BUFSIZ))<0)
                {
                    perror("read() :");
                }
                mesg[n]='\0';
                printf("Received data : %s", mesg);

                if(!strncmp(mesg,"w",1))
                {
                    printf("%s",mesg);
                    close(pfd[0]);
                    if(write(pfd[1],mesg,strlen(mesg))<=0)
                    {
                        perror("pipe write()");
                    }
                    printf("call parent die\n");
                    close(pfd[1]);
                }
                else if(!strncmp(mesg,"q",1))
                {
                    printf("%s",mesg);
                    close(pfd[0]);
                    if(write(pfd[1],mesg,strlen(mesg))<=0)
                    {
                        perror("pipe write()");
                    }
                    printf("call chiled die\n");
                    close(pfd[1]);
                }
                else;

             } while(strncmp(mesg,"q",1)&&strncmp(mesg,"w",1));
            printf("child process die\n");
            close(csock);
            exit(0);
        }
        else
        {
            close(pfd[1]);
            do
            {
                memset(mesg, 0, BUFSIZ);
                if((n=read(pfd[0], mesg, BUFSIZ))<0)
                {
                    perror("pipe read() :");
                }
                printf("parent receive %s\n",mesg);
            }while(strncmp(mesg,"w",1)&&strncmp(mesg,"q",1));
            printf("parent process die\n");
            close(pfd[0]);
            close(csock);
        }
    }while(strncmp(mesg,"w",1));
    close(ssock);

    return 0;
}
