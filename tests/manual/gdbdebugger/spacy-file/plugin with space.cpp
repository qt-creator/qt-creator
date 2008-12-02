

#include <stdio.h>
#include <qglobal.h>

extern "C" Q_DECL_EXPORT int pluginTest()
{
    int s = 0;
    for (int i = 1; i != 2000; ++i)
        s += i; 
    fprintf(stderr, "in plugin test");
    return s;
}
