#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd = open("log_fsync.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd<0)
    {
        perror("open");
        return 1;
    }

    for (int i = 1; i <= 10000; i++)
    {
        dprintf(fd, "Log Entry %d\n", i);
        fsync(fd);
        usleep(10000);
    }

    close(fd);
    return 0;
}


