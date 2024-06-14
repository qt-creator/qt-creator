// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectnodes.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QPromise>

namespace ProjectExplorer {
namespace Internal {

struct DirectoryScanResult
{
    QList<FileNode *> nodes;
    Utils::FilePaths subDirectories;
};

template<typename Result>
DirectoryScanResult scanForFiles(
    QPromise<Result> &promise,
    const Utils::FilePath &directory,
    const QDir::Filters &filter,
    const std::function<FileNode *(const Utils::FilePath &)> factory,
    const QList<Core::IVersionControl *> &versionControls)
{
    DirectoryScanResult result;

    const Utils::FilePaths entries = directory.dirEntries(filter);
    for (const Utils::FilePath &entry : entries) {
        if (promise.isCanceled())
            return result;

        if (Utils::anyOf(versionControls, [entry](const Core::IVersionControl *vc) {
                return vc->isVcsFileOrDirectory(entry);
            })) {
            continue;
        }

        if (entry.isDir())
            result.subDirectories.append(entry);
        else if (FileNode *node = factory(entry))
            result.nodes.append(node);
    }

    return result;
}

template<typename Result>
QList<FileNode *> scanForFilesRecursively(
    QPromise<Result> &promise,
    double progressRange,
    const Utils::FilePath &directory,
    const QDir::Filters &filter,
    const std::function<FileNode *(const Utils::FilePath &)> factory,
    const QList<Core::IVersionControl *> &versionControls)
{
    QSet<Utils::FilePath> visited;
    const DirectoryScanResult result
        = scanForFiles(promise, directory, filter, factory, versionControls);
    QList<FileNode *> fileNodes = result.nodes;
    const double progressIncrement = progressRange
                                     / static_cast<double>(
                                         fileNodes.count() + result.subDirectories.count());
    promise.setProgressValue(fileNodes.count() * progressIncrement);
    QList<QPair<Utils::FilePath, int>> subDirectories;
    auto addSubDirectories = [&](const Utils::FilePaths &subdirs, int progressIncrement) {
        for (const Utils::FilePath &subdir : subdirs) {
            if (Utils::insert(visited, subdir.canonicalPath()))
                subDirectories.append(qMakePair(subdir, progressIncrement));
            else
                promise.setProgressValue(promise.future().progressValue() + progressIncrement);
        }
    };
    addSubDirectories(result.subDirectories, progressIncrement);

    while (!subDirectories.isEmpty()) {
        using namespace Tasking;
        LoopList iterator(subDirectories);
        subDirectories.clear();

        auto onSetup = [&, iterator](Utils::Async<DirectoryScanResult> &task) {
            task.setConcurrentCallData(
                [&filter, &factory, &promise, &versionControls, subdir = iterator->first](
                    QPromise<DirectoryScanResult> &p) {
                    p.addResult(scanForFiles(promise, subdir, filter, factory, versionControls));
                });
        };

        auto onDone = [&, iterator](const Utils::Async<DirectoryScanResult> &task) {
            const int progressRange = iterator->second;
            const DirectoryScanResult result = task.result();
            fileNodes.append(result.nodes);
            const int subDirCount = result.subDirectories.count();
            if (subDirCount == 0) {
                promise.setProgressValue(promise.future().progressValue() + progressRange);
            } else {
                const int fileCount = result.nodes.count();
                const int increment = progressRange / static_cast<double>(fileCount + subDirCount);
                promise.setProgressValue(
                    promise.future().progressValue() + increment * fileCount);
                addSubDirectories(result.subDirectories, increment);
            }
        };

        const Group group{
            Utils::HostOsInfo::isLinuxHost() ? parallelLimit(2) : parallelIdealThreadCountLimit,
            iterator,
            Utils::AsyncTask<DirectoryScanResult>(onSetup, onDone)
        };
        TaskTree::runBlocking(group);
    }
    return fileNodes;
}

} // namespace Internal

template<typename Result>
QList<FileNode *> scanForFiles(
    QPromise<Result> &promise,
    const Utils::FilePath &directory,
    const QDir::Filters &filter,
    const std::function<FileNode *(const Utils::FilePath &)> factory)
{
    promise.setProgressRange(0, 1000000);
    return Internal::scanForFilesRecursively(promise,
                                             1000000.0,
                                             directory,
                                             filter,
                                             factory,
                                             Core::VcsManager::versionControls());
}

} // namespace ProjectExplorer
