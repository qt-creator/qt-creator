// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "sourcepathstorage/nonlockingmutex.h"

#include <modelfwd.h>

namespace QmlDesigner {

class SourcePathStorage;

template<typename Storage, typename Mutex>
class SourcePathCache;

class QMLDESIGNERCORE_EXPORT FileSystem : public FileSystemInterface
{

public:
    FileSystem(PathCacheType &sourcePathCache)
        : m_sourcePathCache(sourcePathCache)
    {}

    SourceIds directoryEntries(const QString &directoryPath) const override;
    QStringList fileNames(const QString &directoryPath, const QStringList &nameFilters) const override;
    long long lastModified(SourceId sourceId) const override;
    FileStatus fileStatus(SourceId sourceId) const override;
    QString contentAsQString(const QString &filePath) const override;
    QStringList subdirectories(const QString &directoryPath) const override;

    void remove(const SourceIds &sourceIds) override;

private:
    PathCacheType &m_sourcePathCache;
};

} // namespace QmlDesigner
