// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "projectnodes.h"

#include <utils/filepath.h>
#include <utils/mimeutils.h>

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

#include <functional>

namespace Core { class IVersionControl; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT TreeScanner : public QObject
{
    Q_OBJECT

public:
    struct Result
    {
        std::shared_ptr<FolderNode> folderNode;
        QList<FileNode *> allFiles;
    };
    using Future = QFuture<Result>;
    using FutureWatcher = QFutureWatcher<Result>;
    using Promise = QPromise<Result>;

    using FileFilter = std::function<bool(const Utils::MimeType &, const Utils::FilePath &)>;
    using FileTypeFactory = std::function<ProjectExplorer::FileType(const Utils::MimeType &, const Utils::FilePath &)>;

    explicit TreeScanner(QObject *parent = nullptr);
    ~TreeScanner() override;

    // Start scanning in given directory
    bool asyncScanForFiles(const Utils::FilePath& directory);

    // Setup filter for ignored files
    void setFilter(FileFilter filter);

    // Setup factory for file types
    void setTypeFactory(FileTypeFactory factory);

    Future future() const;
    bool isFinished() const;

    // Takes not-owning result
    Result result() const;
    // Takes owning of result
    Result release();
    // Clear scan results
    void reset();

    // Standard filters helpers
    static bool isWellKnownBinary(const Utils::MimeType &mimeType, const Utils::FilePath &fn);
    static bool isMimeBinary(const Utils::MimeType &mimeType, const Utils::FilePath &fn);

    // Standard file factory
    static ProjectExplorer::FileType genericFileType(const Utils::MimeType &mdb, const Utils::FilePath& fn);

signals:
    void finished();

private:
    static void scanForFiles(Promise &fi, const Utils::FilePath &directory,
                             const FileFilter &filter, const FileTypeFactory &factory);

private:
    FileFilter m_filter;
    FileTypeFactory m_factory;

    FutureWatcher m_futureWatcher;
    Future m_scanFuture;
};

} // namespace ProjectExplorer
