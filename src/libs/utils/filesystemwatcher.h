// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QObject>

namespace Utils {
class FileSystemWatcherPrivate;

// Documentation inside.
class QTCREATOR_UTILS_EXPORT FileSystemWatcher : public QObject
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

    // Good to use in new code:
    void addFile(const Utils::FilePath &file, WatchMode wm);
    void addFiles(const Utils::FilePaths &files, WatchMode wm);

    void removeFile(const Utils::FilePath &file);
    void removeFiles(const Utils::FilePaths &files);

    bool watchesFile(const Utils::FilePath &file) const;
    Utils::FilePaths filePaths() const;

    void addDirectory(const Utils::FilePath &file, WatchMode wm);
    void addDirectories(const Utils::FilePaths &files, WatchMode wm);

    void removeDirectory(const Utils::FilePath &file);
    void removeDirectories(const Utils::FilePaths &files);

    bool watchesDirectory(const Utils::FilePath &file) const;

    Utils::FilePaths directoryPaths() const;

    // Phase out:
    void addFile(const QString &file, WatchMode wm);
    void addFiles(const QStringList &files, WatchMode wm);

    void removeFile(const QString &file);
    void removeFiles(const QStringList &files);

    bool watchesFile(const QString &file) const;
    QStringList files() const;

    void addDirectory(const QString &file, WatchMode wm);
    void addDirectories(const QStringList &files, WatchMode wm);

    void removeDirectories(const QStringList &files);

    bool watchesDirectory(const QString &file) const;
    QStringList directories() const;

signals:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    void init();
    void slotFileChanged(const QString &path);
    void slotDirectoryChanged(const QString &path);

    FileSystemWatcherPrivate *d;
};

} // namespace Utils
