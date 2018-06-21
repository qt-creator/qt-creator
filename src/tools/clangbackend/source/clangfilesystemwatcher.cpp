/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangfilesystemwatcher.h"

#include "clangdocuments.h"

#include <utf8stringvector.h>

#include <QFileInfo>
#include <QStringList>

#include <algorithm>
#include <iterator>

namespace ClangBackEnd {

namespace {
QStringList toStringList(const QSet<Utf8String> &files)
{
    QStringList resultList;
    resultList.reserve(files.size());

    std::copy(files.cbegin(), files.cend(), std::back_inserter(resultList));

    return resultList;
}

QStringList filterExistingFiles(QStringList &&filePaths)
{
    auto fileExists = [] (const QString &filePath) {
        return QFileInfo::exists(filePath);
    };

    auto startOfNonExistingFilePaths = std::partition(filePaths.begin(),
                                                      filePaths.end(),
                                                      fileExists);

    filePaths.erase(startOfNonExistingFilePaths, filePaths.end());

    return filePaths;
}
}

ClangFileSystemWatcher::ClangFileSystemWatcher(Documents &documents)
    : documents(documents)
{
    connect(&watcher,
            &QFileSystemWatcher::fileChanged,
            this,
            &ClangFileSystemWatcher::updateDocumentsWithChangedDependencies);
}

void ClangFileSystemWatcher::addFiles(const QSet<Utf8String> &filePaths)
{
    const auto existingFiles = filterExistingFiles(toStringList(filePaths));

    if (!existingFiles.isEmpty())
        watcher.addPaths(existingFiles);
}

void ClangFileSystemWatcher::updateDocumentsWithChangedDependencies(const QString &filePath)
{
    documents.updateDocumentsWithChangedDependency(filePath);

    emit fileChanged(filePath);
}

} // namespace ClangBackEnd
