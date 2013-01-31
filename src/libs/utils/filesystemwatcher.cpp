/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "filesystemwatcher.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDateTime>

enum { debug = 0 };

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
    \brief File watcher that internally uses a centralized QFileSystemWatcher
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

// Centralized file watcher static data per integer id.
class FileSystemWatcherStaticData
{
public:
    FileSystemWatcherStaticData() :
        maxFileOpen(getFileLimit()) , m_objectCount(0), m_watcher(0) {}

    quint64 maxFileOpen;
    int m_objectCount;
    QHash<QString, int> m_fileCount;
    QHash<QString, int> m_directoryCount;
    QFileSystemWatcher *m_watcher;
};

typedef QMap<int, FileSystemWatcherStaticData> FileSystemWatcherStaticDataMap;

Q_GLOBAL_STATIC(FileSystemWatcherStaticDataMap, fileSystemWatcherStaticDataMap)

class WatchEntry
{
public:
    typedef FileSystemWatcher::WatchMode WatchMode;

    explicit WatchEntry(const QString &file, WatchMode wm) :
        watchMode(wm), modifiedTime(QFileInfo(file).lastModified()) {}

    WatchEntry() : watchMode(FileSystemWatcher::WatchAllChanges) {}

    bool trigger(const QString &fileName);

    WatchMode watchMode;
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

typedef QHash<QString, WatchEntry> WatchEntryMap;
typedef WatchEntryMap::iterator WatchEntryMapIterator;

class FileSystemWatcherPrivate
{
public:
    explicit FileSystemWatcherPrivate(int id) : m_id(id), m_staticData(0) {}

    WatchEntryMap m_files;
    WatchEntryMap m_directories;

    bool checkLimit() const;

    const int m_id;
    FileSystemWatcherStaticData *m_staticData;
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

/*!
    \brief Add to watcher 0.
*/

FileSystemWatcher::FileSystemWatcher(QObject *parent) :
    QObject(parent), d(new FileSystemWatcherPrivate(0))
{
    init();
}

/*!
    \brief Add to a watcher with specified id.
*/

FileSystemWatcher::FileSystemWatcher(int id, QObject *parent) :
    QObject(parent), d(new FileSystemWatcherPrivate(id))
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
        if (debug)
            qDebug() << this << "Created watcher for id " << d->m_id;
    }
    ++(d->m_staticData->m_objectCount);
    connect(d->m_staticData->m_watcher, SIGNAL(fileChanged(QString)),
            this, SLOT(slotFileChanged(QString)));
    connect(d->m_staticData->m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(slotDirectoryChanged(QString)));
}

FileSystemWatcher::~FileSystemWatcher()
{
    if (!d->m_files.isEmpty())
        removeFiles(files());
    if (!d->m_directories.isEmpty())
        removeDirectories(directories());

    if (--(d->m_staticData->m_objectCount) == 0) {
        delete d->m_staticData->m_watcher;
        d->m_staticData->m_watcher = 0;
        d->m_staticData->m_fileCount.clear();
        d->m_staticData->m_directoryCount.clear();
        if (debug)
              qDebug() << this << "Deleted watcher" << d->m_id;
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
    if (debug)
        qDebug() << this << d->m_id << "addFiles mode=" << wm << files
                 << " limit currently: " << (d->m_files.size() + d->m_directories.size())
                 << " of " << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    foreach (const QString &file, files) {
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
        Q_ASSERT(count > 0);

        if (count == 1)
            toAdd << file;
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
    if (debug)
        qDebug() << this << d->m_id << "removeFiles " << files;
    QStringList toRemove;
    foreach (const QString &file, files) {
        WatchEntryMapIterator it = d->m_files.find(file);
        if (it == d->m_files.end()) {
            qWarning("FileSystemWatcher: File %s is not watched.", qPrintable(file));
            continue;
        }
        d->m_files.erase(it);

        const int count = --(d->m_staticData->m_fileCount[file]);
        Q_ASSERT(count >= 0);

        if (!count)
            toRemove << file;
    }

    if (!toRemove.isEmpty())
        d->m_staticData->m_watcher->removePaths(toRemove);
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
    if (debug)
        qDebug() << this << d->m_id << "addDirectories mode " << wm << directories
                 << " limit currently: " << (d->m_files.size() + d->m_directories.size())
                 << " of " << d->m_staticData->maxFileOpen;
    QStringList toAdd;
    foreach (const QString &directory, directories) {
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
        Q_ASSERT(count > 0);

        if (count == 1)
            toAdd << directory;
    }

    if (!toAdd.isEmpty())
        d->m_staticData->m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeDirectory(const QString &directory)
{
    removeDirectories(QStringList(directory));
}

void FileSystemWatcher::removeDirectories(const QStringList &directories)
{
    if (debug)
        qDebug() << this << d->m_id << "removeDirectories" << directories;

    QStringList toRemove;
    foreach (const QString &directory, directories) {
        WatchEntryMapIterator it = d->m_directories.find(directory);
        if (it == d->m_directories.end()) {
            qWarning("FileSystemWatcher: Directory %s is not watched.", qPrintable(directory));
            continue;
        }
        d->m_directories.erase(it);

        const int count = --d->m_staticData->m_directoryCount[directory];
        Q_ASSERT(count >= 0);

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
    const WatchEntryMapIterator it = d->m_files.find(path);
    if (it != d->m_files.end() && it.value().trigger(path)) {
        if (debug)
            qDebug() << this << "triggers on file " << path
                     << it.value().watchMode
                     << it.value().modifiedTime.toString(Qt::ISODate);
        emit fileChanged(path);
    }
}

void FileSystemWatcher::slotDirectoryChanged(const QString &path)
{
    const WatchEntryMapIterator it = d->m_directories.find(path);
    if (it != d->m_directories.end() && it.value().trigger(path)) {
        if (debug)
            qDebug() << this << "triggers on dir " << path
                     << it.value().watchMode
                     << it.value().modifiedTime.toString(Qt::ISODate);
        emit directoryChanged(path);
    }
}

} // namespace Utils
