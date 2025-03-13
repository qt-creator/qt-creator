// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemwatcher.h"

#include "globalfilechangeblocker.h"
#include "filepath.h"

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QLoggingCategory>

namespace Utils {

// Returns upper limit of file handles that can be opened by this process at
// once. (which is limited on MacOS, exceeding it will probably result in
// crashes).
static inline quint64 getFileLimit()
{
#ifdef Q_OS_MAC
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    return rl.rlim_cur; // quint64
#else
    return 0xFFFFFFFF;
#endif
}

/*!
    \class Utils::FileSystemWatcher
    \inmodule QtCreator
    \brief The FileSystemWatcher class is a file watcher that internally uses
           a centralized QFileSystemWatcher
           and enforces limits on Mac OS.

    \section1 Design Considerations

    Constructing/Destructing a QFileSystemWatcher is expensive. This can be
    worked around by using a centralized watcher.

    \note It is (still) possible to create several instances of a
    QFileSystemWatcher by passing an (arbitrary) integer id != 0 to the
    constructor. This allows separating watchers that
    easily exceed operating system limits from others (see below).

    \section1 Mac OS Specifics

    There is a hard limit on the number of file handles that can be open at
    one point per process on macOS (e.g. it is 2560 on Mac OS X Snow Leopard
    Server, as shown by \c{ulimit -a}). Opening one or several \c .qmlproject's
    with a large number of directories to watch easily exceeds this. The
    results are crashes later on, e.g. when threads cannot be created any more.

    This class implements a heuristic that the file system watcher used for
    \c .qmlproject files never uses more than half the number of available
    file handles. It also increases the number from \c rlim_cur to \c rlim_max
    - the old code in main.cpp failed, see last section in

    \l{http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man2/setrlimit.2.html}

    for details.
*/

static Q_LOGGING_CATEGORY(fileSystemWatcherLog, "qtc.utils.filesystemwatcher", QtInfoMsg)

// Centralized file watcher static data per integer id.
class FileSystemWatcherStaticData
{
public:
    FileSystemWatcherStaticData() :
        maxFileOpen(getFileLimit()) {}

    quint64 maxFileOpen;
    int m_objectCount = 0;
    QHash<FilePath, int> m_fileCount;
    QHash<FilePath, int> m_directoryCount;
    QFileSystemWatcher *m_watcher = nullptr;
};

using FileSystemWatcherStaticDataMap = QMap<int, FileSystemWatcherStaticData>;

Q_GLOBAL_STATIC(FileSystemWatcherStaticDataMap, fileSystemWatcherStaticDataMap)

class WatchEntry
{
public:
    using WatchMode = FileSystemWatcher::WatchMode;

    explicit WatchEntry(const FilePath &file, WatchMode wm) :
        watchMode(wm), modifiedTime(file.lastModified()) {}

    bool trigger(const FilePath &fileName);

    WatchMode watchMode = FileSystemWatcher::WatchAllChanges;
    QDateTime modifiedTime;
};

// Check if watch should trigger on signal considering watchmode.
bool WatchEntry::trigger(const FilePath &filePath)
{
    if (watchMode == FileSystemWatcher::WatchAllChanges)
        return true;
    // Modified changed?
    const QDateTime newModifiedTime = filePath.exists() ? filePath.lastModified() : QDateTime();
    if (newModifiedTime != modifiedTime) {
        modifiedTime = newModifiedTime;
        return true;
    }
    return false;
}

using WatchEntryMap = QHash<FilePath, WatchEntry>;

class FileSystemWatcherPrivate
{
public:
    explicit FileSystemWatcherPrivate(FileSystemWatcher *q, int id) : m_id(id), q(q)
    {
        QObject::connect(Utils::GlobalFileChangeBlocker::instance(),
                         &Utils::GlobalFileChangeBlocker::stateChanged,
                         q,
                         [this](bool blocked) { autoReloadPostponed(blocked); });
    }

    WatchEntryMap m_files;
    WatchEntryMap m_directories;

    QSet<FilePath> m_postponedFiles;
    QSet<FilePath> m_postponedDirectories;

    bool checkLimit() const;
    void fileChanged(const FilePath &path);
    void directoryChanged(const FilePath &path);

