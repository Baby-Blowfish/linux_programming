#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid;
    int pfd[2];
    char line[BUFSIZ];
    int status;

    if(pipe(pfd)<0)
    {
        perror("pipe()");
        return -1;
    }

    if((pid = fork())<0)
    {
        perror("fork()");
        return -1;
    }
    else if(pid == 0)
    {
        printf("brother pfd0 = %d, pfd1 = %d\n",pfd[0],pfd[1]);
        
        close(pfd[0]);
        
        dup2(pfd[1],1);
        execl("/bin/date","date",NULL);
        
        write(pfd[1],"brother send to Parent about hello world\n",sizeof("brother send to Parent about hello world\n"));
        close(pfd[1]);
        _exit(127);
    }
    else
    {

        printf("mother pfd0 = %d, pfd1 = %d\n",pfd[0],pfd[1]);
        close(pfd[1]);
        if(read(pfd[0], line, BUFSIZ) < 0)
        {
            perror("read()");
            return -1;
        }

        printf("%s",line);
        if(read(pfd[0], line, BUFSIZ) < 0)
        {
            perror("read()");
            return -1;
        }

        printf("%s",line);

        close(pfd[0]);
        waitpid(pid, &status, 0);
    }

    return 0;
}
