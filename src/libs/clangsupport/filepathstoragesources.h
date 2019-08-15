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

#include "stringcacheentry.h"

#include <utils/smallstring.h>

#include <cstdint>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace ClangBackEnd {

class FileNameView
{
public:
    friend bool operator==(const FileNameView &first, const FileNameView &second)
    {
        return first.directoryId == second.directoryId && first.fileName == second.fileName;
    }

    static int compare(FileNameView first, FileNameView second) noexcept
    {
        int directoryDifference = first.directoryId - second.directoryId;

        if (directoryDifference)
            return directoryDifference;

        return Utils::compare(first.fileName, second.fileName);
    }

public:
    Utils::SmallStringView fileName;
    int directoryId;
};

class FileNameEntry
{
public:
    FileNameEntry(Utils::SmallStringView fileName, int directoryId)
        : fileName(fileName)
        , directoryId(directoryId)
    {}

    FileNameEntry(FileNameView view)
        : fileName(view.fileName)
        , directoryId(view.directoryId)
    {}

    friend bool operator==(const FileNameEntry &first, const FileNameEntry &second)
    {
        return first.directoryId == second.directoryId && first.fileName == second.fileName;
    }

    friend bool operator!=(const FileNameEntry &first, const FileNameEntry &second)
    {
        return !(first == second);
    }

    operator FileNameView() const { return {fileName, directoryId}; }

    operator Utils::SmallString() && { return std::move(fileName); }

public:
    Utils::SmallString fileName;
    int directoryId;
};

namespace Sources {
class Directory : public StringCacheEntry<Utils::PathString, Utils::SmallStringView, int>
{
    using Base = StringCacheEntry<Utils::PathString, Utils::SmallStringView, int>;

public:
    using Base::Base;

    friend bool operator==(const Directory &first, const Directory &second)
    {
        return first.id == second.id && first.string == second.string;
    }
};

class Source : public StringCacheEntry<FileNameEntry, FileNameView, int>
{
    using Base = StringCacheEntry<FileNameEntry, FileNameView, int>;

public:
    using Base::Base;
    Source(Utils::SmallStringView sourceName, int directoryId, int sourceId)
        : Base{{sourceName, directoryId}, sourceId}
    {}

    friend bool operator==(const Source &first, const Source &second)
    {
        return first.id == second.id && first.string == second.string;
    }
};

class SourceNameAndDirectoryId
{
public:
    SourceNameAndDirectoryId(Utils::SmallStringView sourceName, int directoryId)
        : sourceName(sourceName), directoryId(directoryId)
    {}

    Utils::SmallString sourceName;
    int directoryId = -1;
};
} // namespace ClangBackEnd

} // namespace ClangBackEnd
