// Copyright

#include "t.h"

int main()
{
#ifdef FEATURE_X_ENABLED
#define OK_FEATURE_X_ENABLED
    return 0;
#endif
    return 1;
}

