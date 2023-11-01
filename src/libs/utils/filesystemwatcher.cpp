// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemwatcher.h"

#include "algorithm.h"
#include "globalfilechangeblocker.h"
#include "filepath.h"

#include <QDir>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QLoggingCategory>

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
    one point per process on Mac OS X (e.g. it is 2560 on Mac OS X Snow Leopard
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

namespace Utils {

static Q_LOGGING_CATEGORY(fileSystemWatcherLog, "qtc.utils.filesystemwatcher", QtInfoMsg)

// Centralized file watcher static data per integer id.
class FileSystemWatcherStaticData
{
public:
    FileSystemWatcherStaticData() :
        maxFileOpen(getFileLimit()) {}

    quint64 maxFileOpen;
    int m_objectCount = 0;
    QHash<QString, int> m_fileCount;
    QHash<QString, int> m_directoryCount;
    QFileSystemWatcher *m_watcher = nullptr;
};

using FileSystemWatcherStaticDataMap = QMap<int, FileSystemWatcherStaticData>;

Q_GLOBAL_STATIC(FileSystemWatcherStaticDataMap, fileSystemWatcherStaticDataMap)

class WatchEntry
{
public:
    using WatchMode = FileSystemWatcher::WatchMode;

    explicit WatchEntry(const QString &file, WatchMode wm) :
        watchMode(wm), modifiedTime(QFileInfo(file).lastModified()) {}

    WatchEntry() = default;

    bool trigger(const QString &fileName);

    WatchMode watchMode = FileSystemWatcher::WatchAllChanges;
    QDateTime modifiedTime;
};

// Check if watch should trigger on signal considering watchmode.
bool WatchEntry::trigger(const QString &fileName)
{
    if (watchMode == FileSystemWatcher::WatchAllChanges)
        return true;
    // Modified changed?
    const QFileInfo fi(fileName);
    const QDateTime newModifiedTime = fi.exists() ? fi.lastModified() : QDateTime();
    if (newModifiedTime != modifiedTime) {
        modifiedTime = newModifiedTime;
        return true;
    }
    return false;
}

using WatchEntryMap = QHash<QString, WatchEntry>;

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

    QSet<QString> m_postponedFiles;
    QSet<QString> m_postponedDirectories;

    bool checkLimit() const;
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

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

void FileSystemWatcherPrivate::fileChanged(const QString &path)
{
    if (m_postponed)
        m_postponedFiles.insert(path);
    else
        emit q->fileChanged(path);
}

void FileSystemWatcherPrivate::directoryChanged(const QString &path)
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
        for (const QString &file : std::as_const(m_postponedFiles))
            emit q->fileChanged(file);
        m_postponedFiles.clear();
        for (const QString &directory : std::as_const(m_postponedDirectories))
            emit q->directoryChanged(directory);
        m_postponedDirectories.clear();
    }
}

/*!
    Creates a file system watcher with the ID 0 and the owner \a parent.
*/

FileSystemWatcher::FileSystemWatcher(QObject *parent) :
    QObject(parent), d(new FileSystemWatcherPrivate(this, 0))
{
    init();
}

/*!
    Creates a file system watcher with the ID \a id and the owner \a parent.
*/

FileSystemWatcher::FileSystemWatcher(int id, QObject *parent) :
    QObject(parent), d(new FileSystemWatcherPrivate(this, id))
{
    init();
}

void FileSystemWatcher::init()
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
    connect(d->m_staticData->m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileSystemWatcher::slotFileChanged);
    connect(d->m_staticData->m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileSystemWatcher::slotDirectoryChanged);
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

bool FileSystemWatcher::watchesFile(const QString &file) const
{
    return d->m_files.contains(file);
}

void FileSystemWatcher::addFile(const QString &file, WatchMode wm)
{
    addFiles(QStringList(file), wm);
}

