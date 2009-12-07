#ifndef PACKAGEINFO_H
#define PACKAGEINFO_H

#include <qml/qml_global.h>

#include <QtCore/QString>

namespace Qml {

class QML_EXPORT PackageInfo
{
public:
    PackageInfo(const QString &name, int majorVersion, int minorVersion);

    QString name() const
    { return m_name; }

    int majorVersion() const
    { return m_majorVersion; }

    int minorVersion() const
    { return m_minorVersion; }

private:
    QString m_name;
    int m_majorVersion;
    int m_minorVersion;
};

} // namespace Qml

#endif // PACKAGEINFO_H
