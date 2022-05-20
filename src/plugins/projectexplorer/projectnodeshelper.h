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

#pragma once

#include "projectnodes.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

namespace ProjectExplorer {

template<typename Result>
QList<FileNode *> scanForFiles(QFutureInterface<Result> &future,
                               const Utils::FilePath &directory,
                               const std::function<FileNode *(const Utils::FilePath &)> factory);

namespace Internal {

template<typename Result>
QList<FileNode *> scanForFilesRecursively(
    QFutureInterface<Result> &future,
    double progressStart,
    double progressRange,
    const Utils::FilePath &directory,
    const std::function<FileNode *(const Utils::FilePath &)> factory,
    QSet<QString> &visited,
    const QList<Core::IVersionControl *> &versionControls)
{
    QList<FileNode *> result;

    const QDir baseDir = QDir(directory.toString());

    // Do not follow directory loops:
    const int visitedCount = visited.count();
    visited.insert(baseDir.canonicalPath());
    if (visitedCount == visited.count())
        return result;

    const QFileInfoList entries = baseDir.entryInfoList(QStringList(),
                                                        QDir::AllEntries | QDir::NoDotAndDotDot);
    double progress = 0;
    const double progressIncrement = progressRange / static_cast<double>(entries.count());
    int lastIntProgress = 0;
    for (const QFileInfo &entry : entries) {
        if (future.isCanceled())
            return result;

        const Utils::FilePath entryName = Utils::FilePath::fromString(entry.absoluteFilePath());
        if (!Utils::contains(versionControls, [&entryName](const Core::IVersionControl *vc) {
                return vc->isVcsFileOrDirectory(entryName);
            })) {
            if (entry.isDir())
                result.append(scanForFilesRecursively(future,
                                                      progress,
                                                      progressIncrement,
                                                      entryName,
                                                      factory,
                                                      visited,
                                                      versionControls));
            else if (FileNode *node = factory(entryName))
                result.append(node);
        }
        progress += progressIncrement;
        const int intProgress = std::min(static_cast<int>(progressStart + progress),
                                         future.progressMaximum());
        if (lastIntProgress < intProgress) {
            future.setProgressValue(intProgress);
            lastIntProgress = intProgress;
        }
    }
    future.setProgressValue(
        std::min(static_cast<int>(progressStart + progressRange), future.progressMaximum()));
    return result;
}
} // namespace Internal

template<typename Result>
QList<FileNode *> scanForFiles(QFutureInterface<Result> &future,
                               const Utils::FilePath &directory,
                               const std::function<FileNode *(const Utils::FilePath &)> factory)
{
    QSet<QString> visited;
    future.setProgressRange(0, 1000000);
    return Internal::scanForFilesRecursively(future,
                                             0.0,
                                             1000000.0,
                                             directory,
                                             factory,
                                             visited,
                                             Core::VcsManager::versionControls());
}

} // namespace ProjectExplorer
