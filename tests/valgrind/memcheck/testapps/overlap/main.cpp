#include <string.h>

int main()
{
    int *i = new int[10];
    memcpy(i, &i[1], 20);
    delete[] i;
    return 0;
}
