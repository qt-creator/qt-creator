// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "async.h"
#include "filepath.h"

#include <QObject>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Unarchiver : public QObject
{
    Q_OBJECT

public:
    Unarchiver();

    // Decides, by its name inside the archive, whether to extract an entry.
    // The name is the archive's own, as the extractor uses it. Entries refused
    // as unsafe never reach the filter. Skipping a directory entry leaves the
    // permissions of any kept child's parent at the default.
    // Runs in the worker thread.
    using Filter = std::function<bool(const QString &entry)>;

    void setArchive(const FilePath &archive);
    void setDestination(const FilePath &destination);
    void setFilter(const Filter &filter);

    Result<> result() const;

    void start();

    bool isDone() const;
    bool isCanceled() const;

signals:
    void started();
    void done(QtTaskTree::DoneResult result);
    void progress(const FilePath &path);

private:
    Async<Result<>> m_async;

    FilePath m_archive;
    FilePath m_destination;
    Filter m_filter;
};

using UnarchiverTask = QtTaskTree::QCustomTask<Unarchiver>;

// Decompress a single compressed stream held in memory (e.g. a gzipped log).
// Uses libarchive's "raw" format with automatic filter detection, so gzip,
// xz, bzip2, ... all work. Returns the uncompressed bytes or an error.
QTCREATOR_UTILS_EXPORT Result<QByteArray> gzipDecompress(const QByteArray &compressed);

} // namespace Utils
