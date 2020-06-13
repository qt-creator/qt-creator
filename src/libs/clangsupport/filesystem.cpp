/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include "filepathcachinginterface.h"

#include <utils/algorithm.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace ClangBackEnd {

FilePathIds FileSystem::directoryEntries(const QString &directoryPath) const
{
    QDir directory{directoryPath};

    QFileInfoList fileInfos = directory.entryInfoList();

    FilePathIds filePathIds = Utils::transform<FilePathIds>(fileInfos, [&](const QFileInfo &fileInfo) {
        return m_filePathCache.filePathId(FilePath{fileInfo.path()});
    });

    std::sort(filePathIds.begin(), filePathIds.end());

    return filePathIds;
}

long long FileSystem::lastModified(FilePathId filePathId) const
{
    QFileInfo fileInfo(QString(m_filePathCache.filePath(filePathId)));

    fileInfo.refresh();

    if (fileInfo.exists())
        return fileInfo.lastModified().toMSecsSinceEpoch() / 1000;

    return 0;
}

void FileSystem::remove(const FilePathIds &filePathIds)
{
    for (FilePathId filePathId : filePathIds)
        QFile::remove(QString{m_filePathCache.filePath(filePathId)});
}

} // namespace ClangBackEnd
