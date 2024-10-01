#include <stdio.h>
#include <string.h>

int main()
{
    char str1[10] = "1234567891";
    char *str2 = "123456789";

    printf("%u %u\n", sizeof(str1), strlen(str1));

    return 0;
}
