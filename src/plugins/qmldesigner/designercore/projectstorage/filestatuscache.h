// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "filesysteminterface.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace QmlDesigner {

class FileStatusCache
{
public:
    using size_type = FileStatuses::size_type;

    FileStatusCache(FileSystemInterface &fileSystem)
        : m_fileSystem(fileSystem)
    {}
    FileStatusCache &operator=(const FileStatusCache &) = delete;
    FileStatusCache(const FileStatusCache &) = delete;

    long long lastModifiedTime(SourceId sourceId) const;
    long long fileSize(SourceId sourceId) const;
    const FileStatus &find(SourceId sourceId) const;

    void update(SourceId sourceId);
    void update(SourceIds sourceIds);
    SourceIds modified(SourceIds sourceIds) const;

    size_type size() const;

private:
    mutable FileStatuses m_cacheEntries;
    FileSystemInterface &m_fileSystem;
};

} // namespace QmlDesigner
