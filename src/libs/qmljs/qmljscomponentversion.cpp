#include "qmljscomponentversion.h"

using namespace QmlJS;

const int ComponentVersion::NoVersion = -1;

ComponentVersion::ComponentVersion()
    : _major(NoVersion), _minor(NoVersion)
{
}

ComponentVersion::ComponentVersion(int major, int minor)
    : _major(major), _minor(minor)
{
}

ComponentVersion::~ComponentVersion()
{
}

bool ComponentVersion::isValid() const
{
    return _major >= 0 && _minor >= 0;
}

namespace QmlJS {

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.major() < rhs.major()
            || (lhs.major() == rhs.major() && lhs.minor() < rhs.minor());
}

bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.major() < rhs.major()
            || (lhs.major() == rhs.major() && lhs.minor() <= rhs.minor());
}

bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.major() == rhs.major() && lhs.minor() == rhs.minor();
}

bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return !(lhs == rhs);
}

}
