#include "qmlpackageinfo.h"

using namespace Qml;

PackageInfo::PackageInfo(const QString &name, int majorVersion, int minorVersion):
        m_name(name),
        m_majorVersion(majorVersion),
        m_minorVersion(minorVersion)
{
}