    const int m_id;
    FileSystemWatcherStaticData *m_staticData = nullptr;

private:
    void autoReloadPostponed(bool postponed);
    bool m_postponed = false;
    FileSystemWatcher *q;
};

bool FileSystemWatcherPrivate::checkLimit() const
{
    // We are potentially watching a _lot_ of directories. This might crash
    // qtcreator when we hit the upper limit.
    // Heuristic is therefore: Do not use more than half of the file handles
    // available in THIS watcher.
    return quint64(m_directories.size() + m_files.size()) <
           (m_staticData->maxFileOpen / 2);
}

void FileSystemWatcherPrivate::fileChanged(const FilePath &path)
{
    if (m_postponed)
        m_postponedFiles.insert(path);
    else
        emit q->fileChanged(path);
}

void FileSystemWatcherPrivate::directoryChanged(const FilePath &path)
{
    if (m_postponed)
        m_postponedDirectories.insert(path);
    else
        emit q->directoryChanged(path);
}

void FileSystemWatcherPrivate::autoReloadPostponed(bool postponed)
{
    if (m_postponed == postponed)
        return;
    m_postponed = postponed;
    if (!postponed) {
        for (const FilePath &file : std::as_const(m_postponedFiles))
            emit q->fileChanged(file);
        m_postponedFiles.clear();
        for (const FilePath &directory : std::as_const(m_postponedDirectories))
            emit q->directoryChanged(directory);
        m_postponedDirectories.clear();
    }
}

/*!
    Creates a file system watcher with the ID 0 and the owner \a parent.
*/

FileSystemWatcher::FileSystemWatcher(QObject *parent)
    : FileSystemWatcher(0, parent)
{}

/*!
    Creates a file system watcher with the ID \a id and the owner \a parent.
*/

FileSystemWatcher::FileSystemWatcher(int id, QObject *parent)
    : QObject(parent), d(new FileSystemWatcherPrivate(this, id))
{
    // Check for id in map/
    FileSystemWatcherStaticDataMap &map = *fileSystemWatcherStaticDataMap();
    FileSystemWatcherStaticDataMap::iterator it = map.find(d->m_id);
    if (it == map.end())
        it = map.insert(d->m_id, FileSystemWatcherStaticData());
    d->m_staticData = &it.value();

    if (!d->m_staticData->m_watcher) {
        d->m_staticData->m_watcher = new QFileSystemWatcher();
        qCDebug(fileSystemWatcherLog)
            << this << "Created watcher for id" << d->m_id;
    }
    ++(d->m_staticData->m_objectCount);
    connect(d->m_staticData->m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &file) {
        const FilePath filePath = FilePath::fromString(file);
        const auto it = d->m_files.find(filePath);
        if (it != d->m_files.end() && it.value().trigger(filePath)) {
            qCDebug(fileSystemWatcherLog)
                    << this << "triggers on file" << it.key()
                    << it.value().watchMode
                    << it.value().modifiedTime.toString(Qt::ISODate);

            if (!filePath.exists()) {
                const FilePath directory = filePath.parentDir();
                const int dirCount = ++d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount > 0);

                if (dirCount == 1)
                    d->m_staticData->m_watcher->addPath(directory.toFSPathString());
            }
            d->fileChanged(filePath);
        }
    });

    connect(d->m_staticData->m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &file) {
        const FilePath dir = FilePath::fromString(file);
        const auto it = d->m_directories.find(dir);
        if (it != d->m_directories.end() && it.value().trigger(dir)) {
            qCDebug(fileSystemWatcherLog)
                << this << "triggers on dir" << it.key()
                << it.value().watchMode
                << it.value().modifiedTime.toString(Qt::ISODate);
            d->directoryChanged(dir);
        }

        QStringList toReadd;
        for (const FilePath &entry : dir.dirEntries(QDir::Files)) {
            if (d->m_files.contains(entry))
                toReadd.append(entry.toFSPathString());
        }

        if (!toReadd.isEmpty()) {
            for (const QString &rejected : d->m_staticData->m_watcher->addPaths(toReadd))
                toReadd.removeOne(rejected);

            // If we've successfully added the file, that means it was deleted and replaced.
            for (const QString &dir : std::as_const(toReadd)) {
                const FilePath directory = FilePath::fromString(dir);
                const int dirCount = --d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount >= 0);

                if (!dirCount)
                    d->m_staticData->m_watcher->removePath(dir);

                d->fileChanged(directory);
            }
        }
    });
}

FileSystemWatcher::~FileSystemWatcher()
{
    clear();

    if (!fileSystemWatcherStaticDataMap.isDestroyed() && --(d->m_staticData->m_objectCount) == 0) {
        delete d->m_staticData->m_watcher;
        d->m_staticData->m_watcher = nullptr;
        d->m_staticData->m_fileCount.clear();
        d->m_staticData->m_directoryCount.clear();
        qCDebug(fileSystemWatcherLog)
            << this << "Deleted watcher" << d->m_id;
    }
    delete d;
}

bool FileSystemWatcher::watchesFile(const FilePath &file) const
{
    return d->m_files.contains(file);
}

void FileSystemWatcher::addFile(const FilePath &file, WatchMode wm)
{
    addFiles({file}, wm);
}

