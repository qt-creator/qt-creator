#include "%ProjectName:l%.%CppHeaderSuffix%"
#include "%ObjectName:l%.%CppHeaderSuffix%"

#include <QtDeclarative/qdeclarative.h>

void %ProjectName%::registerTypes(const char *uri)
{
    qmlRegisterType<%ObjectName%>(uri, 1, 0, "%ObjectName%");
}

Q_EXPORT_PLUGIN(%ProjectName%);
