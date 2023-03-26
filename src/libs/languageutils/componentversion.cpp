// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentversion.h"

#include <QString>
#include <QCryptographicHash>

#include <limits>

namespace LanguageUtils {

const int ComponentVersion::NoVersion = -1;
const int ComponentVersion::MaxVersion = std::numeric_limits<int>::max();

ComponentVersion::ComponentVersion()
    : _major(NoVersion), _minor(NoVersion)
{
}

ComponentVersion::ComponentVersion(int major, int minor)
    : _major(major), _minor(minor)
{
}

ComponentVersion::ComponentVersion(const QString &versionString)
    : _major(NoVersion), _minor(NoVersion)
{
    int dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;
    bool ok = false;
    int maybeMajor = versionString.left(dotIdx).toInt(&ok);
    if (!ok)
        return;
    int maybeMinor = versionString.mid(dotIdx + 1).toInt(&ok);
    if (!ok)
        return;
    _major = maybeMajor;
    _minor = maybeMinor;
}

ComponentVersion::~ComponentVersion()
{
}

bool ComponentVersion::isValid() const
{
    return _major >= 0 && _minor >= 0;
}

QString ComponentVersion::toString() const
{
    QByteArray temp;
    QByteArray result;
    result += temp.setNum(_major);
    result += '.';
    result += temp.setNum(_minor);
    return QString::fromLatin1(result);
}

void ComponentVersion::addToHash(QCryptographicHash &hash) const
{
    hash.addData(reinterpret_cast<const char *>(&_major), sizeof(_major));
    hash.addData(reinterpret_cast<const char *>(&_minor), sizeof(_minor));
}

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() < rhs.minorVersion());
}

bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() <= rhs.minorVersion());
}

bool operator>(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return rhs < lhs;
}

bool operator>=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return rhs <= lhs;
}

bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() == rhs.minorVersion();
}

bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return !(lhs == rhs);
}

} // namespace LanguageUtils
