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
    struct sockaddr_in servaddr;
    char mesg[BUFSIZ];
    
    if(argc <2)
    {
        printf("Usage : %s IP_ADDRESS\n",argv[0]);
        return -1;
    }

    if((ssock = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket() :");
        return -1;
    }
        
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TCP_PORT);

    if(connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
    {
        perror("connect() :");
        return -1;
    }

    if(pipe(pfd)<0)
    {
        perror("pipe()");
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
            int n;
            memset(mesg, 0, BUFSIZ);
            if((n=recv(ssock, mesg, BUFSIZ,0)<0))
            {
                perror("child read() :");
                return -1;
            }
            mesg[n]='\0';
            mesg[n+1]='\n';
            printf("%s",mesg);

        } while(strncmp(mesg,"q",1));
        
        printf("child process die\n");
        close(ssock);
        
        exit(0);
    }
    else
    {  
        do
        {
            fgets(mesg,BUFSIZ,stdin);
            if(send(ssock,mesg,BUFSIZ,MSG_DONTWAIT)<=0)
            {
                perror("parent send()");
                return -1;
            }
            if(!strncmp(mesg,"q",1))
            {
                close(pfd[0]);
                if(write(pfd[1],mesg,strlen(mesg))<=0)
                {
                    perror("pipe write()");
                    return -1;
                }
                printf("call parent die\n");
                close(pfd[1]);
            }

        }
        while(strncmp(mesg,"q",1));
        close(ssock);
    }
    
    
    return 0;
}
