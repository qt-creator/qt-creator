// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treescanner.h"

#include "projectnodeshelper.h"
#include "projecttree.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/async.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <memory>

namespace ProjectExplorer {

TreeScanner::TreeScanner(QObject *parent) : QObject(parent)
{
    m_factory = TreeScanner::genericFileType;
    m_filter = [](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
        return isWellKnownBinary(mimeType, fn) && isMimeBinary(mimeType, fn);
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

bool TreeScanner::asyncScanForFiles(const Utils::FilePath &directory)
{
    if (!m_futureWatcher.isFinished())
        return false;

    m_scanFuture = Utils::asyncRun(
        [directory, filter = m_filter, factory = m_factory] (Promise &promise) {
        TreeScanner::scanForFiles(promise, directory, filter, factory);
    });
    m_futureWatcher.setFuture(m_scanFuture);

    return true;
}

void TreeScanner::setFilter(TreeScanner::FileFilter filter)
{
    if (isFinished())
        m_filter = filter;
}

void TreeScanner::setTypeFactory(TreeScanner::FileTypeFactory factory)
{
    if (isFinished())
        m_factory = factory;
}

TreeScanner::Future TreeScanner::future() const
{
    return m_scanFuture;
}

bool TreeScanner::isFinished() const
{
    return m_futureWatcher.isFinished();
}

TreeScanner::Result TreeScanner::result() const
{
    if (isFinished())
        return m_scanFuture.result();
    return {};
}

TreeScanner::Result TreeScanner::release()
{
    if (isFinished() && m_scanFuture.resultCount() > 0) {
        auto result = m_scanFuture.result();
        m_scanFuture = Future();
        return result;
    }
    m_scanFuture = Future();
    return {};
}

void TreeScanner::reset()
{
    if (isFinished())
        m_scanFuture = Future();
}

bool TreeScanner::isWellKnownBinary(const Utils::MimeType & /*mdb*/, const Utils::FilePath &fn)
{
    return fn.endsWith(QLatin1String(".a")) ||
            fn.endsWith(QLatin1String(".o")) ||
            fn.endsWith(QLatin1String(".d")) ||
            fn.endsWith(QLatin1String(".exe")) ||
            fn.endsWith(QLatin1String(".dll")) ||
            fn.endsWith(QLatin1String(".obj")) ||
            fn.endsWith(QLatin1String(".elf"));
}

bool TreeScanner::isMimeBinary(const Utils::MimeType &mimeType, const Utils::FilePath &/*fn*/)
{
    bool isBinary = false;
    if (mimeType.isValid()) {
        QStringList mimes;
        mimes << mimeType.name() << mimeType.allAncestors();
        isBinary = !mimes.contains(QLatin1String("text/plain"));
    }
    return isBinary;
}

FileType TreeScanner::genericFileType(const Utils::MimeType &mimeType, const Utils::FilePath &/*fn*/)
{
    return Node::fileTypeForMimeType(mimeType);
}

static std::unique_ptr<FolderNode> createFolderNode(const Utils::FilePath &directory,
                                                    const QList<FileNode *> &allFiles)
{
    auto fileSystemNode = std::make_unique<FolderNode>(directory);
    for (const FileNode *fn : allFiles) {
        if (!fn->filePath().isChildOf(directory))
            continue;

        std::unique_ptr<FileNode> node(fn->clone());
        fileSystemNode->addNestedNode(std::move(node));
    }
    ProjectTree::applyTreeManager(fileSystemNode.get(), ProjectTree::AsyncPhase); // QRC nodes
    return fileSystemNode;
}

void TreeScanner::scanForFiles(Promise &promise, const Utils::FilePath& directory,
                               const FileFilter &filter, const FileTypeFactory &factory)
{
    QList<FileNode *> nodes = ProjectExplorer::scanForFiles(promise, directory,
                           [&filter, &factory](const Utils::FilePath &fn) -> FileNode * {
        const Utils::MimeType mimeType = Utils::mimeTypeForFile(fn);

        // Skip some files during scan.
        if (filter && filter(mimeType, fn))
            return nullptr;

        // Type detection
        FileType type = FileType::Unknown;
        if (factory)
            type = factory(mimeType, fn);

        return new FileNode(fn, type);
    });

    Utils::sort(nodes, ProjectExplorer::Node::sortByPath);

    promise.setProgressValue(promise.future().progressMaximum());
    Result result{createFolderNode(directory, nodes), nodes};

    promise.addResult(result);
}

} // namespace ProjectExplorer
