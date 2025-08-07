// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystem.h"
#include "projectstorage.h"
#include "projectstorageids.h"
#include "sourcepathstorage/sourcepathcache.h"
#include "sqlitedatabase.h"

#include <utils/algorithm.h>

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <filesystem>

namespace QmlDesigner {

SourceIds FileSystem::directoryEntries(const QString &directoryPath) const
{
    QDir directory{directoryPath};

    QFileInfoList fileInfos = directory.entryInfoList();

    SourceIds sourceIds = Utils::transform<SourceIds>(fileInfos, [&](const QFileInfo &fileInfo) {
        return m_sourcePathCache.sourceId(SourcePath{fileInfo.path()});
    });

    std::sort(sourceIds.begin(), sourceIds.end());

    return sourceIds;
}

QStringList FileSystem::fileNames(const QString &directoryPath, const QStringList &nameFilters) const
{
    return QDir{directoryPath}.entryList(nameFilters, QDir::Files);
}

FileStatus FileSystem::fileStatus(SourceId sourceId) const
{
    auto sourcePath = sourceId.fileNameId()
                          ? m_sourcePathCache.sourcePath(sourceId)
                          : m_sourcePathCache.directoryPath(sourceId.directoryPathId());

    try {
        auto path = std::filesystem::path{std::string_view{sourcePath},
                                          std::filesystem::path::generic_format};

        auto directoryEntry = std::filesystem::directory_entry{path};
        if (directoryEntry.exists()) {
            auto lastWrite = directoryEntry.last_write_time();
            std::error_code error;
            long long fileSize = static_cast<long long>(directoryEntry.file_size(error));
            if (error)
                fileSize = 0;
            return FileStatus{sourceId, static_cast<long long>(fileSize), lastWrite};
        }
    } catch (const std::filesystem::filesystem_error &) {
    }

    return FileStatus{sourceId};
}

QString FileSystem::contentAsQString(const QString &filePath) const
{
    QFile file{filePath};
    if (file.open(QIODevice::ReadOnly))
        return QString::fromUtf8(file.readAll());

    return {};
}

QStringList FileSystem::subdirectories(const QString &directoryPath) const
{
    QStringList directoryPaths;
    directoryPaths.reserve(100);
    QDirIterator directoryIterator{directoryPath, QDir::Dirs | QDir::NoDotAndDotDot};

    while (directoryIterator.hasNext())
        directoryPaths.push_back(directoryIterator.next());

    return directoryPaths;
}

void FileSystem::remove(const SourceIds &sourceIds)
{
    for (SourceId sourceId : sourceIds)
        QFile::remove(QString{m_sourcePathCache.sourcePath(sourceId)});
}

} // namespace QmlDesigner
