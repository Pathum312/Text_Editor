#include <stdio.h>

typedef struct
{
    char name[50];
    int age;
    char email[255];
} User;

int main()
{
    User person = { "Pathum Senanyake", 22, "pathumsenanayake@proton.me" };

    printf("Name: %s\nAge: %d\nEmail: %s", person.name, person.age, person.email);

    return 0;
}