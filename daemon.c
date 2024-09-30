#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>

int main(int argc, char **argv)
{
    struct sigaction sa;
    struct rlimit rl;
    int fd0, fd1, fd2, i;
    pid_t pid;

    if(argc < 2)
    {
        printf("Usage : %s command\n",argv[0]);
        return -1;
    }
    
    /* set permissions */
    umask(0);
    /* system calls get and set resource limits.
     * RLIMIT_NOFILE : This  specifies  a  value  one greater than the maximum file descriptor number 
     *                 that can be opened by this process */
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        perror("getrlimit");
    }

    if((pid=fork())<0)
    {
        perror("fork()");
        return -1;
    }
    else if(pid != 0)       // parent process kill
    {
        return 0;
    }

    /* setsid runs a program in a new session. The command calls fork(2) if already a process group leader. Otherwise, it
       executes a program in the current process. This default behavior is possible to override by the --fork option.*/
    setsid();

    /* the  sigaction()  system call is used to change the action taken by a process on receipt of a specific signal.
        SIG_IGN to ignore this signal.
        sigemptyset() initializes the signal set given by set to empty, with all signals excluded from the set.
    */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL)< 0)
        {
        perror("sigaction() : Can't ignore SIGHUP");
    }

    /* chdir() changes the current working directory of the calling process to the directory specified in path.*/
    if(chdir("/")<0)
    {
        perror("cd()");
    }

    if(rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }

    for(i=0; i < rl.rlim_max; i++)
    {
        close(i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(argv[1], LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    syslog(LOG_INFO, "Daemon Process");
    
    while(1){}

    closelog();
    
    return 0;

    /* sudo apt install rsyslog */
}
