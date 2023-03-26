// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <vector>

namespace QmlDesigner {

class FileStatus
{
public:
    explicit FileStatus() = default;
    explicit FileStatus(SourceId sourceId, long long size, long long lastModified)
        : sourceId{sourceId}
        , size{size}
        , lastModified{lastModified}
    {}

    friend bool operator==(const FileStatus &first, const FileStatus &second)
    {
        return first.sourceId == second.sourceId && first.size == second.size
               && first.lastModified == second.lastModified && first.size >= 0
               && first.lastModified >= 0;
    }

    friend bool operator!=(const FileStatus &first, const FileStatus &second)
    {
        return !(first == second);
    }

    friend bool operator<(const FileStatus &first, const FileStatus &second)
    {
        return first.sourceId < second.sourceId;
    }

    friend bool operator<(SourceId first, const FileStatus &second)
    {
        return first < second.sourceId;
    }

    friend bool operator<(const FileStatus &first, SourceId second)
    {
        return first.sourceId < second;
    }

    bool isValid() const { return sourceId && size >= 0 && lastModified >= 0; }

    explicit operator bool() const { return isValid(); }

public:
    SourceId sourceId;
    long long size = -1;
    long long lastModified = -1;
};

using FileStatuses = std::vector<FileStatus>;
} // namespace QmlDesigner
