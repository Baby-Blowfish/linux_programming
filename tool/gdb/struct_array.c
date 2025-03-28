#include <stdio.h>

typedef struct student
{
    int id;
    char name[20];
    int score;
}student;

int main()
{
    student list[3] =
    {
        {1,"kim",100},
        {2,"hyo",200},
        {3,"jin",300}
    };

    return 0;
}

