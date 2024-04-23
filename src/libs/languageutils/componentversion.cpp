// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentversion.h"

#include <QString>
#include <QCryptographicHash>

#include <limits>

namespace LanguageUtils {

// QTC_TEMP

ComponentVersion::ComponentVersion(QStringView versionString)
    : _major(NoVersion)
    , _minor(NoVersion)
{
    auto dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;
    bool ok = false;
    auto maybeMajor = versionString.left(dotIdx).toInt(&ok);
    if (!ok)
        return;
    int maybeMinor = versionString.mid(dotIdx + 1).toInt(&ok);
    if (!ok)
        return;
    _major = maybeMajor;
    _minor = maybeMinor;
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

} // namespace LanguageUtils
