// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "efswfilesystemwatcher.h"

#include <efsw/efsw.hpp>

#include <filesystem>

namespace QmlDesigner {

class EfswFileSystemWatcher::Listener : public efsw::FileWatchListener
{
public:
    Listener(EfswFileSystemWatcher *watcher)
        : watcher(watcher)
    {}

    void handleFileAction(efsw::WatchID,
                          const std::string &dir,
                          const std::string &filename,
                          efsw::Action,
                          std::string) override;

private:
    EfswFileSystemWatcher *watcher;
};

void EfswFileSystemWatcher::Listener::handleFileAction(
    efsw::WatchID, const std::string &dir, const std::string &filename, efsw::Action, std::string)
{
    std::filesystem::path directory{dir};
    auto file = directory / filename;

    QString directorPath = QString::fromStdU16String(directory.lexically_normal().generic_u16string());
    if (directorPath.back() == u'/')
        directorPath.chop(1);
    QString filePath = QString::fromStdU16String(file.lexically_normal().generic_u16string());

    QMetaObject::invokeMethod(watcher, [=, this]() { emit watcher->fileChanged(filePath); });
    QMetaObject::invokeMethod(watcher, [=, this]() { emit watcher->directoryChanged(directorPath); });
}

EfswFileSystemWatcher::EfswFileSystemWatcher()
    : fileWatcher{std::make_unique<efsw::FileWatcher>()}
    , listener(std::make_unique<Listener>(this))
{}

void EfswFileSystemWatcher::addPaths(const QStringList &paths)
{
    for (const QString &path : paths)
        addPath(path);

    fileWatcher->watch();
}

void EfswFileSystemWatcher::removePaths(const QStringList &paths)
{
    for (const QString &path : paths)
        removePath(path);
}

EfswFileSystemWatcher::~EfswFileSystemWatcher() = default;

void EfswFileSystemWatcher::addPath(const QString &path)
{
    auto [iter, inserted] = watchedPaths.emplace(path);
    if (inserted) {
        std::filesystem::path nativePath{path.toStdString()};
        efsw::WatchID id = fileWatcher->addWatch(nativePath.string(), listener.get());
        if (id == efsw::Errors::FileNotFound)
            watchedPaths.erase(iter);
    }
    fileWatcher->watch();
}

void EfswFileSystemWatcher::removePath(const QString &path)
{
    if (watchedPaths.erase(path))
        fileWatcher->removeWatch(path.toStdString());
}

} // namespace QmlDesigner
