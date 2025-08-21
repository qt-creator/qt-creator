// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include "filestatus.h"
#include "filesysteminterface.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT FileStatusCache
{
public:
    using size_type = FileStatuses::size_type;

    FileStatusCache(FileSystemInterface &fileSystem)
        : m_fileSystem(fileSystem)
    {}
    FileStatusCache &operator=(const FileStatusCache &) = delete;
    FileStatusCache(const FileStatusCache &) = delete;

    const FileStatus &find(SourceId sourceId) const;
    const FileStatus &updateAndFind(SourceId sourceId) const;

    void remove(const DirectoryPathIds &directoryPathIds);
    void remove(const SourceIds &sourceIds);

    SourceIds modified(SourceIds sourceIds) const;

    size_type size() const;

private:
    mutable FileStatuses m_cacheEntries;
    FileSystemInterface &m_fileSystem;
};

} // namespace QmlDesigner
