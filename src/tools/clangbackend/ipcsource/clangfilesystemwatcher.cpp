/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangfilesystemwatcher.h"

#include "translationunits.h"

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

ClangFileSystemWatcher::ClangFileSystemWatcher(TranslationUnits &translationUnits)
    : translationUnits(translationUnits)
{
    connect(&watcher,
            &QFileSystemWatcher::fileChanged,
            this,
            &ClangFileSystemWatcher::updateTranslationUnitsWithChangedDependencies);
}

void ClangFileSystemWatcher::addFiles(const QSet<Utf8String> &filePaths)
{
    const auto existingFiles = filterExistingFiles(toStringList(filePaths));

    if (!existingFiles.isEmpty())
        watcher.addPaths(existingFiles);
}

void ClangFileSystemWatcher::updateTranslationUnitsWithChangedDependencies(const QString &filePath)
{
    translationUnits.updateTranslationUnitsWithChangedDependency(filePath);

    emit fileChanged(filePath);
}

} // namespace ClangBackEnd
