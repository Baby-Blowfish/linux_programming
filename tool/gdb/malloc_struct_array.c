#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[20];
    float score;
} Student;

int main() {
    int count = 3;
    Student *list = (Student *)malloc(sizeof(Student) * count);

    list[0].id = 1;
    snprintf(list[0].name, 20, "kim");
    list[0].score = 95.5;

    list[1].id = 2;
    snprintf(list[1].name, 20, "hyo");
    list[1].score = 88.0;

    list[2].id = 3;
    snprintf(list[2].name, 20, "jin");
    list[2].score = 92.0;

    free(list);
    return 0;
}