void FileSystemWatcher::addFiles(const QStringList &files, WatchMode wm)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "addFiles mode" << wm << files
        << "limit currently:" << (d->m_files.size() + d->m_directories.size())
        << "of" << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    for (const QString &file : files) {
        if (watchesFile(file)) {
            qWarning("FileSystemWatcher: File %s is already being watched", qPrintable(file));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning("File %s is not watched: Too many file handles are already open (max is %u).",
                     qPrintable(file), unsigned(d->m_staticData->maxFileOpen));
            break;
        }

        d->m_files.insert(file, WatchEntry(file, wm));

        const int count = ++d->m_staticData->m_fileCount[file];
        QTC_CHECK(count > 0);

        if (count == 1) {
            toAdd << file;

            QFileInfo fi(file);
            if (!fi.exists()) {
                const QString directory = fi.path();
                const int dirCount = ++d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount > 0);

                if (dirCount == 1)
                    toAdd << directory;
            }
        }
    }

    if (!toAdd.isEmpty())
        d->m_staticData->m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeFile(const QString &file)
{
    removeFiles(QStringList(file));
}

void FileSystemWatcher::removeFiles(const QStringList &files)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "removeFiles" << files;
    QStringList toRemove;
    for (const QString &file : files) {
        const auto it = d->m_files.constFind(file);
        if (it == d->m_files.constEnd()) {
            qWarning("FileSystemWatcher: File %s is not watched.", qPrintable(file));
            continue;
        }
        d->m_files.erase(it);

        const int count = --(d->m_staticData->m_fileCount[file]);
        QTC_CHECK(count >= 0);

        if (!count) {
            toRemove << file;

            QFileInfo fi(file);
            if (!fi.exists()) {
                const QString directory = fi.path();
                const int dirCount = --d->m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount >= 0);

                if (!dirCount)
                    toRemove << directory;
            }
        }
    }

    if (!toRemove.isEmpty())
        d->m_staticData->m_watcher->removePaths(toRemove);
}

void FileSystemWatcher::clear()
{
    if (!d->m_files.isEmpty())
        removeFiles(filePaths());
    if (!d->m_directories.isEmpty())
        removeDirectories(directoryPaths());
}

QStringList FileSystemWatcher::files() const
{
    return d->m_files.keys();
}

bool FileSystemWatcher::watchesDirectory(const QString &directory) const
{
    return d->m_directories.contains(directory);
}

void FileSystemWatcher::addDirectory(const QString &directory, WatchMode wm)
{
    addDirectories(QStringList(directory), wm);
}

void FileSystemWatcher::addDirectories(const QStringList &directories, WatchMode wm)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "addDirectories mode" << wm << directories
        << "limit currently:" << (d->m_files.size() + d->m_directories.size())
        << "of" << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    for (const QString &directory : directories) {
        if (watchesDirectory(directory)) {
            qWarning("FileSystemWatcher: Directory %s is already being watched.", qPrintable(directory));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning("Directory %s is not watched: Too many file handles are already open (max is %u).",
                     qPrintable(directory), unsigned(d->m_staticData->maxFileOpen));
            break;
        }

        d->m_directories.insert(directory, WatchEntry(directory, wm));

        const int count = ++d->m_staticData->m_directoryCount[directory];
        QTC_CHECK(count > 0);

        if (count == 1)
            toAdd << directory;
    }

    if (!toAdd.isEmpty())
        d->m_staticData->m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeDirectory(const FilePath &file)
{
    removeDirectories({file.toFSPathString()});
}

void FileSystemWatcher::removeDirectories(const QStringList &directories)
{
    qCDebug(fileSystemWatcherLog)
        << this << d->m_id << "removeDirectories" << directories;

    QStringList toRemove;
    for (const QString &directory : directories) {
        const auto it = d->m_directories.constFind(directory);
        if (it == d->m_directories.constEnd()) {
            qWarning("FileSystemWatcher: Directory %s is not watched.", qPrintable(directory));
            continue;
        }
        d->m_directories.erase(it);

        const int count = --d->m_staticData->m_directoryCount[directory];
        QTC_CHECK(count >= 0);

        if (!count)
            toRemove << directory;
    }
    if (!toRemove.isEmpty())
        d->m_staticData->m_watcher->removePaths(toRemove);
}

