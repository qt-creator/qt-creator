#include <cstdlib>

int main()
{
    int *p = new int;
    free(p);
    return 0;
}
