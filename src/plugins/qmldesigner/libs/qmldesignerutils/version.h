// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerutils_global.h"

#include <limits>
#include <QString>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace QmlDesigner {

class Version
{
public:
    static Version QMLDESIGNERUTILS_EXPORT fromString(QStringView string);

    QString QMLDESIGNERUTILS_EXPORT toString() const;

    bool isEmpty() const { return major == std::numeric_limits<int>::max(); }

    friend bool operator==(Version first, Version second)
    {
        return std::tie(first.major, first.minor, first.patch)
               == std::tie(second.major, second.minor, second.patch);
    }

    friend bool operator<(Version first, Version second)
    {
        return std::tie(first.major, first.minor, first.patch)
               < std::tie(second.major, second.minor, second.patch);
    }

    friend bool operator>(Version first, Version second) { return second < first; }

    friend bool operator<=(Version first, Version second) { return !(second < first); }

    friend bool operator>=(Version first, Version second) { return !(first < second); }

    friend QMLDESIGNERUTILS_EXPORT QDebug operator<<(QDebug debug, Version version);

public:
    int major = std::numeric_limits<int>::max();
    int minor = 0;
    int patch = 0;
};

} // namespace QmlDesigner
