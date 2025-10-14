// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "efswfilesystemwatcher.h"

#include "projectstoragetracing.h"

#include <efsw/efsw.hpp>

#include <filesystem>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using ProjectStorageTracing::category;

class EfswFileSystemWatcher::Listener : public efsw::FileWatchListener
{
public:
    Listener(EfswFileSystemWatcher *watcher)
        : watcher(watcher)
    {
        NanotraceHR::Tracer tracer{"efsw file system watcher listener constructor", category()};
    }

    void handleFileAction(efsw::WatchID,
                          const std::string &dir,
                          const std::string &filename,
                          efsw::Action,
                          std::string) override;

private:
    EfswFileSystemWatcher *watcher;
};

void EfswFileSystemWatcher::Listener::handleFileAction(efsw::WatchID,
                                                       const std::string &dir,
                                                       const std::string &filename,
                                                       efsw::Action action,
                                                       std::string)
{
    NanotraceHR::Tracer tracer{"efsw file system watcher listener handle file action",
                               category(),
                               keyValue("filename", filename)};

    std::filesystem::path directory{dir};

    QString directoryPath = QString::fromStdU16String(directory.lexically_normal().generic_u16string());
    if (directoryPath.back() == u'/')
        directoryPath.chop(1);

    if (action == efsw::Action::Delete or action == efsw::Action::Moved) {
        tracer.tick("file deleted");

        auto file = directory / filename;

        QString removedDirectoryPath = QString::fromStdU16String(
            file.lexically_normal().generic_u16string());
        if (removedDirectoryPath.back() == u'/')
            removedDirectoryPath.chop(1);

        if (watcher->isWatched(removedDirectoryPath)) {
            tracer.tick("directory deleted", keyValue("path", removedDirectoryPath));

            QMetaObject::invokeMethod(watcher, [=, this]() {
                emit watcher->directoryRemoved(removedDirectoryPath);
            });
        }
    }

    tracer.tick("directory changed", keyValue("directory", directoryPath));

    QMetaObject::invokeMethod(watcher, [=, this]() { emit watcher->directoryChanged(directoryPath); });
}

EfswFileSystemWatcher::EfswFileSystemWatcher()
    : listener(std::make_unique<Listener>(this))
    , fileWatcher{std::make_unique<efsw::FileWatcher>()}
{
    NanotraceHR::Tracer tracer{"efsw file system watcher constructor", category()};
}

void EfswFileSystemWatcher::addPaths(const QStringList &paths)
{
    NanotraceHR::Tracer tracer{"efsw file system watcher add paths", category()};
    for (const QString &path : paths)
        addPath(path);

    fileWatcher->watch();
}

void EfswFileSystemWatcher::removePaths(const QStringList &paths)
{
    NanotraceHR::Tracer tracer{"efsw file system watcher remove paths", category()};

    for (const QString &path : paths)
        removePath(path);
}

EfswFileSystemWatcher::~EfswFileSystemWatcher()
{
    std::lock_guard lock{mutex};

    watchedPaths.clear();
}

namespace {
auto emplaceWatchedPath(std::set<QString> &watchedPaths, const QString &path, std::mutex &mutex)
{
    std::lock_guard lock{mutex};

    return watchedPaths.emplace(path);
}

bool eraseWatchedPath(std::set<QString> &watchedPaths, const QString &path, std::mutex &mutex)
{
    std::lock_guard lock{mutex};

    return watchedPaths.erase(path);
}

void eraseWatchedPathIterator(std::set<QString> &watchedPaths, auto &iterator, std::mutex &mutex)
{
    std::lock_guard lock{mutex};

    watchedPaths.erase(iterator);
}

} // namespace

void EfswFileSystemWatcher::addPath(const QString &path)
{
    NanotraceHR::Tracer tracer{"efsw file system watcher add path", category(), keyValue("path", path)};

    auto [iter, inserted] = emplaceWatchedPath(watchedPaths, path, mutex);

    if (inserted) {
        tracer.tick("inserted");

        std::filesystem::path nativePath{path.toStdString()};
        efsw::WatchID id = fileWatcher->addWatch(nativePath.string(), listener.get());
        if (id == efsw::Errors::FileNotFound) {
            eraseWatchedPathIterator(watchedPaths, iter, mutex);
            tracer.tick("not found");
        }
    }
    fileWatcher->watch();
}

void EfswFileSystemWatcher::removePath(const QString &path)
{
    NanotraceHR::Tracer tracer{"efsw file system watcher remove path",
                               category(),
                               keyValue("path", path)};

    if (eraseWatchedPath(watchedPaths, path, mutex)) {
        tracer.tick("erased");
        fileWatcher->removeWatch(path.toStdString());
    }
}

bool EfswFileSystemWatcher::isWatched(const QString &path) const
{
    std::lock_guard lock{mutex};

    return watchedPaths.contains(path);
}

} // namespace QmlDesigner
