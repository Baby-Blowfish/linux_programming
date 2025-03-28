#include <stdio.h>
#include <string.h>
#include <ctype.h>

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);  // \n 제거용
}

int get_valid_int(const char *prompt)
{
    int n;
    while(1)
    {
        printf("%s", prompt);
        if(scanf("%d", &n) == 1)
        {
            clear_input_buffer();
            return n;
        }
        else
        {
            printf(" 숫자만 입력하세요");
            clear_input_buffer();
        }
    }
}

int is_all_digits(const char *s) {
    for (int i = 0; s[i] != '\0' && s[i] != '\n'; ++i) {
        if (!isdigit(s[i]))
            return 0;  // 숫자가 아닌 문자가 있다면 OK
    }
    return 1;  // 모두 숫자일 경우
}

void get_valid_string(char *dest, size_t size, const char *prompt) {
    while (1) {
        printf("%s", prompt);
        if (fgets(dest, size, stdin) == NULL) continue;
        dest[strcspn(dest, "\n")] = '\0';  // 개행 제거

        if (strlen(dest) == 0) continue;

        if (is_all_digits(dest)) {
            printf("❌ 숫자로만 구성된 문자열은 입력할 수 없습니다.\n");
        } else {
            break;
        }
    }
}



int main() {
    int num;
    char str[100];

    printf("정수를 입력하세요: ");
    scanf("%d", &num);
    clear_input_buffer();  // 입력 버퍼에 남은 개행 제거

    printf("문자열을 입력하세요: ");
    fgets(str, sizeof(str), stdin);  // 안전하게 문자열 입력

    printf("입력된 정수: %d\n", num);
    printf("입력된 문자열: %s\n", str);

    int age = get_valid_int("나이를 입력하세요: ");
    char name[100];
    get_valid_string(name, sizeof(name), "이름을 입력하세요 (숫자만❌): ");

    printf("입력된 나이: %d\n", age);
    printf("입력된 이름: %s\n", name);

    return 0;
}

