#include <stdio.h>

typedef struct
{
    int id;
    char name[20];
    float score;
}Student;

int main()
{
    Student s1  = {1, "kim", 98.5};
    Student s2  = {2, "hyo", 91.0};
    Student s3  = {2, "jin", 89.5};

    Student *list[3] = {&s1, &s2, &s3};

    return 0;
}




