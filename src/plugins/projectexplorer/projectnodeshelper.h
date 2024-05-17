// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectnodes.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QPromise>

namespace ProjectExplorer {
namespace Internal {

template<typename Result>
QList<FileNode *> scanForFilesRecursively(
    QPromise<Result> &promise,
    double progressStart,
    double progressRange,
    const Utils::FilePath &directory,
    const QDir::Filters &filter,
    const std::function<FileNode *(const Utils::FilePath &)> factory,
    QSet<Utils::FilePath> &visited,
    const QList<Core::IVersionControl *> &versionControls)
{
    QList<FileNode *> result;

    // Do not follow directory loops:
    if (!Utils::insert(visited, directory.canonicalPath()))
        return result;

    const Utils::FilePaths entries = directory.dirEntries(filter);
    double progress = 0;
    const double progressIncrement = progressRange / static_cast<double>(entries.count());
    int lastIntProgress = 0;
    for (const Utils::FilePath &entry : entries) {
        if (promise.isCanceled())
            return result;

        if (!Utils::contains(versionControls, [entry](const Core::IVersionControl *vc) {
                return vc->isVcsFileOrDirectory(entry);
            })) {
            if (entry.isDir()) {
                result.append(scanForFilesRecursively(promise,
                                                      progress,
                                                      progressIncrement,
                                                      entry,
                                                      filter,
                                                      factory,
                                                      visited,
                                                      versionControls));
            } else if (FileNode *node = factory(entry)) {
                result.append(node);
            }
        }
        progress += progressIncrement;
        const int intProgress = std::min(static_cast<int>(progressStart + progress),
                                         promise.future().progressMaximum());
        if (lastIntProgress < intProgress) {
            promise.setProgressValue(intProgress);
            lastIntProgress = intProgress;
        }
    }
    promise.setProgressValue(std::min(static_cast<int>(progressStart + progressRange),
                                      promise.future().progressMaximum()));
    return result;
}
} // namespace Internal

template<typename Result>
QList<FileNode *> scanForFiles(
    QPromise<Result> &promise,
    const Utils::FilePath &directory,
    const QDir::Filters &filter,
    const std::function<FileNode *(const Utils::FilePath &)> factory)
{
    QSet<Utils::FilePath> visited;
    promise.setProgressRange(0, 1000000);
    return Internal::scanForFilesRecursively(promise,
                                             0.0,
                                             1000000.0,
                                             directory,
                                             filter,
                                             factory,
                                             visited,
                                             Core::VcsManager::versionControls());
}

} // namespace ProjectExplorer
