#include <string.h>

char *lower;

int main()
{
    lower = strdup("asdf");

    while (*lower)
        *(lower++);

    return 0;
}
