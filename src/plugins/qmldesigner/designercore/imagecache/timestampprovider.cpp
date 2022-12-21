// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timestampprovider.h"

#include <QDateTime>
#include <QFileInfo>

#include <limits>

namespace QmlDesigner {

Sqlite::TimeStamp TimeStampProvider::timeStamp(Utils::SmallStringView name) const
{
    QFileInfo info{QString{name}};
    if (info.exists())
        return info.lastModified().toSecsSinceEpoch();

    return {std::numeric_limits<long long>::max()};
}

} // namespace QmlDesigner
