#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[20];
    float score;
} Student;

int main() {
    Student *p = (Student *)malloc(sizeof(Student));
    p->id = 42;
    snprintf(p->name, 20, "hyojin");
    p->score = 99.9;

    free(p);

    // 접근 테스트 (의도적으로 위험)
    printf("ID: %d\n", p->id);
    return 0;
}

