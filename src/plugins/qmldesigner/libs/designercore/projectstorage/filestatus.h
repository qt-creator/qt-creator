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
               && first.lastModified == second.lastModified;
    }

    friend std::weak_ordering operator<=>(const FileStatus &first, const FileStatus &second)
    {
        return first.sourceId <=> second.sourceId;
    }

    bool isExisting() const { return sourceId && size >= 0 && lastModified >= 0; }

    explicit operator bool() const { return bool(sourceId); }

    template<typename String>
    friend void convertToString(String &string, const FileStatus &fileStatus)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("source id", fileStatus.sourceId),
                              keyValue("size", fileStatus.size),
                              keyValue("last modified", fileStatus.lastModified));

        convertToString(string, dict);
    }

public:
    SourceId sourceId;
    long long size = -1;
    long long lastModified = -1;
};

using FileStatuses = std::vector<FileStatus>;
} // namespace QmlDesigner
