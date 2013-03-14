#include "myplugin.h"
#include "mytype.h"

#include <qqml.h>

void MyPlugin::registerTypes(const char *uri)
{
    // @uri mymodule
    // bp here should be hit on startup
    qmlRegisterType<MyType>(uri, 1, 0, "MyType");
}

