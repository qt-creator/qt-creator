/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
