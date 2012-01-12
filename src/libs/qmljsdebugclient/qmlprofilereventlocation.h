#ifndef QMLPROFILEREVENTLOCATION_H
#define QMLPROFILEREVENTLOCATION_H

#include "qmljsdebugclient_global.h"

namespace QmlJsDebugClient {

struct QMLJSDEBUGCLIENT_EXPORT QmlEventLocation
{
    QmlEventLocation() : line(-1),column(-1) {}
    QmlEventLocation(const QString &file, int lineNumber, int columnNumber) : filename(file), line(lineNumber), column(columnNumber) {}
    QString filename;
    int line;
    int column;
};

}

#endif // QMLPROFILEREVENTLOCATION_H
