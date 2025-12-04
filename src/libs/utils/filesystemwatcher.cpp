// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemwatcher.h"

#include "globalfilechangeblocker.h"
#include "filepath.h"

#include <chrono>

#include <QDateTime>
#include <QDir>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QTimer>

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

    explicit WatchEntry(const FilePath &file, WatchMode wm)
        : watchMode(wm)
    {
        if (watchMode == FileSystemWatcher::WatchModifiedDate) {
            modifiedTime = file.lastModified();
            QTC_CHECK(modifiedTime.isValid());
        }
    }

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
    QTC_ASSERT(modifiedTime.isValid(), return false);
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
    explicit FileSystemWatcherPrivate(FileSystemWatcher *q, int id)
        : m_id(id)
        , q(q)
    {
        QObject::connect(
            Utils::GlobalFileChangeBlocker::instance(),
            &Utils::GlobalFileChangeBlocker::stateChanged,
            q,
            [this](bool blocked) { autoReloadPostponed(blocked); });
    }

    bool checkLimit() const
    {
        // Do not use more than half of the available file handles.
        return quint64(m_directories.size() + m_files.size()) < (m_staticData->maxFileOpen / 2);
    }

    void fileChanged(const FilePath &path)
    {
        if (m_postponed)
            m_postponedFiles.insert(path);
        else
            emit q->fileChanged(path);
    }

    void directoryChanged(const FilePath &path)
    {
        if (m_postponed)
            m_postponedDirectories.insert(path);
        else
            emit q->directoryChanged(path);
    }

    void autoReloadPostponed(bool postponed)
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

    // Handle debounced directory change.
    void handleDirChanged(const FilePath &dirPath)
    {
        // Emit only if trigger says it's a "real" change for this watch mode.
        if (auto it = m_directories.find(dirPath);
            it != m_directories.end() && it.value().trigger(dirPath)) {
            qCDebug(fileSystemWatcherLog)
                << q << "triggers on dir" << it.key() << it.value().watchMode
                << it.value().modifiedTime.toString(Qt::ISODate);
            directoryChanged(dirPath);
        }

        // Re-add files that were replaced (deleted + recreated).
        QStringList toReadd;
        for (const FilePath &entry : dirPath.dirEntries(QDir::Files)) {
            if (m_files.contains(entry))
                toReadd.append(entry.toFSPathString());
        }

        if (!toReadd.isEmpty()) {
            // addPaths returns rejected; keep only successfully re-added files.
            const QStringList rejected = m_staticData->m_watcher->addPaths(toReadd);
            for (const QString &r : rejected)
                toReadd.removeOne(r);

            for (const QString &reAddedStr : std::as_const(toReadd)) {
                const FilePath reAdded = FilePath::fromString(reAddedStr);
                const FilePath directory = reAdded.parentDir();

                // We watched the directory temporarily while file was gone; drop it again.
                const int dirCount = --m_staticData->m_directoryCount[directory];
                QTC_CHECK(dirCount >= 0);
                if (!dirCount)
                    m_staticData->m_watcher->removePath(directory.toFSPathString());

                fileChanged(reAdded); // notify: file effectively changed (replacement)
            }
        }
    }

    WatchEntryMap m_files;
    WatchEntryMap m_directories;

    QSet<FilePath> m_postponedFiles;
    QSet<FilePath> m_postponedDirectories;

    const int m_id;
    FileSystemWatcherStaticData *m_staticData = nullptr;

    // Debounce map for directoryChanged
    QHash<FilePath, QTimer *> m_timerMap;

private:
    bool m_postponed = false;
    FileSystemWatcher *q;
};

/*! Creates a file system watcher with the ID 0 and the owner \a parent. */
FileSystemWatcher::FileSystemWatcher(QObject *parent)
    : FileSystemWatcher(0, parent)
{}

