#include <cstdlib>

int main()
{
    int *p = new int;
    delete p;
    delete p;
    return 0;
}
