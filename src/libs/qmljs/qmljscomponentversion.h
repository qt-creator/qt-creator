#ifndef QMLJSCOMPONENTVERSION_H
#define QMLJSCOMPONENTVERSION_H

#include "qmljs_global.h"

namespace QmlJS {

class QMLJS_EXPORT ComponentVersion
{
    int _major;
    int _minor;

public:
    static const int NoVersion;

    ComponentVersion();
    ComponentVersion(int major, int minor);
    ~ComponentVersion();

    int major() const
    { return _major; }
    int minor() const
    { return _minor; }

    // something in the GNU headers introduces the major() and minor() defines,
    // use these two to disambiguate when necessary
    int majorVersion() const
    { return _major; }
    int minorVersion() const
    { return _minor; }

    bool isValid() const;
};

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs);

} // namespace QmlJS

#endif // QMLJSCOMPONENTVERSION_H
