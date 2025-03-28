#include <stdio.h>

typedef struct Student{

    int id;
    char name[20];
    float score;
}student;

int main()
{
    student s1 = {1,"hyojin",98.5};
    student *p = &s1;

    printf("Name: %s\n", p->name);

    return 0;
}


