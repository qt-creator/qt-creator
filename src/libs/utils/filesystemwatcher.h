// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QObject>

namespace Utils {

// Documentation inside.
class QTCREATOR_UTILS_EXPORT FileSystemWatcher final : public QObject
{
    Q_OBJECT

public:
    enum WatchMode
    {
        WatchModifiedDate,
        WatchAllChanges
    };

    explicit FileSystemWatcher(QObject *parent = nullptr);
    explicit FileSystemWatcher(int id, QObject *parent = nullptr);
    ~FileSystemWatcher() override;

    void clear();

    void addFile(const Utils::FilePath &file, WatchMode wm);
    void addFiles(const Utils::FilePaths &files, WatchMode wm);

    void removeFile(const Utils::FilePath &file);
    void removeFiles(const Utils::FilePaths &files);

    bool watchesFile(const Utils::FilePath &file) const;
    Utils::FilePaths files() const;

    void addDirectory(const Utils::FilePath &dir, WatchMode wm);
    void addDirectories(const Utils::FilePaths &dirs, WatchMode wm);

    void removeDirectory(const Utils::FilePath &dir);
    void removeDirectories(const Utils::FilePaths &dirs);

    bool watchesDirectory(const Utils::FilePath &dir) const;

    Utils::FilePaths directories() const;

signals:
    void fileChanged(const Utils::FilePath &path);
    void directoryChanged(const Utils::FilePath &path);

private:
    class FileSystemWatcherPrivate *d;
};

} // namespace Utils
