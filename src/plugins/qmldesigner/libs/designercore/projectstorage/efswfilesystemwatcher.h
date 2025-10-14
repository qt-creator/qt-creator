// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include <QObject>

#include <mutex>
#include <set>

namespace efsw {
class FileWatcher;
}

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT EfswFileSystemWatcher : public QObject
{
    Q_OBJECT

    class Listener;
    friend class Listener;

public:
    EfswFileSystemWatcher();
    ~EfswFileSystemWatcher();

    void addPaths(const QStringList &paths);
    void removePaths(const QStringList &paths);

signals:
    void directoryRemoved(const QString &path);
    void directoryChanged(const QString &path);

private:
    void addPath(const QString &path);
    void removePath(const QString &path);
    bool isWatched(const QString &path) const;

private:
    mutable std::mutex mutex;
    std::set<QString> watchedPaths;
    std::unique_ptr<Listener> listener;
    std::unique_ptr<efsw::FileWatcher> fileWatcher;
};

} // namespace QmlDesigner
