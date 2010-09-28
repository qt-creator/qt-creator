#include "%ObjectName:l%.%CppHeaderSuffix%"

#include <QtCore/QTime>
#include <QtDeclarative/qdeclarative.h>

%ObjectName%::%ObjectName%(QObject *parent):
        QObject(parent)
{
}

QML_DECLARE_TYPE(%ObjectName%);
