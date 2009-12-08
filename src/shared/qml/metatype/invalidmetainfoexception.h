#ifndef INVALIDMETAINFOEXCEPTION_H
#define INVALIDMETAINFOEXCEPTION_H

#include "exception.h"

namespace Qml {

class QML_EXPORT InvalidMetaInfoException : public Exception
{
public:
    InvalidMetaInfoException(int line,
                             const QString &function,
                             const QString &file);

    QString type() const;

};

}

#endif // INVALIDMETAINFOEXCEPTION_H
