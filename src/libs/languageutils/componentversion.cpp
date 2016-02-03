/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "componentversion.h"

#include <QString>
#include <QCryptographicHash>

#include <limits>

using namespace LanguageUtils;

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
    int maybeMajor = versionString.leftRef(dotIdx).toInt(&ok);
    if (!ok)
        return;
    int maybeMinor = versionString.midRef(dotIdx + 1).toInt(&ok);
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
    return QString::fromLatin1("%1.%2").arg(QString::number(_major),
                                            QString::number(_minor));
}

void ComponentVersion::addToHash(QCryptographicHash &hash) const
{
    hash.addData(reinterpret_cast<const char *>(&_major), sizeof(_major));
    hash.addData(reinterpret_cast<const char *>(&_minor), sizeof(_minor));
}

namespace LanguageUtils {

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

}
