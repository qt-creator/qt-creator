// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treescanner.h"

#include "projecttree.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/synchronizedvalue.h>

#include <qapplicationstatic.h>

#include <memory>

using namespace Utils;

namespace ProjectExplorer {

TreeScanner::TreeScanner(QObject *parent) : QObject(parent)
{
    m_factory = TreeScanner::genericFileType;
    m_filter = [](const MimeType &mimeType, const FilePath &fn) {
        return isWellKnownBinary(fn) && isMimeBinary(mimeType);
    };

    connect(&m_futureWatcher, &FutureWatcher::finished, this, &TreeScanner::finished);
}

TreeScanner::~TreeScanner()
{
    disconnect(&m_futureWatcher, nullptr, nullptr, nullptr); // Do not trigger signals anymore!

    if (!m_futureWatcher.isFinished()) {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }
}

bool TreeScanner::asyncScanForFiles(const FilePath &directory)
{
    if (!m_futureWatcher.isFinished())
        return false;

    m_scanFuture = Utils::asyncRun(
        [directory, filter = m_filter, dirFilter = m_dirFilter, factory = m_factory](
            Promise &promise) {
            TreeScanner::scanForFiles(promise, directory, filter, dirFilter, factory);
        });
    m_futureWatcher.setFuture(m_scanFuture);

    return true;
}

void TreeScanner::setFilter(TreeScanner::Filter filter)
{
    if (isFinished())
        m_filter = filter;
}

void TreeScanner::setDirFilter(QDir::Filters dirFilter)
{
    if (isFinished())
        m_dirFilter = dirFilter;
}

void TreeScanner::setTypeFactory(TreeScanner::FileTypeFactory factory)
{
    if (isFinished())
        m_factory = factory;
}

bool TreeScanner::isFinished() const
{
    return m_futureWatcher.isFinished();
}

TreeScanner::Result TreeScanner::release()
{
    if (isFinished() && m_scanFuture.resultCount() > 0) {
        Result result = m_scanFuture.takeResult();
        m_scanFuture = Future();
        return result;
    }
    m_scanFuture = Future();
    return {};
}

bool TreeScanner::isWellKnownBinary(const FilePath &fn)
{
    return fn.endsWith(QLatin1String(".a")) ||
            fn.endsWith(QLatin1String(".o")) ||
            fn.endsWith(QLatin1String(".d")) ||
            fn.endsWith(QLatin1String(".exe")) ||
            fn.endsWith(QLatin1String(".dll")) ||
            fn.endsWith(QLatin1String(".obj")) ||
            fn.endsWith(QLatin1String(".elf"));
}

bool TreeScanner::isMimeBinary(const MimeType &mimeType)
{
    bool isBinary = false;
    if (mimeType.isValid()) {
        QStringList mimes;
        mimes << mimeType.name() << mimeType.allAncestors();
        isBinary = !mimes.contains(QLatin1String("text/plain"));
    }
    return isBinary;
}

using MimeBinaryCache = SynchronizedValue<QHash<QString, bool>>;
Q_APPLICATION_STATIC(MimeBinaryCache, s_mimeBinaryCache);

bool TreeScanner::isMimeTypeIgnored(const MimeType &mimeType)
{
    if (auto it = s_mimeBinaryCache->get(
            [mimeType](const QHash<QString, bool> &cache) -> std::optional<bool> {
                auto cache_it = cache.find(mimeType.name());
                if (cache_it != cache.end())
                    return *cache_it;
                return std::nullopt;
            })) {
        return *it;
    }

    const bool isIgnored = TreeScanner::isMimeBinary(mimeType);
    s_mimeBinaryCache->writeLocked()->insert(mimeType.name(), isIgnored);
    return isIgnored;
}

FileType TreeScanner::genericFileType(const MimeType &mimeType)
{
    return Node::fileTypeForMimeType(mimeType);
}

struct DirectoryScanResult
{
    std::vector<std::unique_ptr<FileNode>> nodes;
    std::vector<std::unique_ptr<FolderNode>> subDirectories;
};

static DirectoryScanResult scanForFilesImpl(
    const QFuture<void> &future,
    const FilePath &directory,
    QDir::Filters filter,
    const std::function<FileNode *(const FilePath &)> &factory,
    const QList<Core::IVersionControl *> &versionControls)
{
    DirectoryScanResult result;

    const FilePaths entries = directory.dirEntries(filter);
    for (const FilePath &entry : entries) {
        if (future.isCanceled())
            return result;

        if (Utils::anyOf(versionControls, [entry](const Core::IVersionControl *vc) {
                return vc->isVcsFileOrDirectory(entry);
            })) {
            continue;
        }

        if (entry.isDir())
            result.subDirectories.emplace_back(new FolderNode(entry));
        else if (FileNode *node = factory(entry))
            result.nodes.emplace_back(node);
    }
    return result;
}

static const MimeType &directoryMimeType()
{
    static const MimeType mimeType = Utils::mimeTypeForName("inode/directory");
    return mimeType;
}

static bool sortByPath(const std::unique_ptr<FileNode> &a, const std::unique_ptr<FileNode> &b)
{
    return a->filePath() < b->filePath();
}

static TreeScanner::Result scanForFilesHelper(
    TreeScanner::Promise &promise,
    const FilePath &directory,
    QDir::Filters dirfilter,
    const TreeScanner::Filter &filter,
    const std::function<FileNode *(const FilePath &)> &factory)
{
    const QFuture<void> future(promise.future());

    const int progressRange = 1000000;
    const QList<Core::IVersionControl *> &versionControls = Core::VcsManager::versionControls();
    promise.setProgressRange(0, progressRange);

    QSet<FilePath> visited;
    DirectoryScanResult result = scanForFilesImpl(future, directory, dirfilter, factory,
                                                  versionControls);
    if (promise.isCanceled())
        return {};

    TreeScanner::Result finalResult;
    finalResult.allFiles = std::move(result.nodes);
    for (auto &fileNode : finalResult.allFiles)
        finalResult.firstLevelNodes.emplace_back(fileNode->clone());

    const int progressIncrement = int(
        progressRange / static_cast<double>(finalResult.allFiles.size() + result.subDirectories.size()));
    promise.setProgressValue(int(finalResult.allFiles.size() * progressIncrement));
    QList<QPair<FolderNode *, int>> subDirectories;
    auto addSubDirectories = [&](std::vector<std::unique_ptr<FolderNode>> &&subdirs, FolderNode * parent, int progressIncrement) {
        for (auto &subdir : subdirs) {
            if (Utils::insert(visited, subdir->filePath().canonicalPath())
                && !(filter && filter(directoryMimeType(), subdir->filePath()))) {
                subDirectories.append(qMakePair(subdir.get(), progressIncrement));
                subdir->setDisplayName(subdir->filePath().fileName());
                if (parent)
                    parent->addNode(std::move(subdir));
                else
                    finalResult.firstLevelNodes.emplace_back(std::move(subdir));
            } else {
                promise.setProgressValue(future.progressValue() + progressIncrement);
            }
        }
    };
    addSubDirectories(std::move(result.subDirectories), nullptr, progressIncrement);

    if (promise.isCanceled())
        return {};

    while (!subDirectories.isEmpty()) {
        using namespace QtTaskTree;
        const ListIterator iterator(subDirectories);
        subDirectories.clear();

        auto onSetup = [&, iterator](Async<DirectoryScanResult> &task) {
            task.setConcurrentCallData(
                scanForFilesImpl,
                future,
                iterator->first->filePath(),
                dirfilter,
                factory,
                versionControls);
        };

        auto onDone = [&, iterator](const Async<DirectoryScanResult> &task) {
            const int progressRange = iterator->second;
            DirectoryScanResult result = task.takeResult();
            for (auto &fileNode : result.nodes)
                finalResult.allFiles.emplace_back(std::move(fileNode));
            const qsizetype subDirCount = result.subDirectories.size();
            if (iterator->first) {
                for (auto &fn : result.nodes)
                    iterator->first->addNode(std::unique_ptr<FileNode>(fn->clone()));
            }
            if (subDirCount == 0) {
                promise.setProgressValue(future.progressValue() + progressRange);
            } else {
                const qsizetype fileCount = result.nodes.size();
                const int increment = int(
                    progressRange / static_cast<double>(fileCount + subDirCount));
                promise.setProgressValue(future.progressValue() + increment * fileCount);
                addSubDirectories(std::move(result.subDirectories), iterator->first, increment);
            }
        };

        const Group recipe = For (iterator) >> Do {
            HostOsInfo::isLinuxHost() ? ParallelLimit(2) : parallelIdealThreadCountLimit,
            AsyncTask<DirectoryScanResult>(onSetup, onDone)
        };
        QTaskTree::runBlocking(recipe, future);

        if (promise.isCanceled())
            return {};
    }

    Utils::sort(finalResult.allFiles, sortByPath);
    return finalResult;
}

void TreeScanner::scanForFiles(
    Promise &promise,
    const FilePath &directory,
    const Filter &filter,
    QDir::Filters dirFilter,
    const FileTypeFactory &factory)
{
    Result result = scanForFilesHelper(
        promise,
        directory,
        dirFilter,
        filter,
        [&filter, &factory](const FilePath &fn) -> FileNode * {
            const MimeType mimeType = Utils::mimeTypesForFileName(fn.path()).value(0);

            // Skip some files during scan.
            if (filter && filter(mimeType, fn))
                return nullptr;

            // Type detection
            FileType type = FileType::Unknown;
            if (factory)
                type = factory(mimeType);

            return new FileNode(fn, type);
        });

    promise.setProgressValue(promise.future().progressMaximum());
    promise.addResult(std::move(result));
}

} // namespace ProjectExplorer