QStringList FileSystemWatcher::directories() const
{
    return d->m_directories.keys();
}

void FileSystemWatcher::slotFileChanged(const QString &path)
{
    const auto it = d->m_files.find(path);
    if (it != d->m_files.end() && it.value().trigger(path)) {
        qCDebug(fileSystemWatcherLog)
            << this << "triggers on file" << it.key()
            << it.value().watchMode
            << it.value().modifiedTime.toString(Qt::ISODate);

        QFileInfo fi(path);
        if (!fi.exists()) {
            const QString directory = fi.path();
            const int dirCount = ++d->m_staticData->m_directoryCount[directory];
            QTC_CHECK(dirCount > 0);

            if (dirCount == 1)
                d->m_staticData->m_watcher->addPath(directory);
        }
        d->fileChanged(path);
    }
}

void FileSystemWatcher::slotDirectoryChanged(const QString &path)
{
    const auto it = d->m_directories.find(path);
    if (it != d->m_directories.end() && it.value().trigger(path)) {
        qCDebug(fileSystemWatcherLog)
            << this << "triggers on dir" << it.key()
            << it.value().watchMode
            << it.value().modifiedTime.toString(Qt::ISODate);
        d->directoryChanged(path);
    }

    QStringList toReadd;
    const auto dir = FilePath::fromString(path);
    for (const FilePath &entry : dir.dirEntries(QDir::Files)) {
        const QString file = entry.toString();
        if (d->m_files.contains(file))
            toReadd.append(file);
    }

    if (!toReadd.isEmpty()) {
        for (const QString &rejected : d->m_staticData->m_watcher->addPaths(toReadd))
            toReadd.removeOne(rejected);

        // If we've successfully added the file, that means it was deleted and replaced.
        for (const QString &reAdded : std::as_const(toReadd)) {
            const QString directory = QFileInfo(reAdded).path();
            const int dirCount = --d->m_staticData->m_directoryCount[directory];
            QTC_CHECK(dirCount >= 0);

            if (!dirCount)
                d->m_staticData->m_watcher->removePath(directory);

            d->fileChanged(reAdded);
        }
    }
}

void FileSystemWatcher::addFile(const FilePath &file, WatchMode wm)
{
    addFile(file.toFSPathString(), wm);
}

void FileSystemWatcher::addFiles(const FilePaths &files, WatchMode wm)
{
    addFiles(transform(files, &FilePath::toFSPathString), wm);
}

void FileSystemWatcher::removeFile(const FilePath &file)
{
    removeFile(file.toFSPathString());
}

void FileSystemWatcher::removeFiles(const FilePaths &files)
{
    removeFiles(transform(files, &FilePath::toFSPathString));
}

bool FileSystemWatcher::watchesFile(const FilePath &file) const
{
    return watchesFile(file.toFSPathString());
}

FilePaths FileSystemWatcher::filePaths() const
{
    return transform(files(), &FilePath::fromString);
}

void FileSystemWatcher::addDirectory(const FilePath &file, WatchMode wm)
{
    addDirectory(file.toFSPathString(), wm);
}

void FileSystemWatcher::addDirectories(const FilePaths &files, WatchMode wm)
{
    addDirectories(transform(files, &FilePath::toFSPathString), wm);
}

void FileSystemWatcher::removeDirectories(const FilePaths &files)
{
    removeDirectories(transform(files, &FilePath::toFSPathString));
}

bool FileSystemWatcher::watchesDirectory(const FilePath &file) const
{
    return watchesDirectory(file.toFSPathString());
}

FilePaths FileSystemWatcher::directoryPaths() const
{
    return transform(directories(), &FilePath::fromString);
}

} //Utils