void FileSystemWatcher::addFiles(const FilePaths &filePaths, WatchMode wm)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "addFiles mode" << wm << filePaths
        << "limit currently:" << (d->m_files.size() + d->m_directories.size())
        << "of" << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    for (const FilePath &filePath : filePaths) {
        if (watchesFile(filePath)) {
            qWarning("FileSystemWatcher: File %s is already being watched",
                     qPrintable(filePath.toUserOutput()));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning("File %s is not watched: Too many file handles are already open (max is %u).",
                     qPrintable(filePath.toUserOutput()), unsigned(d->m_staticData->maxFileOpen));
            break;
        }

        d->m_files.insert(filePath, WatchEntry(filePath, wm));

        const int count = ++d->m_staticData->m_fileCount[filePath];
        QTC_CHECK(count > 0);

        if (count == 1) {
            toAdd << filePath.toFSPathString();

            if (!filePath.exists()) {
                const FilePath directory = filePath.parentDir();
                const int dirCount = ++d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount > 0);

                if (dirCount == 1)
                    toAdd << directory.toFSPathString();
            }
        }
    }

    if (!toAdd.isEmpty())
        d->m_staticData->m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeFile(const FilePath &file)
{
    removeFiles({file});
}

void FileSystemWatcher::removeFiles(const FilePaths &filePaths)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "removeFiles" << filePaths;
    QStringList toRemove;
    for (const FilePath &filePath : filePaths) {
        const QString file = filePath.toFSPathString();
        const auto it = d->m_files.constFind(filePath);
        if (it == d->m_files.constEnd()) {
            qWarning("FileSystemWatcher: File %s is not watched.",
                     qPrintable(filePath.toUserOutput()));
            continue;
        }
        d->m_files.erase(it);

        const int count = --(d->m_staticData->m_fileCount[filePath]);
        QTC_CHECK(count >= 0);

        if (!count) {
            toRemove << file;

            if (!filePath.exists()) {
                const FilePath directory = filePath.parentDir();
                const int dirCount = --d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount >= 0);

                if (!dirCount)
                    toRemove << directory.toFSPathString();
            }
        }
    }

    if (!toRemove.isEmpty())
        d->m_staticData->m_watcher->removePaths(toRemove);
}

void FileSystemWatcher::clear()
{
    if (!d->m_files.isEmpty())
        removeFiles(files());
    if (!d->m_directories.isEmpty())
        removeDirectories(directories());
}

FilePaths FileSystemWatcher::files() const
{
    return d->m_files.keys();
}

bool FileSystemWatcher::watchesDirectory(const FilePath &directory) const
{
    return d->m_directories.contains(directory);
}

void FileSystemWatcher::addDirectory(const FilePath &directory, WatchMode wm)
{
    addDirectories({directory}, wm);
}

void FileSystemWatcher::addDirectories(const FilePaths &directories, WatchMode wm)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "addDirectories mode" << wm << directories
        << "limit currently:" << (d->m_files.size() + d->m_directories.size())
        << "of" << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    for (const FilePath &dir : directories) {
        if (watchesDirectory(dir)) {
            qWarning("FileSystemWatcher: Directory %s is already being watched.",
                     qPrintable(dir.toUserOutput()));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning("Directory %s is not watched: Too many file handles are already open (max is %u).",
                     qPrintable(dir.toUserOutput()), unsigned(d->m_staticData->maxFileOpen));
            break;
        }

        d->m_directories.insert(dir, WatchEntry(dir, wm));

        const int count = ++d->m_staticData->m_directoryCount[dir];
        QTC_CHECK(count > 0);

        if (count == 1)
            toAdd << dir.toFSPathString();
    }

    if (!toAdd.isEmpty())
        d->m_staticData->m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeDirectory(const FilePath &file)
{
    removeDirectories({file});
}

void FileSystemWatcher::removeDirectories(const FilePaths &directories)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "removeDirectories" << directories;

    QStringList toRemove;
    for (const FilePath &dir : directories) {
        const auto it = d->m_directories.constFind(dir);
        if (it == d->m_directories.constEnd()) {
            qWarning("FileSystemWatcher: Directory %s is not watched.",
                     qPrintable(dir.toUserOutput()));
            continue;
        }
        d->m_directories.erase(it);

        const int count = --d->m_staticData->m_directoryCount[dir];
        QTC_CHECK(count >= 0);

        if (!count)
            toRemove << dir.toFSPathString();
    }
    if (!toRemove.isEmpty())
        d->m_staticData->m_watcher->removePaths(toRemove);
}

FilePaths FileSystemWatcher::directories() const
{
    return d->m_directories.keys();
}

} //Utils
