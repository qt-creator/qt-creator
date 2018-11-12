/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include <filepathid.h>

#include <vector>

namespace ClangBackEnd {

enum class SourceType : unsigned char
{
    TopInclude,
    TopSystemInclude,
    UserInclude,
    SystemInclude
};

class TimeStamp
{
    using int64 = long long;
public:
    TimeStamp(int64 value)
        : value(value)
    {}

    operator int64() const
    {
        return value;
    }

    int64 value = -1;
};

class SourceEntry
{
    using int64 = long long;
public:
    SourceEntry(int sourceId, int64 lastModified, int sourceType)
        : lastModified(lastModified),
          sourceId(sourceId),
          sourceType(static_cast<SourceType>(sourceType))
    {}

    SourceEntry(FilePathId sourceId, SourceType sourceType, TimeStamp lastModified)
        : lastModified(lastModified),
          sourceId(sourceId),
          sourceType(sourceType)
    {}

    friend
    bool operator<(SourceEntry first, SourceEntry second)
    {
        return first.sourceId < second.sourceId;
    }

    friend
    bool operator==(SourceEntry first, SourceEntry second)
    {
        return first.sourceId == second.sourceId
            && first.sourceType == second.sourceType
            && first.lastModified == second.lastModified ;
    }

    friend bool operator!=(SourceEntry first, SourceEntry second) { return !(first == second); }

public:
    TimeStamp lastModified;
    FilePathId sourceId;
    SourceType sourceType = SourceType::UserInclude;
};

using SourceEntries = std::vector<SourceEntry>;

}
