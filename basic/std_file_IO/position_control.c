#include <stdio.h>

int main()
{
    FILE *fp;
    char ch;
    long pos;
    fpos_t saved_pos;


    fp = fopen("position.txt", "w+");
    if(fp == NULL)
    {
        perror("fopen");
        return 1;
    }

    fputs("ABCDEFG12345678", fp);

    fseek(fp, 4, SEEK_SET);
    ch = fgetc(fp);
    printf("fseek후 문자 : %c \n", ch);

    pos = ftell(fp);
    printf("현재 파일 위치(ftell) : %ld \n", pos);


    fgetpos(fp, &saved_pos);

    fseek(fp, -1, SEEK_END);
    ch = fgetc(fp);
    printf("파일 끝에서 1바이트 전 문자: %c\n", ch);

    rewind(fp);
    ch =  fgetc(fp);
    printf("rewind 후 첫 문자: %c\n",ch);

    fsetpos(fp, &saved_pos);
    ch =  fgetc(fp);
    printf("fsetpos 첫 문자: %c\n",ch);

    fclose(fp);

    return 0;
}



