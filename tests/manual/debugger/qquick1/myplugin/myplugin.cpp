#include "myplugin.h"
#include "mytype.h"

void MyPlugin::registerTypes(const char *uri)
{
    // @uri mymodule
    // bp here should be hit on startup
    qmlRegisterType<MyType>(uri, 1, 0, "MyType");
}

Q_EXPORT_PLUGIN(MyPlugin)
