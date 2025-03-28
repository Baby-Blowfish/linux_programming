#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int main( int argc, char **argv)
{
    int n, in, out;
    char buf[1024];

    if(argc < 3)
    {
        write(2, "Usage : ./copy source_file1 target_file2\n", 39);
        return -1;
    }

    if( (in = open(argv[1], O_RDONLY))<0)
    {
        perror("source_file1");
        return -1;
    }

    if((out = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0)
    {
        perror(argv[2]);
        close(in);
        return -1;
    }

    while((n= read(in,buf,sizeof(buf)))>0)
    {
        if(write(out, buf, n) != n)
        {
            perror("write");
            close(in);
            close(out);
            return -1;
        }
    }

    close(in);
    close(out);


    return 0;
}

