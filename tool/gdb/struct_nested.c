#include <stdio.h>

typedef struct {
    char city[20];
    int zipcode;
} Address;

typedef struct{
    int id;
    char name[20];
    Address addr;
    float *score;
} Student;

int main()
{
    float s = 97.5;

    Student st =
    {
        1, "hyojin", {"Seoul",12345}, &s
    };

    return 0;

}
