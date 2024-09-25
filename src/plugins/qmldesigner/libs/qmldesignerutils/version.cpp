// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "version.h"

#include <QDebug>
#include <QList>

namespace QmlDesigner {

Version Version::fromString(QStringView string)
{
    Version result;
    // Split version into parts (major, minor, patch)
    QList<QStringView> versionParts = string.split('.');

    auto readDigit = [](QStringView string, int defaultValue = 0) -> int {
        bool canConvert = false;
        int digit = string.toInt(&canConvert);
        return (canConvert && digit > -1) ? digit : defaultValue;
    };

    if (versionParts.size() > 0)
        result.major = readDigit(versionParts[0], std::numeric_limits<int>::max());

    if (versionParts.size() > 1)
        result.minor = readDigit(versionParts[1]);

    if (versionParts.size() > 2)
        result.patch = readDigit(versionParts[2]);

    return result;
}

QString Version::toString() const
{
    if (isEmpty())
        return {};

    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

QDebug operator<<(QDebug debug, Version version)
{
    return debug.noquote() << QStringView(u"Version(%1)").arg(version.toString());
}

} // namespace QmlDesigner
