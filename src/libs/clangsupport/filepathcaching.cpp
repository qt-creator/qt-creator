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

#include "filepathcaching.h"

namespace ClangBackEnd {

FilePathId FilePathCaching::filePathId(FilePathView filePath) const
{
    return m_cache.filePathId(filePath);
}

FilePath FilePathCaching::filePath(FilePathId filePathId) const
{
    return m_cache.filePath(filePathId);
}

DirectoryPathId FilePathCaching::directoryPathId(Utils::SmallStringView directoryPath) const
{
    return m_cache.directoryPathId(directoryPath);
}

Utils::PathString FilePathCaching::directoryPath(DirectoryPathId directoryPathId) const
{
    return m_cache.directoryPath(directoryPathId);
}

DirectoryPathId FilePathCaching::directoryPathId(FilePathId filePathId) const
{
    return m_cache.directoryPathId(filePathId);
}

void FilePathCaching::addFilePaths(const FilePaths &filePaths)
{
    m_cache.addFilePaths(filePaths);
}

void FilePathCaching::populateIfEmpty()
{
    m_cache.populateIfEmpty();
}

FilePathId CopyableFilePathCaching::filePathId(FilePathView filePath) const
{
    return m_cache.filePathId(filePath);
}

FilePath CopyableFilePathCaching::filePath(FilePathId filePathId) const
{
    return m_cache.filePath(filePathId);
}

DirectoryPathId CopyableFilePathCaching::directoryPathId(Utils::SmallStringView directoryPath) const
{
    return m_cache.directoryPathId(directoryPath);
}

Utils::PathString CopyableFilePathCaching::directoryPath(DirectoryPathId directoryPathId) const
{
    return m_cache.directoryPath(directoryPathId);
}

DirectoryPathId CopyableFilePathCaching::directoryPathId(FilePathId filePathId) const
{
    return m_cache.directoryPathId(filePathId);
}

void CopyableFilePathCaching::addFilePaths(const FilePaths &filePaths)
{
    m_cache.addFilePaths(filePaths);
}

void CopyableFilePathCaching::populateIfEmpty()
{
    m_cache.populateIfEmpty();
}

} // namespace ClangBackEnd
