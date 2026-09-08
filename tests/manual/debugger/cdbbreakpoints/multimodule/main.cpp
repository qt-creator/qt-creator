#include <cstdio>

int alphaValue();
int betaValue();

int main()
{
    int total = 0;
    total += alphaValue();
    total += betaValue();
    printf("total %d\n", total);
    return 0;
}
