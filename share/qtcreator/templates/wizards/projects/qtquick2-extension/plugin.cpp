#include "%{PluginHdr}"

#include "%{ObjectHdr}"

#include <qqml.h>

void %{PluginName}::registerTypes(const char *uri)
{
    // @uri %{Uri}
    qmlRegisterType<%{ObjectName}>(uri, 1, 0, "%{ObjectName}");
}

