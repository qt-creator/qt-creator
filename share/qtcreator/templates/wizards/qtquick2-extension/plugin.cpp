#include "%ProjectName:l%_plugin.%CppHeaderSuffix%"
#include "%ObjectName:l%.%CppHeaderSuffix%"

#include <qqml.h>

void %ProjectName:s%Plugin::registerTypes(const char *uri)
{
    // @uri %Uri%
    qmlRegisterType<%ObjectName%>(uri, 1, 0, "%ObjectName:c%");
}

