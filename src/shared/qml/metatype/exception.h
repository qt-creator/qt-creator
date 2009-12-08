#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <qml/qml_global.h>

#include <QString>

namespace Qml {

class QML_EXPORT Exception
{
public:
    Exception(int line,
              const QString &function,
              const QString &file);
    virtual ~Exception();

    virtual QString type() const=0;
    virtual QString description() const;

    int line() const;
    QString function() const;
    QString file() const;
    QString backTrace() const;

    static void setShouldAssert(bool assert);
    static bool shouldAssert();

private:
    int m_line;
    QString m_function;
    QString m_file;
    QString m_backTrace;
    static bool s_shouldAssert;
};

QML_EXPORT QDebug operator<<(QDebug debug, const Exception &exception);

} // namespace Qml

#endif // EXCEPTION_H
