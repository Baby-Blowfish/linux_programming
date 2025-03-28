#include <stdio.h>
#include <string.h>

int main()
{
    FILE * fp_text, *fp_binary;
    char line[256];

    fp_text  = fopen("textfile.txt","w");
    if(fp_text == NULL)
    {
        perror("fopen - write");
        return 1;
    }

    printf("문자열을 입력하세요 (빈 줄 입력 시 종료):\n");

    while(1)
    {
        printf("> ");
        fgets(line, sizeof(line), stdin);

        if(strcmp(line, "\n")  == 0) break;
        fputs(line, fp_text);
    }

    fclose(fp_text);
    printf("\n입력한 내용을 텍스트 파일에 저장했습니다.\n\n");

    fp_text = fopen("textfile.txt","r");
    if(fp_text == NULL)
    {
        perror("fopen - read");
            return 1;
    }


    printf("====== 텍스트 파일 내용 =======\n");
    while(fgets(line, sizeof(line), fp_text))
    {
        printf("%s", line);
    }
    fclose(fp_text);


    fp_text = fopen("textfile.txt", "rb");
    fp_binary =  fopen("copy.bin", "wb");

    if(fp_text == NULL || fp_binary == NULL)
    {
        perror("fopen - binary");
        return 1;
    }


    size_t n;
    while((n = fread(line, sizeof(char), sizeof(line),fp_text))> 0)
    {
        fwrite(line, sizeof(char), n, fp_binary);
    }

    fclose(fp_text);
    fclose(fp_binary);
    printf("\n 텍스트 파일을 바이너리 파일(copy.bin)으로 복사완료!\n");

    return 0;
}


