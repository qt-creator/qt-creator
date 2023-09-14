// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystem.h"
#include "projectstorage.h"
#include "projectstorageids.h"
#include "sourcepathcache.h"
#include "sqlitedatabase.h"

#include <utils/algorithm.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

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

QStringList FileSystem::qmlFileNames(const QString &directoryPath) const
{
    return QDir{directoryPath}.entryList({"*.qml"}, QDir::Files);
}

long long FileSystem::lastModified(SourceId sourceId) const
{
    QFileInfo fileInfo(QString(m_sourcePathCache.sourcePath(sourceId)));

    fileInfo.refresh();

    if (fileInfo.exists())
        return fileInfo.lastModified().toMSecsSinceEpoch() / 1000;

    return 0;
}

FileStatus FileSystem::fileStatus(SourceId sourceId) const
{
    QFileInfo fileInfo(QString(m_sourcePathCache.sourcePath(sourceId)));

    fileInfo.refresh();

    if (fileInfo.exists()) {
        return FileStatus{sourceId, fileInfo.size(), fileInfo.lastModified().toMSecsSinceEpoch()};
    }

    return FileStatus{sourceId, -1, -1};
}

QString FileSystem::contentAsQString(const QString &filePath) const
{
    QFile file{filePath};
    if (file.open(QIODevice::ReadOnly))
        return QString::fromUtf8(file.readAll());

    return {};
}

void FileSystem::remove(const SourceIds &sourceIds)
{
    for (SourceId sourceId : sourceIds)
        QFile::remove(QString{m_sourcePathCache.sourcePath(sourceId)});
}

} // namespace QmlDesigner
