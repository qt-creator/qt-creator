#include "diagnostic_erroneous_header.h"

enum Numbers { One, Two, Three };

void f(Numbers n)
{
    switch (n) {
    case One: return;
    case Two: return;
    }
}

void g()
{
    return 3;
}
