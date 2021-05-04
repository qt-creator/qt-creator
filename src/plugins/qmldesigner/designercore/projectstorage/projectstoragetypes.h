/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "projectstorageids.h"
#include "storagecacheentry.h"

#include <utils/smallstring.h>

#include <cstdint>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace QmlDesigner {

class FileNameView
{
public:
    friend bool operator==(const FileNameView &first, const FileNameView &second)
    {
        return first.sourceContextId == second.sourceContextId && first.fileName == second.fileName;
    }

    static int compare(FileNameView first, FileNameView second) noexcept
    {
        int directoryDifference = first.sourceContextId.id - second.sourceContextId.id;

        if (directoryDifference)
            return directoryDifference;

        return Utils::compare(first.fileName, second.fileName);
    }

public:
    Utils::SmallStringView fileName;
    SourceContextId sourceContextId;
};

class FileNameEntry
{
public:
    FileNameEntry(Utils::SmallStringView fileName, int sourceContextId)
        : fileName(fileName)
        , sourceContextId(sourceContextId)
    {}

    FileNameEntry(Utils::SmallStringView fileName, SourceContextId sourceContextId)
        : fileName(fileName)
        , sourceContextId(sourceContextId)
    {}

    FileNameEntry(FileNameView view)
        : fileName(view.fileName)
        , sourceContextId(view.sourceContextId)
    {}

    friend bool operator==(const FileNameEntry &first, const FileNameEntry &second)
    {
        return first.sourceContextId == second.sourceContextId && first.fileName == second.fileName;
    }

    friend bool operator!=(const FileNameEntry &first, const FileNameEntry &second)
    {
        return !(first == second);
    }

    friend bool operator==(const FileNameEntry &first, const FileNameView &second)
    {
        return first.sourceContextId == second.sourceContextId && first.fileName == second.fileName;
    }

    friend bool operator!=(const FileNameEntry &first, const FileNameView &second)
    {
        return !(first == second);
    }

    operator FileNameView() const { return {fileName, sourceContextId}; }

    operator Utils::SmallString() && { return std::move(fileName); }

public:
    Utils::SmallString fileName;
    SourceContextId sourceContextId;
};

namespace Sources {
class SourceContext
    : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceContextId>
{
    using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceContextId>;

public:
    using Base::Base;

    friend bool operator==(const SourceContext &first, const SourceContext &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using SourceContexts = std::vector<SourceContext>;

class Source : public StorageCacheEntry<FileNameEntry, FileNameView, SourceId>
{
    using Base = StorageCacheEntry<FileNameEntry, FileNameView, SourceId>;

public:
    using Base::Base;
    Source(Utils::SmallStringView sourceName, SourceContextId sourceContextId, SourceId sourceId)
        : Base{{sourceName, sourceContextId}, sourceId}
    {}

    Source(Utils::SmallStringView sourceName, int sourceContextId, int sourceId)
        : Base{{sourceName, SourceContextId{sourceContextId}}, SourceId{sourceId}}
    {}

    friend bool operator==(const Source &first, const Source &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using Sources = std::vector<Source>;

class SourceNameAndSourceContextId
{
public:
    constexpr SourceNameAndSourceContextId() = default;
    SourceNameAndSourceContextId(Utils::SmallStringView sourceName, int sourceContextId)
        : sourceName(sourceName)
        , sourceContextId(sourceContextId)
    {}
    SourceNameAndSourceContextId(Utils::SmallStringView sourceName, SourceContextId sourceContextId)
        : sourceName{sourceName}
        , sourceContextId{sourceContextId}
    {}

    Utils::SmallString sourceName;
    SourceContextId sourceContextId;
};
} // namespace ClangBackEnd

} // namespace QmlDesigner