/*! Creates a file system watcher with the ID \a id and the owner \a parent. */
FileSystemWatcher::FileSystemWatcher(int id, QObject *parent)
    : QObject(parent)
    , d(new FileSystemWatcherPrivate(this, id))
{
    // Acquire / create shared watcher for this id
    FileSystemWatcherStaticDataMap &map = *fileSystemWatcherStaticDataMap();
    auto it = map.find(d->m_id);
    if (it == map.end())
        it = map.insert(d->m_id, FileSystemWatcherStaticData());
    d->m_staticData = &it.value();

    if (!d->m_staticData->m_watcher) {
        d->m_staticData->m_watcher = new QFileSystemWatcher();
        qCDebug(fileSystemWatcherLog) << this << "Created watcher for id" << d->m_id;
    }
    ++(d->m_staticData->m_objectCount);

    // QFileSystemWatcher emits QString; convert to FilePath and dispatch.

    connect(
        d->m_staticData->m_watcher,
        &QFileSystemWatcher::fileChanged,
        this,
        [this](const QString &fileStr) {
            const FilePath filePath = FilePath::fromString(fileStr);
            auto it = d->m_files.find(filePath);
            if (it != d->m_files.end() && it.value().trigger(filePath)) {
                qCDebug(fileSystemWatcherLog)
                    << this << "triggers on file" << it.key() << it.value().watchMode
                    << it.value().modifiedTime.toString(Qt::ISODate);

                if (!filePath.exists()) {
                    // File disappeared: watch its directory so we notice recreation.
                    const FilePath directory = filePath.parentDir();
                    const int dirCount = ++d->m_staticData->m_directoryCount[directory];
                    QTC_CHECK(dirCount > 0);
                    if (dirCount == 1)
                        d->m_staticData->m_watcher->addPath(directory.toFSPathString());
                }
                d->fileChanged(filePath);
            }
        });

    connect(
        d->m_staticData->m_watcher,
        &QFileSystemWatcher::directoryChanged,
        this,
        [this](const QString &dirStr) {
            const FilePath dir = FilePath::fromString(dirStr);

            // Debounce per directory (100 ms)
            QTimer *t = d->m_timerMap.value(dir, nullptr);
            if (t) {
                t->start(100);
                return;
            }
            t = new QTimer(this);
            t->setSingleShot(true);
            t->setInterval(100);
            connect(t, &QTimer::timeout, this, [this, dir, t]() {
                d->m_timerMap.remove(dir);
                t->deleteLater();
                d->handleDirChanged(dir);
            });
            d->m_timerMap.insert(dir, t);
            t->start();
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
        qCDebug(fileSystemWatcherLog) << this << "Deleted watcher" << d->m_id;
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
    qCDebug(fileSystemWatcherLog) << this << d->m_id << "addFiles mode" << wm << filePaths
                                  << "limit currently:"
                                  << (d->m_files.size() + d->m_directories.size()) << "of"
                                  << d->m_staticData->maxFileOpen;

    QStringList toAdd;
    for (const FilePath &fp : filePaths) {
        if (watchesFile(fp)) {
            qWarning(
                "FileSystemWatcher: File %s is already being watched",
                qPrintable(fp.toUserOutput()));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning(
                "File %s is not watched: Too many file handles are already open (max is %u).",
                qPrintable(fp.toUserOutput()),
                unsigned(d->m_staticData->maxFileOpen));
            break;
        }

        d->m_files.insert(fp, WatchEntry(fp, wm));

        const int count = ++d->m_staticData->m_fileCount[fp];
        QTC_CHECK(count > 0);

        if (count == 1) {
            toAdd << fp.toFSPathString();

            if (!fp.exists()) {
                const FilePath dir = fp.parentDir();
                const int dirCount = ++d->m_staticData->m_directoryCount[dir];
                QTC_CHECK(dirCount > 0);
                if (dirCount == 1)
                    toAdd << dir.toFSPathString();
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
    qCDebug(fileSystemWatcherLog) << this << d->m_id << "removeFiles" << filePaths;

    QStringList toRemove;
    for (const FilePath &fp : filePaths) {
        const auto it = d->m_files.constFind(fp);
        if (it == d->m_files.constEnd()) {
            qWarning("FileSystemWatcher: File %s is not watched.", qPrintable(fp.toUserOutput()));
            continue;
        }
        d->m_files.erase(it);

        const int count = --(d->m_staticData->m_fileCount[fp]);
        QTC_CHECK(count >= 0);

        if (!count) {
            toRemove << fp.toFSPathString();

            if (!fp.exists()) {
                const FilePath dir = fp.parentDir();
                const int dirCount = --d->m_staticData->m_directoryCount[dir];
                QTC_CHECK(dirCount >= 0);
                if (!dirCount)
                    toRemove << dir.toFSPathString();
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
    qCDebug(fileSystemWatcherLog) << this << d->m_id << "addDirectories mode" << wm << directories
                                  << "limit currently:"
                                  << (d->m_files.size() + d->m_directories.size()) << "of"
                                  << d->m_staticData->maxFileOpen;

    QStringList toAdd;
    for (const FilePath &dir : directories) {
        if (watchesDirectory(dir)) {
            qWarning(
                "FileSystemWatcher: Directory %s is already being watched.",
                qPrintable(dir.toUserOutput()));
            continue;
        }

        if (!d->checkLimit()) {
            qWarning(
                "Directory %s is not watched: Too many file handles are already open (max is %u).",
                qPrintable(dir.toUserOutput()),
                unsigned(d->m_staticData->maxFileOpen));
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

void FileSystemWatcher::removeDirectory(const FilePath &directory)
{
    removeDirectories({directory});
}

void FileSystemWatcher::removeDirectories(const FilePaths &directories)
{
    qCDebug(fileSystemWatcherLog) << this << d->m_id << "removeDirectories" << directories;

    QStringList toRemove;
    for (const FilePath &dir : directories) {
        const auto it = d->m_directories.constFind(dir);
        if (it == d->m_directories.constEnd()) {
            qWarning(
                "FileSystemWatcher: Directory %s is not watched.", qPrintable(dir.toUserOutput()));
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

} // namespace Utils
