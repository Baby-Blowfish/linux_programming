#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<fcntl.h>

#define SERVER_PORT 5100

int main(int argc, char**argv)
{
    int ssock;
    int n;
    struct sockaddr_in  servaddr;
    char mesg[BUFSIZ];

    fd_set readfd;

    if(argc<2)
    {
        printf("Usage : %s IP_AEERESS\n",argv[0]);
        return -1;
    }

    if((ssock = socket(AF_INET, SOCK_STREAM,0))<0)
    {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(servaddr.sin_addr.s_addr)); 
    servaddr.sin_port = htons(SERVER_PORT);
    
    if(connect(ssock,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
    {
        perror("connect()");
        return -1;
    }

    fcntl(0,F_SETFL,SOCK_NONBLOCK);

    do
    { 
        FD_ZERO(&readfd);
        FD_SET(0,&readfd);
        FD_SET(ssock,&readfd);
        
        select(ssock,&readfd, NULL,NULL,NULL);
        
        if(FD_ISSET(0, &readfd))
        {
            memset(mesg,0,sizeof(mesg));

            if((n = read(0,mesg,BUFSIZ)) > 0)
            {

                mesg[n]='\0';
                printf("Received data : %s",mesg);
                write(ssock,mesg,n);
            }

        
        }
        else if(FD_ISSET(ssock, &readfd))
        {
            memset(mesg,0,sizeof(mesg));

            if((n = read(ssock,mesg,BUFSIZ)) > 0)
            {
                if(n==0)
                {
                    mesg[0]='q';
                }
                else
                {
                    mesg[n]='\0';
                    printf("Received data : %s",mesg);
                    write(1,mesg,n);
                }
            }

         }

    }while(strncmp(mesg,"q",1));\

    close(ssock);

    return 0;
}
