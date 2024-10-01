#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>

int main(int argc, char **argv)
{
    mqd_t mq;
    const char* name = "/posix_msq";
    char buf[BUFSIZ];

    mq = mq_open(name, O_WRONLY);

   strcpy(buf, "hello world\n"); 
   mq_send(mq,buf,strlen(buf),0);

   strcpy(buf,"q");
   mq_send(mq,buf,strlen(buf),0);

   mq_close(mq);

   return 0;
}

