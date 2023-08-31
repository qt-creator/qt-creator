// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "filesystemmodel.h"

#include "environment.h"
#include "hostosinfo.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QDateTime>
#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QIcon>
#include <QLocale>
#include <QMimeData>
#include <QMutex>
#include <QPair>
#include <QStack>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>
#include <QUrl>
#include <QVarLengthArray>
#include <QWaitCondition>

#include <vector>
#include <algorithm>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
 #define CALLBACK WINAPI
#endif
#  include <qt_windows.h>
#  include <shlobj.h>
#else
#  include <unistd.h>
#  include <sys/types.h>
#endif

namespace Utils {

static bool useFileSystemWatcher()
{
    return true;
}

class ExtendedInformation {
public:
    enum Type { Dir, File, System };

    ExtendedInformation() {}
    ExtendedInformation(const QFileInfo &info) : mFileInfo(info) {}

    inline bool isDir() { return type() == Dir; }
    inline bool isFile() { return type() == File; }
    inline bool isSystem() { return type() == System; }

    bool operator ==(const ExtendedInformation &fileInfo) const {
       return mFileInfo == fileInfo.mFileInfo
       && displayType == fileInfo.displayType
       && permissions() == fileInfo.permissions()
       && lastModified() == fileInfo.lastModified();
    }

    QFile::Permissions permissions() const {
        return mFileInfo.permissions();
    }

    Type type() const {
        if (mFileInfo.isDir()) {
            return ExtendedInformation::Dir;
        }
        if (mFileInfo.isFile()) {
            return ExtendedInformation::File;
        }
        if (!mFileInfo.exists() && mFileInfo.isSymLink()) {
            return ExtendedInformation::System;
        }
        return ExtendedInformation::System;
    }

    bool isSymLink(bool ignoreNtfsSymLinks = false) const
    {
        if (ignoreNtfsSymLinks && HostOsInfo::isWindowsHost())
            return !mFileInfo.suffix().compare(QLatin1String("lnk"), Qt::CaseInsensitive);
        return mFileInfo.isSymLink();
    }

    bool isHidden() const {
        return mFileInfo.isHidden();
    }

    QFileInfo fileInfo() const {
        return mFileInfo;
    }

    QDateTime lastModified() const {
        return mFileInfo.lastModified();
    }

    qint64 size() const {
        qint64 size = -1;
        if (type() == ExtendedInformation::Dir)
            size = 0;
        if (type() == ExtendedInformation::File)
            size = mFileInfo.size();
        if (!mFileInfo.exists() && !mFileInfo.isSymLink())
            size = -1;
        return size;
    }

    QString displayType;
    QIcon icon;

private :
    QFileInfo mFileInfo;
};

void static doStat(QFileInfo &fi)
{
    Q_UNUSED(fi)
//            driveInfo.stat();
}

static QString translateDriveName(const QFileInfo &drive)
{
    QString driveName = drive.absoluteFilePath();
    if (HostOsInfo::isWindowsHost()) {
        if (driveName.startsWith(QLatin1Char('/'))) // UNC host
            return drive.fileName();
        if (driveName.endsWith(QLatin1Char('/')))
            driveName.chop(1);
    }
    return driveName;
}

class FileInfoGatherer : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void updates(const QString &directory, const QList<QPair<QString, QFileInfo>> &updates);
    void newListOfFiles(const QString &directory, const QStringList &listOfFiles) const;
    void nameResolved(const QString &fileName, const QString &resolvedName) const;
    void directoryLoaded(const QString &path);

public:
    explicit FileInfoGatherer(QObject *parent = nullptr);
    ~FileInfoGatherer();

    QStringList watchedFiles() const;
    QStringList watchedDirectories() const;
    void watchPaths(const QStringList &paths);
    void unwatchPaths(const QStringList &paths);

    bool isWatching() const;
    void setWatching(bool v);

    // only callable from this->thread():
    void clear();
    void removePath(const QString &path);
    ExtendedInformation getInfo(const QFileInfo &info) const;
    QFileIconProvider *iconProvider() const;
    bool resolveSymlinks() const;

public Q_SLOTS:
    void list(const QString &directoryPath);
    void fetchExtendedInformation(const QString &path, const QStringList &files);
    void updateFile(const QString &path);
    void setResolveSymlinks(bool enable);
    void setIconProvider(QFileIconProvider *provider);

private Q_SLOTS:
    void driveAdded();
    void driveRemoved();

private:
    void run() override;
    // called by run():
    void getFileInfos(const QString &path, const QStringList &files);
    void fetch(const QFileInfo &info, QElapsedTimer &base, bool &firstTime,
               QList<QPair<QString, QFileInfo>> &updatedFiles, const QString &path);

private:
    void createWatcher();

    mutable QMutex mutex;
    // begin protected by mutex
    QWaitCondition condition;
    QStack<QString> path;
    QStack<QStringList> files;
    // end protected by mutex
    QAtomicInt abort;

    QFileSystemWatcher *m_watcher = nullptr;
    bool m_watching = true;

    QFileIconProvider *m_iconProvider; // not accessed by run()
    QFileIconProvider defaultProvider;
    bool m_resolveSymlinks = true; // not accessed by run() // Windows only
};

/*!
    \internal
    Creates a thread.
*/
FileInfoGatherer::FileInfoGatherer(QObject *parent)
    : QThread(parent)
    , m_iconProvider(&defaultProvider)
{
    start(LowPriority);
}

/*!
    \internal
    Destroys a thread.
*/
FileInfoGatherer::~FileInfoGatherer()
{
    abort.storeRelaxed(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
    wait();
}

void FileInfoGatherer::setResolveSymlinks(bool enable)
{
    m_resolveSymlinks = enable;
}

void FileInfoGatherer::driveAdded()
{
    fetchExtendedInformation(QString(), QStringList());
}

void FileInfoGatherer::driveRemoved()
{
    QStringList drives;
    const QFileInfoList driveInfoList = QDir::drives();
    for (const QFileInfo &fi : driveInfoList)
        drives.append(translateDriveName(fi));
    newListOfFiles(QString(), drives);
}

bool FileInfoGatherer::resolveSymlinks() const
{
    return HostOsInfo::isWindowsHost() && m_resolveSymlinks;
}

void FileInfoGatherer::setIconProvider(QFileIconProvider *provider)
{
    m_iconProvider = provider;
}

QFileIconProvider *FileInfoGatherer::iconProvider() const
{
    return m_iconProvider;
}

/*!
    \internal
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void FileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    QMutexLocker locker(&mutex);
    // See if we already have this dir/file in our queue
    int loc = this->path.lastIndexOf(path);
    while (loc > 0)  {
        if (this->files.at(loc) == files) {
            return;
        }
        loc = this->path.lastIndexOf(path, loc - 1);
    }
    this->path.push(path);
    this->files.push(files);
    condition.wakeAll();

    if (useFileSystemWatcher()) {
        if (files.isEmpty()
                && !path.isEmpty()
                && !path.startsWith(QLatin1String("//")) /*don't watch UNC path*/) {
            if (!watchedDirectories().contains(path))
                watchPaths(QStringList(path));
        }
    }
}

/*!
    \internal
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void FileInfoGatherer::updateFile(const QString &filePath)
{
    QString dir = filePath.mid(0, filePath.lastIndexOf(QLatin1Char('/')));
    QString fileName = filePath.mid(dir.length() + 1);
    fetchExtendedInformation(dir, QStringList(fileName));
}

QStringList FileInfoGatherer::watchedFiles() const
{
    if (useFileSystemWatcher() && m_watcher)
        return m_watcher->files();
    return {};
}

QStringList FileInfoGatherer::watchedDirectories() const
{
    if (useFileSystemWatcher() && m_watcher)
        return m_watcher->directories();
    return {};
}

void FileInfoGatherer::createWatcher()
{
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileInfoGatherer::list);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &FileInfoGatherer::updateFile);
    if (HostOsInfo::isWindowsHost()) {
        const QVariant listener = m_watcher->property("_q_driveListener");
        if (listener.canConvert<QObject *>()) {
            if (QObject *driveListener = listener.value<QObject *>()) {
                connect(driveListener, SIGNAL(driveAdded()), this, SLOT(driveAdded()));
                connect(driveListener, SIGNAL(driveRemoved()), this, SLOT(driveRemoved()));
            }
        }
    }
}

void FileInfoGatherer::watchPaths(const QStringList &paths)
{
    if (useFileSystemWatcher() && m_watching) {
        if (m_watcher == nullptr)
            createWatcher();
        m_watcher->addPaths(paths);
    }
}

void FileInfoGatherer::unwatchPaths(const QStringList &paths)
{
    if (useFileSystemWatcher() && m_watcher && !paths.isEmpty())
        m_watcher->removePaths(paths);
}

bool FileInfoGatherer::isWatching() const
{
    bool result = false;
    QMutexLocker locker(&mutex);
    result = m_watching;
    return result;
}

void FileInfoGatherer::setWatching(bool v)
{
    QMutexLocker locker(&mutex);
    if (v != m_watching) {
        if (!v) {
            delete m_watcher;
            m_watcher = nullptr;
        }
        m_watching = v;
    }
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void FileInfoGatherer::clear()
{
    QTC_CHECK(useFileSystemWatcher());
    QMutexLocker locker(&mutex);
    unwatchPaths(watchedFiles());
    unwatchPaths(watchedDirectories());
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void FileInfoGatherer::removePath(const QString &path)
{
    QTC_CHECK(useFileSystemWatcher());
    QMutexLocker locker(&mutex);
    unwatchPaths(QStringList(path));
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void FileInfoGatherer::list(const QString &directoryPath)
{
    fetchExtendedInformation(directoryPath, QStringList());
}

/*
    Until aborted wait to fetch a directory or files
*/
void FileInfoGatherer::run()
{
    forever {
        QMutexLocker locker(&mutex);
        while (!abort.loadRelaxed() && path.isEmpty())
            condition.wait(&mutex);
        if (abort.loadRelaxed())
            return;
        const QString thisPath = std::as_const(path).front();
        path.pop_front();
        const QStringList thisList = std::as_const(files).front();
        files.pop_front();
        locker.unlock();

        getFileInfos(thisPath, thisList);
    }
}

ExtendedInformation FileInfoGatherer::getInfo(const QFileInfo &fileInfo) const
{
    ExtendedInformation info(fileInfo);
    info.icon = m_iconProvider->icon(fileInfo);
    info.displayType = m_iconProvider->type(fileInfo);
    if (useFileSystemWatcher()) {
        // ### Not ready to listen all modifications by default
        static const bool watchFiles = qtcEnvironmentVariableIsSet(
            "QT_FILESYSTEMMODEL_WATCH_FILES");
        if (watchFiles) {
            if (!fileInfo.exists() && !fileInfo.isSymLink()) {
                const_cast<FileInfoGatherer *>(this)->
                    unwatchPaths(QStringList(fileInfo.absoluteFilePath()));
            } else {
                const QString path = fileInfo.absoluteFilePath();
                if (!path.isEmpty() && fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()
                    && !watchedFiles().contains(path)) {
                    const_cast<FileInfoGatherer *>(this)->watchPaths(QStringList(path));
                }
            }
        }
    }

    if (HostOsInfo::isWindowsHost() && m_resolveSymlinks && info.isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QFileInfo resolvedInfo(QFileInfo(fileInfo.symLinkTarget()).canonicalFilePath());
        if (resolvedInfo.exists()) {
            emit nameResolved(fileInfo.filePath(), resolvedInfo.fileName());
        }
    }
    return info;
}

/*
    Get specific file info's, batch the files so update when we have 100
    items and every 200ms after that
 */
void FileInfoGatherer::getFileInfos(const QString &path, const QStringList &files)
{
    // List drives
    if (path.isEmpty()) {
        QFileInfoList infoList;
        if (files.isEmpty()) {
            infoList = QDir::drives();
        } else {
            infoList.reserve(files.count());
            for (const auto &file : files)
                infoList << QFileInfo(file);
        }
        QList<QPair<QString, QFileInfo>> updatedFiles;
        updatedFiles.reserve(infoList.count());
        for (int i = infoList.count() - 1; i >= 0; --i) {
            QFileInfo driveInfo = infoList.at(i);
            doStat(driveInfo);
            QString driveName = translateDriveName(driveInfo);
            updatedFiles.append(QPair<QString,QFileInfo>(driveName, driveInfo));
        }
        emit updates(path, updatedFiles);
        return;
    }

    QElapsedTimer base;
    base.start();
    QFileInfo fileInfo;
    bool firstTime = true;
    QList<QPair<QString, QFileInfo>> updatedFiles;
    QStringList filesToCheck = files;

    QStringList allFiles;
    if (files.isEmpty()) {
        QDirIterator dirIt(path, QDir::AllEntries | QDir::System | QDir::Hidden);
        while (!abort.loadRelaxed() && dirIt.hasNext()) {
            dirIt.next();
            fileInfo = dirIt.fileInfo();
            doStat(fileInfo);
            allFiles.append(fileInfo.fileName());
            fetch(fileInfo, base, firstTime, updatedFiles, path);
        }
    }
    if (!allFiles.isEmpty())
        emit newListOfFiles(path, allFiles);

    QStringList::const_iterator filesIt = filesToCheck.constBegin();
    while (!abort.loadRelaxed() && filesIt != filesToCheck.constEnd()) {
        fileInfo.setFile(path + QDir::separator() + *filesIt);
        ++filesIt;
        doStat(fileInfo);
        fetch(fileInfo, base, firstTime, updatedFiles, path);
    }
    if (!updatedFiles.isEmpty())
        emit updates(path, updatedFiles);
    emit directoryLoaded(path);
}

void FileInfoGatherer::fetch(const QFileInfo &fileInfo, QElapsedTimer &base, bool &firstTime,
                              QList<QPair<QString, QFileInfo>> &updatedFiles, const QString &path)
{
    updatedFiles.append(QPair<QString, QFileInfo>(fileInfo.fileName(), fileInfo));
    QElapsedTimer current;
    current.start();
    if ((firstTime && updatedFiles.count() > 100) || base.msecsTo(current) > 1000) {
        emit updates(path, updatedFiles);
        updatedFiles.clear();
        base = current;
        firstTime = false;
    }
}

class PathKey
{
public:
    PathKey() : caseSensitivity(Qt::CaseInsensitive) {}
    explicit PathKey(Qt::CaseSensitivity cs) : caseSensitivity(cs) {}
    explicit PathKey(const QString &s, Qt::CaseSensitivity cs) : data(s), caseSensitivity(cs) {}

    friend bool operator==(const PathKey &a, const PathKey &b) {
        return a.data.compare(b.data, a.caseSensitivity) == 0;
    }
    friend bool operator!=(const PathKey &a, const PathKey &b) {
        return a.data.compare(b.data, a.caseSensitivity) != 0;
    }
    friend bool operator<(const PathKey &a, const PathKey &b) {
        return a.data.compare(b.data, a.caseSensitivity) == -1;
    }
    friend bool operator>(const PathKey &a, const PathKey &b) {
        return a.data.compare(b.data, a.caseSensitivity) == 1;
    }

    QString data;
    Qt::CaseSensitivity caseSensitivity;
};

using PathKeys = QList<PathKey>;

size_t qHash(const PathKey &key)
{
    if (key.caseSensitivity == Qt::CaseInsensitive)
        return qHash(key.data.toCaseFolded());
    return qHash(key.data);
}

class FileSystemModelSlots : public QObject
{
public:
    explicit FileSystemModelSlots(FileSystemModelPrivate *owner,
                                  FileSystemModel *q_owner)
        : owner(owner), q_owner(q_owner)
    {}

    void _q_directoryChanged(const QString &directory, const QStringList &list);
    void _q_performDelayedSort();
    void _q_fileSystemChanged(const QString &path,
                              const QList<QPair<QString, QFileInfo>> &);
    void _q_resolvedName(const QString &fileName, const QString &resolvedName);


    void directoryLoaded(const QString &path);

    FileSystemModelPrivate *owner;
    FileSystemModel *q_owner;
};

class FileSystemNode
{
public:
    Q_DISABLE_COPY_MOVE(FileSystemNode)

    explicit FileSystemNode(const PathKey &filename = {}, FileSystemNode *p = nullptr)
        : fileName(filename), parent(p)
    {}

    ~FileSystemNode() {
        qDeleteAll(children);
        delete info;
    }

    PathKey fileName;
    QString volumeName; // Windows only

    inline qint64 size() const { if (info && !info->isDir()) return info->size(); return 0; }
    inline QString type() const { if (info) return info->displayType; return QLatin1String(""); }
    inline QDateTime lastModified() const { if (info) return info->lastModified(); return QDateTime(); }
    inline QFile::Permissions permissions() const { if (info) return info->permissions(); return { }; }
    inline bool isReadable() const { return ((permissions() & QFile::ReadUser) != 0); }
    inline bool isWritable() const { return ((permissions() & QFile::WriteUser) != 0); }
    inline bool isExecutable() const { return ((permissions() & QFile::ExeUser) != 0); }
    inline bool isDir() const {
        if (info)
            return info->isDir();
        if (children.count() > 0)
            return true;
        return false;
    }
    inline QFileInfo fileInfo() const { if (info) return info->fileInfo(); return QFileInfo(); }
    inline bool isFile() const { if (info) return info->isFile(); return true; }
    inline bool isSystem() const { if (info) return info->isSystem(); return true; }
    inline bool isHidden() const { if (info) return info->isHidden(); return false; }
    inline bool isSymLink(bool ignoreNtfsSymLinks = false) const { return info && info->isSymLink(ignoreNtfsSymLinks); }
    inline bool caseSensitive() const { return fileName.caseSensitivity == Qt::CaseSensitive; }
    inline Qt::CaseSensitivity caseSensitivity() const { return fileName.caseSensitivity; }
    inline QIcon icon() const { if (info) return info->icon; return QIcon(); }

    friend bool operator<(const FileSystemNode &a, const FileSystemNode &b) {
        return a.fileName < b.fileName;
    }
    friend bool operator>(const FileSystemNode &a, const FileSystemNode &b) {
        return a.fileName > b.fileName;
    }
    friend bool operator==(const FileSystemNode &a, const FileSystemNode &b) {
        return a.fileName == b.fileName;
    }

    friend bool operator!=(const FileSystemNode &a, const ExtendedInformation &fileInfo) {
        return !(a == fileInfo);
    }
    friend bool operator==(const FileSystemNode &a, const ExtendedInformation &fileInfo) {
        return a.info && (*a.info == fileInfo);
    }

    inline bool hasInformation() const { return info != nullptr; }

    void populate(const ExtendedInformation &fileInfo) {
        if (!info)
            info = new ExtendedInformation(fileInfo.fileInfo());
        (*info) = fileInfo;
    }

    // children shouldn't normally be accessed directly, use node()
    inline int visibleLocation(const PathKey &childName) {
        return visibleChildren.indexOf(childName);
    }
    void updateIcon(QFileIconProvider *iconProvider, const QString &path) {
        if (info)
            info->icon = iconProvider->icon(QFileInfo(path));
        for (FileSystemNode *child : std::as_const(children)) {
            //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
            if (!path.isEmpty()) {
                if (path.endsWith(QLatin1Char('/')))
                    child->updateIcon(iconProvider, path + child->fileName.data);
                else
                    child->updateIcon(iconProvider, path + QLatin1Char('/') + child->fileName.data);
            } else
                child->updateIcon(iconProvider, child->fileName.data);
        }
    }

    void retranslateStrings(QFileIconProvider *iconProvider, const QString &path) {
        if (info)
            info->displayType = iconProvider->type(QFileInfo(path));
        for (FileSystemNode *child : std::as_const(children)) {
            //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
            if (!path.isEmpty()) {
                if (path.endsWith(QLatin1Char('/')))
                    child->retranslateStrings(iconProvider, path + child->fileName.data);
                else
                    child->retranslateStrings(iconProvider, path + QLatin1Char('/') + child->fileName.data);
            } else
                child->retranslateStrings(iconProvider, child->fileName.data);
        }
    }

    QHash<PathKey, FileSystemNode *> children;
    QList<PathKey> visibleChildren;
    ExtendedInformation *info = nullptr;
    FileSystemNode *parent;
    int dirtyChildrenIndex = -1;
    bool populatedChildren = false;
    bool isVisible = false;
};

class FileSystemModelPrivate
{
public:
    enum { NumColumns = 4 };

    FileSystemModelPrivate(FileSystemModel *q) : q(q), mySlots(this, q)
    {
        init();
    }

    inline bool indexValid(const QModelIndex &index) const {
         return (index.row() >= 0) && (index.column() >= 0) && (index.model() == q);
    }

    void init();
    // Return true if index which is owned by node is hidden by the filter.
    bool isHiddenByFilter(FileSystemNode *indexNode, const QModelIndex &index) const
    {
       return (indexNode != &root && !index.isValid());
    }
    FileSystemNode *node(const QModelIndex &index) const;
    FileSystemNode *node(const QString &path, bool fetch = true) const;
    inline QModelIndex index(const QString &path, int column = 0) { return index(node(path), column); }
    QModelIndex index(const FileSystemNode *node, int column = 0) const;
    bool filtersAcceptsNode(const FileSystemNode *node) const;
    bool passNameFilters(const FileSystemNode *node) const;
    void removeNode(FileSystemNode *parentNode, const PathKey &name);
    FileSystemNode *addNode(FileSystemNode *parentNode, const PathKey &fileName, const QFileInfo &info);
    void addVisibleFiles(FileSystemNode *parentNode, const PathKeys &newFiles);
    void removeVisibleFile(FileSystemNode *parentNode, int visibleLocation);
    void sortChildren(int column, const QModelIndex &parent);

    inline int translateVisibleLocation(FileSystemNode *parent, int row) const {
        if (sortOrder != Qt::AscendingOrder) {
            if (parent->dirtyChildrenIndex == -1)
                return parent->visibleChildren.count() - row - 1;

            if (row < parent->dirtyChildrenIndex)
                return parent->dirtyChildrenIndex - row - 1;
        }

        return row;
    }

    static QString myComputer()
    {
        // ### TODO We should query the system to find out what the string should be
        // XP == "My Computer",
        // Vista == "Computer",
        // OS X == "Computer" (sometime user generated) "Benjamin's PowerBook G4"
        if (HostOsInfo::isWindowsHost())
            return Tr::tr("My Computer");
        return Tr::tr("Computer");
    }

    inline void delayedSort() {
        if (!delayedSortTimer.isActive())
            delayedSortTimer.start(0);
    }

    QIcon icon(const QModelIndex &index) const;
    QString name(const QModelIndex &index) const;
    QString displayName(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;
    QString size(const QModelIndex &index) const;
    static QString size(qint64 bytes);
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    void _q_directoryChanged(const QString &directory, const QStringList &list);
    void _q_performDelayedSort();
    void _q_fileSystemChanged(const QString &path, const QList<QPair<QString, QFileInfo>> &);
    void _q_resolvedName(const QString &fileName, const QString &resolvedName);

    FileSystemModel * const q;

    QDir rootDir;
    //Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive; // FIXME: Set properly

    // Next two used on Windows only
    QStringList unwatchPathsAt(const QModelIndex &);
    void watchPaths(const QStringList &paths) { fileInfoGatherer.watchPaths(paths); }

    FileInfoGatherer fileInfoGatherer;
    FileSystemModelSlots mySlots;

    QTimer delayedSortTimer;
    QHash<const FileSystemNode*, bool> bypassFilters;
    QStringList nameFilters;
    std::vector<QRegularExpression> nameFiltersRegexps;
    void rebuildNameFilterRegexps();

    QHash<QString, QString> resolvedSymLinks;

    FileSystemNode root;

    struct Fetching {
        QString dir;
        QString file;
        const FileSystemNode *node;
    };
    QList<Fetching> toFetch;

    QBasicTimer fetchingTimer;

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs;
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    bool forceSort = true;
    bool readOnly = true;
    bool setRootPath = false;
    bool nameFilterDisables = true; // false on windows, true on mac and unix
    // This flag is an optimization for QFileDialog. It enables a sort which is
    // not recursive, meaning we sort only what we see.
    bool disableRecursiveSort = false;
};


QFileInfo FileSystemModel::fileInfo(const QModelIndex &index) const
{
    return d->node(index)->fileInfo();
}

/*!
    \fn void FileSystemModel::rootPathChanged(const QString &newPath);

    This signal is emitted whenever the root path has been changed to a \a newPath.
*/

/*!
    \fn void FileSystemModel::fileRenamed(const QString &path, const QString &oldName, const QString &newName)

    This signal is emitted whenever a file with the \a oldName is successfully
    renamed to \a newName.  The file is located in in the directory \a path.
*/

/*!
    \since 4.7
    \fn void FileSystemModel::directoryLoaded(const QString &path)

    This signal is emitted when the gatherer thread has finished to load the \a path.

*/

/*!
    \fn bool FileSystemModel::remove(const QModelIndex &index)

    Removes the model item \a index from the file system model and \b{deletes the
    corresponding file from the file system}, returning true if successful. If the
    item cannot be removed, false is returned.

    \warning This function deletes files from the file system; it does \b{not}
    move them to a location where they can be recovered.

    \sa rmdir()
*/

bool FileSystemModel::remove(const QModelIndex &aindex)
{

    const QString path = d->filePath(aindex);
    const QFileInfo fileInfo(path);
    QStringList watchedPaths;
    // FIXME: This is reported as "Done" in Qt 5.11
    if (useFileSystemWatcher() && HostOsInfo::isWindowsHost())  {
        // QTBUG-65683: Remove file system watchers prior to deletion to prevent
        // failure due to locked files on Windows.
        d->unwatchPathsAt(aindex);
    }
    const bool success = (fileInfo.isFile() || fileInfo.isSymLink())
            ? QFile::remove(path) : QDir(path).removeRecursively();
    if (!success && useFileSystemWatcher() && HostOsInfo::isWindowsHost())
        d->watchPaths(watchedPaths);

    return success;
}

/*!
  Constructs a file system model with the given \a parent.
*/
FileSystemModel::FileSystemModel(QObject *parent)
    : QAbstractItemModel(parent),
    d(new FileSystemModelPrivate(this))
{
}

/*!
  Destroys this file system model.
*/
FileSystemModel::~FileSystemModel()
{
    delete d;
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // get the parent node
    FileSystemNode *parentNode = (d->indexValid(parent) ? d->node(parent) :
                                                   const_cast<FileSystemNode*>(&d->root));
    Q_ASSERT(parentNode);

    // now get the internal pointer for the index
    const int i = d->translateVisibleLocation(parentNode, row);
    if (i >= parentNode->visibleChildren.size())
        return QModelIndex();
    const PathKey childName = parentNode->visibleChildren.at(i);
    const FileSystemNode *indexNode = parentNode->children.value(childName);
    Q_ASSERT(indexNode);

    return createIndex(row, column, const_cast<FileSystemNode*>(indexNode));
}

QModelIndex FileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (row == idx.row() && column < FileSystemModelPrivate::NumColumns) {
        // cheap sibling operation: just adjust the column:
        return createIndex(row, column, idx.internalPointer());
    } else {
        // for anything else: call the default implementation
        // (this could probably be optimized, too):
        return QAbstractItemModel::sibling(row, column, idx);
    }
}

/*!
    \overload

    Returns the model item index for the given \a path and \a column.
*/
QModelIndex FileSystemModel::index(const QString &path, int column) const
{
    FileSystemNode *node = d->node(path, false);
    return d->index(node, column);
}

/*!
    \internal

    Return the FileSystemNode that goes to index.
  */
FileSystemNode *FileSystemModelPrivate::node(const QModelIndex &index) const
{
    if (!index.isValid())
        return const_cast<FileSystemNode*>(&root);
    FileSystemNode *indexNode = static_cast<FileSystemNode*>(index.internalPointer());
    Q_ASSERT(indexNode);
    return indexNode;
}

static QString qt_GetLongPathName(const QString &strShortPath)
{
#ifdef Q_OS_WIN32
    if (strShortPath.isEmpty()
        || strShortPath == QLatin1String(".") || strShortPath == QLatin1String(".."))
        return strShortPath;
    if (strShortPath.length() == 2 && strShortPath.endsWith(QLatin1Char(':')))
        return strShortPath.toUpper();
    const QString absPath = QDir(strShortPath).absolutePath();
    if (absPath.startsWith(QLatin1String("//"))
        || absPath.startsWith(QLatin1String("\\\\"))) // unc
        return QDir::fromNativeSeparators(absPath);
    if (absPath.startsWith(QLatin1Char('/')))
        return QString();
    const QString inputString = QLatin1String("\\\\?\\") + QDir::toNativeSeparators(absPath);
    QVarLengthArray<TCHAR, MAX_PATH> buffer(MAX_PATH);
    DWORD result = ::GetLongPathName((wchar_t*)inputString.utf16(),
                                     buffer.data(),
                                     buffer.size());
    if (result > DWORD(buffer.size())) {
        buffer.resize(result);
        result = ::GetLongPathName((wchar_t*)inputString.utf16(),
                                   buffer.data(),
                                   buffer.size());
    }
    if (result > 4) {
        QString longPath = QString::fromWCharArray(buffer.data() + 4); // ignoring prefix
        longPath[0] = longPath.at(0).toUpper(); // capital drive letters
        return QDir::fromNativeSeparators(longPath);
    } else {
        return QDir::fromNativeSeparators(strShortPath);
    }
#else
    return strShortPath;
#endif
}

/*!
    \internal

    Given a path return the matching FileSystemNode or &root if invalid
*/
FileSystemNode *FileSystemModelPrivate::node(const QString &path, bool fetch) const
{
    if (path.isEmpty() || path == myComputer() || path.startsWith(QLatin1Char(':')))
        return const_cast<FileSystemNode*>(&root);

    // Construct the nodes up to the new root path if they need to be built
    QString absolutePath;
    QString longPath = qt_GetLongPathName(path);
    if (longPath == rootDir.path())
        absolutePath = rootDir.absolutePath();
    else
        absolutePath = QDir(longPath).absolutePath();

    // ### TODO can we use bool QAbstractFileEngine::caseSensitive() const?
    QStringList pathElements = absolutePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if ((pathElements.isEmpty())
#if !defined(Q_OS_WIN)
        && QDir::fromNativeSeparators(longPath) != QLatin1String("/")
#endif
        )
        return const_cast<FileSystemNode*>(&root);
    QModelIndex index = QModelIndex(); // start with "My Computer"
    QString elementPath;
    QChar separator = QLatin1Char('/');
    QString trailingSeparator;
    if (HostOsInfo::isWindowsHost()) {
        if (absolutePath.startsWith(QLatin1String("//"))) { // UNC path
            QString host = QLatin1String("\\\\") + pathElements.constFirst();
            if (absolutePath == QDir::fromNativeSeparators(host))
                absolutePath.append(QLatin1Char('/'));
            if (longPath.endsWith(QLatin1Char('/')) && !absolutePath.endsWith(QLatin1Char('/')))
                absolutePath.append(QLatin1Char('/'));
            if (absolutePath.endsWith(QLatin1Char('/')))
                trailingSeparator = QLatin1String("\\");
            int r = 0;
            auto rootNode = const_cast<FileSystemNode*>(&root);
            auto it = root.children.constFind(PathKey(host, rootNode->caseSensitivity()));
            if (it != root.children.cend()) {
                host = it.key().data; // Normalize case for lookup in visibleLocation()
            } else {
                const PathKey hostKey{host, rootNode->caseSensitivity()};
                if (pathElements.count() == 1 && !absolutePath.endsWith(QLatin1Char('/')))
                    return rootNode;
                QFileInfo info(host);
                if (!info.exists())
                    return rootNode;
                FileSystemModelPrivate *p = const_cast<FileSystemModelPrivate*>(this);
                p->addNode(rootNode, hostKey, info);
                p->addVisibleFiles(rootNode, {hostKey});
            }
            const PathKey hostKey{host, rootNode->caseSensitivity()};
            r = rootNode->visibleLocation(hostKey);
            r = translateVisibleLocation(rootNode, r);
            index = q->index(r, 0, QModelIndex());
            pathElements.pop_front();
            separator = QLatin1Char('\\');
            elementPath = host;
            elementPath.append(separator);
        } else {
            if (!pathElements.at(0).contains(QLatin1Char(':')))
                pathElements.prepend(HostOsInfo::root().path());
            if (pathElements.at(0).endsWith(QLatin1Char('/')))
                pathElements[0].chop(1);
        }
    } else {
        // add the "/" item, since it is a valid path element on Unix
        if (absolutePath[0] == QLatin1Char('/'))
            pathElements.prepend(QLatin1String("/"));
    }

    FileSystemNode *parent = node(index);

    for (int i = 0; i < pathElements.count(); ++i) {
        QString element = pathElements.at(i);
        if (i != 0)
            elementPath.append(separator);
        elementPath.append(element);
        if (i == pathElements.count() - 1)
            elementPath.append(trailingSeparator);

        if (HostOsInfo::isWindowsHost()) {
            // On Windows, "filename    " and "filename" are equivalent and
            // "filename  .  " and "filename" are equivalent
            // "filename......." and "filename" are equivalent Task #133928
            // whereas "filename  .txt" is still "filename  .txt"
            // If after stripping the characters there is nothing left then we
            // just return the parent directory as it is assumed that the path
            // is referring to the parent
            while (element.endsWith(QLatin1Char('.')) || element.endsWith(QLatin1Char(' ')))
                element.chop(1);
            // Only filenames that can't possibly exist will be end up being empty
            if (element.isEmpty())
                return parent;
        }
        const PathKey elementKey(element, parent->caseSensitivity());
        bool alreadyExisted = parent->children.contains(elementKey);

        // we couldn't find the path element, we create a new node since we
        // _know_ that the path is valid
        if (alreadyExisted) {
            if (parent->children.count() == 0
                    || parent->children.value(elementKey)->fileName != elementKey)
                alreadyExisted = false;
        }

        FileSystemNode *node;
        if (!alreadyExisted) {
            // Someone might call ::index("file://cookie/monster/doesn't/like/veggies"),
            // a path that doesn't exists, I.E. don't blindly create directories.
            QFileInfo info(elementPath);
            if (!info.exists())
                return const_cast<FileSystemNode*>(&root);
            FileSystemModelPrivate *p = const_cast<FileSystemModelPrivate*>(this);
            node = p->addNode(parent, PathKey(element, parent->caseSensitivity()), info);
            if (useFileSystemWatcher())
                node->populate(fileInfoGatherer.getInfo(info));
        } else {
            node = parent->children.value(elementKey);
        }

        Q_ASSERT(node);
        if (!node->isVisible) {
            // It has been filtered out
            if (alreadyExisted && node->hasInformation() && !fetch)
                return const_cast<FileSystemNode*>(&root);

            FileSystemModelPrivate *p = const_cast<FileSystemModelPrivate*>(this);
            p->addVisibleFiles(parent, {elementKey});
            if (!p->bypassFilters.contains(node))
                p->bypassFilters[node] = 1;
            QString dir = q->filePath(this->index(parent));
            if (!node->hasInformation() && fetch) {
                Fetching f = { std::move(dir), std::move(element), node };
                p->toFetch.append(std::move(f));
                p->fetchingTimer.start(0, const_cast<FileSystemModel*>(q));
            }
        }
        parent = node;
    }

    return parent;
}

void FileSystemModel::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->fetchingTimer.timerId()) {
        d->fetchingTimer.stop();
        if (useFileSystemWatcher()) {
            for (int i = 0; i < d->toFetch.count(); ++i) {
                const FileSystemNode *node = d->toFetch.at(i).node;
                if (!node->hasInformation()) {
                    d->fileInfoGatherer.fetchExtendedInformation(d->toFetch.at(i).dir,
                                                                 QStringList(d->toFetch.at(i).file));
                }
            }
        }
        d->toFetch.clear();
    }
}

/*!
    Returns \c true if the model item \a index represents a directory;
    otherwise returns \c false.
*/
bool FileSystemModel::isDir(const QModelIndex &index) const
{
    // This function is for public usage only because it could create a file info
    if (!index.isValid())
        return true;
    FileSystemNode *n = d->node(index);
    if (n->hasInformation())
        return n->isDir();
    return fileInfo(index).isDir();
}

/*!
    Returns the size in bytes of \a index. If the file does not exist, 0 is returned.
  */
qint64 FileSystemModel::size(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    return d->node(index)->size();
}

/*!
    Returns the type of file \a index such as "Directory" or "JPEG file".
  */
QString FileSystemModel::type(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    return d->node(index)->type();
}

/*!
    Returns the date and time when \a index was last modified.
 */
QDateTime FileSystemModel::lastModified(const QModelIndex &index) const
{
    if (!index.isValid())
        return QDateTime();
    return d->node(index)->lastModified();
}

QModelIndex FileSystemModel::parent(const QModelIndex &index) const
{
    if (!d->indexValid(index))
        return QModelIndex();

    FileSystemNode *indexNode = d->node(index);
    Q_ASSERT(indexNode != nullptr);
    FileSystemNode *parentNode = indexNode->parent;
    if (parentNode == nullptr || parentNode == &d->root)
        return QModelIndex();

    // get the parent's row
    FileSystemNode *grandParentNode = parentNode->parent;
    Q_ASSERT(grandParentNode->children.contains(parentNode->fileName));
    int visualRow = d->translateVisibleLocation(grandParentNode,
                                                grandParentNode->visibleLocation(
                                                    grandParentNode->children.value(parentNode->fileName)->fileName));
    if (visualRow == -1)
        return QModelIndex();
    return createIndex(visualRow, 0, parentNode);
}

/*
    \internal

    return the index for node
*/
QModelIndex FileSystemModelPrivate::index(const FileSystemNode *node, int column) const
{
    FileSystemNode *parentNode = (node ? node->parent : nullptr);
    if (node == &root || !parentNode)
        return QModelIndex();

    // get the parent's row
    Q_ASSERT(node);
    if (!node->isVisible)
        return QModelIndex();

    int visualRow = translateVisibleLocation(parentNode, parentNode->visibleLocation(node->fileName));
    return q->createIndex(visualRow, column, const_cast<FileSystemNode*>(node));
}

bool FileSystemModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return false;

    if (!parent.isValid()) // drives
        return true;

    const FileSystemNode *indexNode = d->node(parent);
    Q_ASSERT(indexNode);
    return (indexNode->isDir());
}

bool FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if (!d->setRootPath)
        return false;
    const FileSystemNode *indexNode = d->node(parent);
    return (!indexNode->populatedChildren);
}

void FileSystemModel::fetchMore(const QModelIndex &parent)
{
    if (!d->setRootPath)
        return;
    FileSystemNode *indexNode = d->node(parent);
    if (indexNode->populatedChildren)
        return;
    indexNode->populatedChildren = true;
    if (useFileSystemWatcher())
        d->fileInfoGatherer.list(filePath(parent));
}

int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return d->root.visibleChildren.count();

    const FileSystemNode *parentNode = d->node(parent);
    return parentNode->visibleChildren.count();
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    return (parent.column() > 0) ? 0 : FileSystemModelPrivate::NumColumns;
}

/*!
    Returns the data stored under the given \a role for the item "My Computer".

    \sa Qt::ItemDataRole
 */
QVariant FileSystemModel::myComputer(int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        return FileSystemModelPrivate::myComputer();
    case Qt::DecorationRole:
        if (useFileSystemWatcher())
            return d->fileInfoGatherer.iconProvider()->icon(QFileIconProvider::Computer);
    }
    return QVariant();
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this)
        return QVariant();

    switch (role) {
    case Qt::EditRole:
        if (index.column() == 0)
            return d->name(index);
        Q_FALLTHROUGH();
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return d->displayName(index);
        case 1: return d->size(index);
        case 2: return d->type(index);
        case 3: return d->time(index);
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
        }
        break;
    case FilePathRole:
        return filePath(index);
    case FileNameRole:
        return d->name(index);
    case Qt::DecorationRole:
        if (index.column() == 0) {
            QIcon icon = d->icon(index);
            if (useFileSystemWatcher() && icon.isNull()) {
                if (d->node(index)->isDir())
                    icon = d->fileInfoGatherer.iconProvider()->icon(QFileIconProvider::Folder);
                else
                    icon = d->fileInfoGatherer.iconProvider()->icon(QFileIconProvider::File);
            }
            return icon;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return QVariant(Qt::AlignTrailing | Qt::AlignVCenter);
        break;
    case FilePermissions:
        int p = permissions(index);
        return p;
    }

    return QVariant();
}

QString FileSystemModelPrivate::size(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    const FileSystemNode *n = node(index);
    if (n->isDir()) {
        if (HostOsInfo::isMacHost())
            return QLatin1String("--");
        else
            return QLatin1String("");
    // Windows   - ""
    // OS X      - "--"
    // Konqueror - "4 KB"
    // Nautilus  - "9 items" (the number of children)
    }
    return size(n->size());
}

QString FileSystemModelPrivate::size(qint64 bytes)
{
    return QLocale::system().formattedDataSize(bytes);
}

QString FileSystemModelPrivate::time(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    return QLocale::system().toString(node(index)->lastModified(), QLocale::ShortFormat);
}

QString FileSystemModelPrivate::type(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    return node(index)->type();
}

QString FileSystemModelPrivate::name(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    FileSystemNode *dirNode = node(index);
    if (fileInfoGatherer.resolveSymlinks() &&
            !resolvedSymLinks.isEmpty() && dirNode->isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QString fullPath = QDir::fromNativeSeparators(filePath(index));
        return resolvedSymLinks.value(fullPath, dirNode->fileName.data);
    }
    return dirNode->fileName.data;
}

QString FileSystemModelPrivate::displayName(const QModelIndex &index) const
{
    if (HostOsInfo::isWindowsHost()) {
        FileSystemNode *dirNode = node(index);
        if (!dirNode->volumeName.isEmpty())
            return dirNode->volumeName;
    }
    return name(index);
}

QIcon FileSystemModelPrivate::icon(const QModelIndex &index) const
{
    if (!index.isValid())
        return QIcon();
    return node(index)->icon();
}

bool FileSystemModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid()
        || idx.column() != 0
        || role != Qt::EditRole
        || (flags(idx) & Qt::ItemIsEditable) == 0) {
        return false;
    }

    QString newName = value.toString();
    QString oldName = idx.data().toString();
    if (newName == oldName)
        return true;

    const QString parentPath = filePath(parent(idx));

    if (newName.isEmpty() || QDir::toNativeSeparators(newName).contains(QDir::separator()))
        return false;

    QStringList watchedPaths;
    if (useFileSystemWatcher() && HostOsInfo::isWindowsHost()) {
        // FIXME: Probably no more relevant
        // QTBUG-65683: Remove file system watchers prior to renaming to prevent
        // failure due to locked files on Windows.
         watchedPaths = d->unwatchPathsAt(idx);
    }
    if (!QDir(parentPath).rename(oldName, newName)) {
        if (useFileSystemWatcher() && HostOsInfo::isWindowsHost())
            d->watchPaths(watchedPaths);
        return false;
    } else {
        /*
            *After re-naming something we don't want the selection to change*
            - can't remove rows and later insert
            - can't quickly remove and insert
            - index pointer can't change because treeview doesn't use persistent index's

            - if this get any more complicated think of changing it to just
              use layoutChanged
         */

        FileSystemNode *indexNode = d->node(idx);
        FileSystemNode *parentNode = indexNode->parent;
        int visibleLocation = parentNode->visibleLocation(parentNode->children.value(indexNode->fileName)->fileName);

        const PathKey newNameKey{newName, indexNode->caseSensitivity()};
        const PathKey oldNameKey{oldName, indexNode->caseSensitivity()};
        parentNode->visibleChildren.removeAt(visibleLocation);
        QScopedPointer<FileSystemNode> nodeToRename(parentNode->children.take(oldNameKey));
        nodeToRename->fileName = newNameKey;
        nodeToRename->parent = parentNode;
        if (useFileSystemWatcher())
            nodeToRename->populate(d->fileInfoGatherer.getInfo(QFileInfo(parentPath, newName)));
        nodeToRename->isVisible = true;
        parentNode->children[newNameKey] = nodeToRename.take();
        parentNode->visibleChildren.insert(visibleLocation, newNameKey);

        d->delayedSort();
        emit fileRenamed(parentPath, oldName, newName);
    }
    return true;
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            // ### TODO oh man this is ugly and doesn't even work all the way!
            // it is still 2 pixels off
            QImage pixmap(16, 1, QImage::Format_ARGB32_Premultiplied);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case 0: returnValue = Tr::tr("Name");
            break;
    case 1: returnValue = Tr::tr("Size");
            break;
    case 2: returnValue = HostOsInfo::isMacHost()
                    ? Tr::tr("Kind", "Match OS X Finder")
                    : Tr::tr("Type", "All other platforms");
           break;
    // Windows   - Type
    // OS X      - Kind
    // Konqueror - File Type
    // Nautilus  - Type
    case 3: returnValue = Tr::tr("Date Modified");
            break;
    default: return QVariant();
    }
    return returnValue;
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return flags;

    FileSystemNode *indexNode = d->node(index);
    if (d->nameFilterDisables && !d->passNameFilters(indexNode)) {
        flags &= ~Qt::ItemIsEnabled;
        // ### TODO you shouldn't be able to set this as the current item, task 119433
        return flags;
    }

    flags |= Qt::ItemIsDragEnabled;
    if (d->readOnly)
        return flags;
    if ((index.column() == 0) && indexNode->permissions() & QFile::WriteUser) {
        flags |= Qt::ItemIsEditable;
        if (indexNode->isDir())
            flags |= Qt::ItemIsDropEnabled;
        else
            flags |= Qt::ItemNeverHasChildren;
    }
    return flags;
}

void FileSystemModelPrivate::_q_performDelayedSort()
{
    q->sort(sortColumn, sortOrder);
}

/*
    \internal
    Helper functor used by sort()
*/
class QFileSystemModelSorter
{
public:
    inline QFileSystemModelSorter(int column) : sortColumn(column)
    {
        naturalCompare.setNumericMode(true);
        naturalCompare.setCaseSensitivity(Qt::CaseInsensitive);
    }

    bool compareNodes(const FileSystemNode *l, const FileSystemNode *r) const
    {
        switch (sortColumn) {
        case 0: {
            if (!HostOsInfo::isMacHost()) {
                // place directories before files
                bool left = l->isDir();
                bool right = r->isDir();
                if (left ^ right)
                    return left;
            }
            return naturalCompare.compare(l->fileName.data, r->fileName.data) < 0;
        }
        case 1:
        {
            // Directories go first
            bool left = l->isDir();
            bool right = r->isDir();
            if (left ^ right)
                return left;

            qint64 sizeDifference = l->size() - r->size();
            if (sizeDifference == 0)
                return naturalCompare.compare(l->fileName.data, r->fileName.data) < 0;

            return sizeDifference < 0;
        }
        case 2:
        {
            int compare = naturalCompare.compare(l->type(), r->type());
            if (compare == 0)
                return naturalCompare.compare(l->fileName.data, r->fileName.data) < 0;

            return compare < 0;
        }
        case 3:
        {
            if (l->lastModified() == r->lastModified())
                return naturalCompare.compare(l->fileName.data, r->fileName.data) < 0;

            return l->lastModified() < r->lastModified();
        }
        }
        Q_ASSERT(false);
        return false;
    }

    bool operator()(const FileSystemNode *l, const FileSystemNode *r) const
    {
        return compareNodes(l, r);
    }

private:
    QCollator naturalCompare;
    int sortColumn;
};

/*
    \internal

    Sort all of the children of parent
*/
void FileSystemModelPrivate::sortChildren(int column, const QModelIndex &parent)
{
    FileSystemNode *indexNode = node(parent);
    if (indexNode->children.count() == 0)
        return;

    QList<FileSystemNode *> values;

    for (auto iterator = indexNode->children.constBegin(), cend = indexNode->children.constEnd(); iterator != cend; ++iterator) {
        if (filtersAcceptsNode(iterator.value())) {
            values.append(iterator.value());
        } else {
            iterator.value()->isVisible = false;
        }
    }
    QFileSystemModelSorter ms(column);
    std::sort(values.begin(), values.end(), ms);
    // First update the new visible list
    indexNode->visibleChildren.clear();
    //No more dirty item we reset our internal dirty index
    indexNode->dirtyChildrenIndex = -1;
    const int numValues = values.count();
    indexNode->visibleChildren.reserve(numValues);
    for (int i = 0; i < numValues; ++i) {
        indexNode->visibleChildren.append(values.at(i)->fileName);
        values.at(i)->isVisible = true;
    }

    if (!disableRecursiveSort) {
        for (int i = 0; i < q->rowCount(parent); ++i) {
            const QModelIndex childIndex = q->index(i, 0, parent);
            FileSystemNode *indexNode = node(childIndex);
            //Only do a recursive sort on visible nodes
            if (indexNode->isVisible)
                sortChildren(column, childIndex);
        }
    }
}

void FileSystemModel::sort(int column, Qt::SortOrder order)
{
    if (d->sortOrder == order && d->sortColumn == column && !d->forceSort)
        return;

    emit layoutAboutToBeChanged();
    QModelIndexList oldList = persistentIndexList();
    QList<QPair<FileSystemNode *, int>> oldNodes;
    const int nodeCount = oldList.count();
    oldNodes.reserve(nodeCount);
    for (int i = 0; i < nodeCount; ++i) {
        const QModelIndex &oldNode = oldList.at(i);
        QPair<FileSystemNode*, int> pair(d->node(oldNode), oldNode.column());
        oldNodes.append(pair);
    }

    if (!(d->sortColumn == column && d->sortOrder != order && !d->forceSort)) {
        //we sort only from where we are, don't need to sort all the model
        d->sortChildren(column, index(rootPath()));
        d->sortColumn = column;
        d->forceSort = false;
    }
    d->sortOrder = order;

    QModelIndexList newList;
    const int numOldNodes = oldNodes.size();
    newList.reserve(numOldNodes);
    for (int i = 0; i < numOldNodes; ++i) {
        const QPair<FileSystemNode*, int> &oldNode = oldNodes.at(i);
        newList.append(d->index(oldNode.first, oldNode.second));
    }
    changePersistentIndexList(oldList, newList);
    emit layoutChanged();
}

/*!
    Returns a list of MIME types that can be used to describe a list of items
    in the model.
*/
QStringList FileSystemModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    Returns an object that contains a serialized description of the specified
    \a indexes. The format used to describe the items corresponding to the
    indexes is obtained from the mimeTypes() function.

    If the list of indexes is empty, \c nullptr is returned rather than a
    serialized empty list.
*/
QMimeData *FileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
        if ((*it).column() == 0)
            urls << QUrl::fromLocalFile(filePath(*it));
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action over the row in the model specified by the \a row and
    \a column and by the \a parent index. Returns true if the operation was
    successful.

    \sa supportedDropActions()
*/
bool FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (!parent.isValid() || isReadOnly())
        return false;

    bool success = true;
    QString to = filePath(parent) + QDir::separator();

    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    switch (action) {
    case Qt::CopyAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::copy(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::LinkAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::link(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::MoveAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::rename(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    default:
        return false;
    }

    return success;
}

Qt::DropActions FileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

QHash<int, QByteArray> FileSystemModel::roleNames() const
{
    auto ret = QAbstractItemModel::roleNames();
    ret.insert(FileSystemModel::FileIconRole,
               QByteArrayLiteral("fileIcon")); // == Qt::decoration
    ret.insert(FileSystemModel::FilePathRole, QByteArrayLiteral("filePath"));
    ret.insert(FileSystemModel::FileNameRole, QByteArrayLiteral("fileName"));
    ret.insert(FileSystemModel::FilePermissions, QByteArrayLiteral("filePermissions"));
    return ret;
}

/*!
    \internal
    \enum Utils::FileSystemModel::Option
    \since 5.14

    \value DontWatchForChanges Do not add file watchers to the paths.
    This reduces overhead when using the model for simple tasks
    like line edit completion.

    \value DontResolveSymlinks Don't resolve symlinks in the file
    system model. By default, symlinks are resolved.

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    causes a big performance impact over network or removable drives.
    This sets the QFileIconProvider::DontUseCustomDirectoryIcons
    option in the icon provider accordingly.

    \sa resolveSymlinks
*/

/*!
    \since 5.14
    Sets the given \a option to be enabled if \a on is true; otherwise,
    clears the given \a option.

    Options should be set before changing properties.

    \sa options, testOption()
*/
void FileSystemModel::setOption(Option option, bool on)
{
    FileSystemModel::Options previousOptions = options();
    setOptions(previousOptions.setFlag(option, on));
}

/*!
    \since 5.14

    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool FileSystemModel::testOption(Option option) const
{
    return options().testFlag(option);
}

/*!
    \internal
    \property FileSystemModel::options
    \brief the various options that affect the model
    \since 5.14

    By default, all options are disabled.

    Options should be set before changing properties.

    \sa setOption(), testOption()
*/
void FileSystemModel::setOptions(Options options)
{
    const Options changed = (options ^ FileSystemModel::options());

    if (changed.testFlag(DontResolveSymlinks))
        setResolveSymlinks(!options.testFlag(DontResolveSymlinks));

    if (useFileSystemWatcher() && changed.testFlag(DontWatchForChanges))
        d->fileInfoGatherer.setWatching(!options.testFlag(DontWatchForChanges));

    if (changed.testFlag(DontUseCustomDirectoryIcons)) {
        if (auto provider = iconProvider()) {
            QFileIconProvider::Options providerOptions = provider->options();
            providerOptions.setFlag(QFileIconProvider::DontUseCustomDirectoryIcons,
                                    options.testFlag(FileSystemModel::DontUseCustomDirectoryIcons));
            provider->setOptions(providerOptions);
        } else {
            qWarning("Setting FileSystemModel::DontUseCustomDirectoryIcons has no effect when no provider is used");
        }
    }
}

FileSystemModel::Options FileSystemModel::options() const
{
    FileSystemModel::Options result;
    result.setFlag(DontResolveSymlinks, !resolveSymlinks());
    if (useFileSystemWatcher())
        result.setFlag(DontWatchForChanges, !d->fileInfoGatherer.isWatching());
    else
        result.setFlag(DontWatchForChanges);
    if (auto provider = iconProvider()) {
        result.setFlag(DontUseCustomDirectoryIcons,
                       provider->options().testFlag(QFileIconProvider::DontUseCustomDirectoryIcons));
    }
    return result;
}

/*!
    Returns the path of the item stored in the model under the
    \a index given.
*/
QString FileSystemModel::filePath(const QModelIndex &index) const
{
    QString fullPath = d->filePath(index);
    FileSystemNode *dirNode = d->node(index);
    if (dirNode->isSymLink()
        && d->fileInfoGatherer.resolveSymlinks()
        && d->resolvedSymLinks.contains(fullPath)
        && dirNode->isDir()) {
        QFileInfo fullPathInfo(dirNode->fileInfo());
        if (!dirNode->hasInformation())
            fullPathInfo = QFileInfo(fullPath);
        QString canonicalPath = fullPathInfo.canonicalFilePath();
        auto *canonicalNode = d->node(fullPathInfo.canonicalFilePath(), false);
        QFileInfo resolvedInfo = canonicalNode->fileInfo();
        if (!canonicalNode->hasInformation())
            resolvedInfo = QFileInfo(canonicalPath);
        if (resolvedInfo.exists())
            return resolvedInfo.filePath();
    }
    return fullPath;
}

QString FileSystemModelPrivate::filePath(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    Q_ASSERT(index.model() == q);

    QStringList path;
    QModelIndex idx = index;
    while (idx.isValid()) {
        FileSystemNode *dirNode = node(idx);
        if (dirNode)
            path.prepend(dirNode->fileName.data);
        idx = idx.parent();
    }
    QString fullPath = QDir::fromNativeSeparators(path.join(QDir::separator()));
    if (!HostOsInfo::isWindowsHost()) {
        if ((fullPath.length() > 2) && fullPath[0] == QLatin1Char('/') && fullPath[1] == QLatin1Char('/'))
            fullPath = fullPath.mid(1);
    } else {
        if (fullPath.length() == 2 && fullPath.endsWith(QLatin1Char(':')))
            fullPath.append(QLatin1Char('/'));
    }
    return fullPath;
}

/*!
    Create a directory with the \a name in the \a parent model index.
*/
QModelIndex FileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    if (!parent.isValid())
        return parent;

    QDir dir(filePath(parent));
    if (!dir.mkdir(name))
        return QModelIndex();
    FileSystemNode *parentNode = d->node(parent);
    PathKey nameKey(name, parentNode->caseSensitivity());
    d->addNode(parentNode, nameKey, QFileInfo());
    Q_ASSERT(parentNode->children.contains(nameKey));
    FileSystemNode *node = parentNode->children[nameKey];
    if (useFileSystemWatcher())
        node->populate(d->fileInfoGatherer.getInfo(QFileInfo(dir.absolutePath() + QDir::separator() + name)));
    d->addVisibleFiles(parentNode, {PathKey(name, parentNode->caseSensitivity())});
    return d->index(node);
}

/*!
    Returns the complete OR-ed together combination of QFile::Permission for the \a index.
 */
QFile::Permissions FileSystemModel::permissions(const QModelIndex &index) const
{
    return d->node(index)->permissions();
}

/*!
    Sets the directory that is being watched by the model to \a newPath by
    installing a \l{QFileSystemWatcher}{file system watcher} on it. Any
    changes to files and directories within this directory will be
    reflected in the model.

    If the path is changed, the rootPathChanged() signal will be emitted.

    \note This function does not change the structure of the model or
    modify the data available to views. In other words, the "root" of
    the model is \e not changed to include only files and directories
    within the directory specified by \a newPath in the file system.
  */
QModelIndex FileSystemModel::setRootPath(const QString &newPath)
{
    QString longNewPath = qt_GetLongPathName(newPath);
    //we remove .. and . from the given path if exist
    if (!newPath.isEmpty())
        longNewPath = QDir::cleanPath(longNewPath);

    d->setRootPath = true;

    //user don't ask for the root path ("") but the conversion failed
    if (!newPath.isEmpty() && longNewPath.isEmpty())
        return d->index(rootPath());

    if (d->rootDir.path() == longNewPath)
        return d->index(rootPath());

    auto node = d->node(longNewPath);
    QFileInfo newPathInfo;
    if (node && node->hasInformation())
        newPathInfo = node->fileInfo();
    else
        newPathInfo = QFileInfo(longNewPath);

    bool showDrives = (longNewPath.isEmpty() || longNewPath == FileSystemModelPrivate::myComputer());
    if (!showDrives && !newPathInfo.exists())
        return d->index(rootPath());

    //We remove the watcher on the previous path
    if (!rootPath().isEmpty() && rootPath() != QLatin1String(".")) {
        //This remove the watcher for the old rootPath
        if (useFileSystemWatcher())
            d->fileInfoGatherer.removePath(rootPath());
        //This line "marks" the node as dirty, so the next fetchMore
        //call on the path will ask the gatherer to install a watcher again
        //But it doesn't re-fetch everything
        d->node(rootPath())->populatedChildren = false;
    }

    // We have a new valid root path
    d->rootDir = QDir(longNewPath);
    QModelIndex newRootIndex;
    if (showDrives) {
        // otherwise dir will become '.'
        d->rootDir.setPath(QLatin1String(""));
    } else {
        newRootIndex = d->index(d->rootDir.path());
    }
    fetchMore(newRootIndex);
    emit rootPathChanged(longNewPath);
    d->forceSort = true;
    d->delayedSort();
    return newRootIndex;
}

/*!
    The currently set root path

    \sa rootDirectory()
*/
QString FileSystemModel::rootPath() const
{
    return d->rootDir.path();
}

/*!
    The currently set directory

    \sa rootPath()
*/
QDir FileSystemModel::rootDirectory() const
{
    QDir dir(d->rootDir);
    dir.setNameFilters(nameFilters());
    dir.setFilter(filter());
    return dir;
}

/*!
    Sets the \a provider of file icons for the directory model.
*/
void FileSystemModel::setIconProvider(QFileIconProvider *provider)
{
    if (useFileSystemWatcher())
        d->fileInfoGatherer.setIconProvider(provider);
    d->root.updateIcon(provider, QString());
}

/*!
    Returns the file icon provider for this directory model.
*/
QFileIconProvider *FileSystemModel::iconProvider() const
{
    if (useFileSystemWatcher()) {
        return d->fileInfoGatherer.iconProvider();
    }
    return nullptr;
}

/*!
    Sets the directory model's filter to that specified by \a filters.

    Note that the filter you set should always include the QDir::AllDirs enum value,
    otherwise FileSystemModel won't be able to read the directory structure.

    \sa QDir::Filters
*/
void FileSystemModel::setFilter(QDir::Filters filters)
{
    if (d->filters == filters)
        return;
    const bool changingCaseSensitivity =
        filters.testFlag(QDir::CaseSensitive) != d->filters.testFlag(QDir::CaseSensitive);
    d->filters = filters;
    if (changingCaseSensitivity)
        d->rebuildNameFilterRegexps();
    d->forceSort = true;
    d->delayedSort();
}

/*!
    Returns the filter specified for the directory model.

    If a filter has not been set, the default filter is QDir::AllEntries |
    QDir::NoDotAndDotDot | QDir::AllDirs.

    \sa QDir::Filters
*/
QDir::Filters FileSystemModel::filter() const
{
    return d->filters;
}

/*!
    \internal
    \property FileSystemModel::resolveSymlinks
    \brief Whether the directory model should resolve symbolic links

    This is only relevant on Windows.

    By default, this property is \c true.

    \sa FileSystemModel::Options
*/
void FileSystemModel::setResolveSymlinks(bool enable)
{
    if (useFileSystemWatcher()) {
        d->fileInfoGatherer.setResolveSymlinks(enable);
    }
}

bool FileSystemModel::resolveSymlinks() const
{
    if (useFileSystemWatcher()) {
        return d->fileInfoGatherer.resolveSymlinks();
    }
    return false;
}

/*!
    \internal
    \property FileSystemModel::readOnly
    \brief Whether the directory model allows writing to the file system

    If this property is set to false, the directory model will allow renaming, copying
    and deleting of files and directories.

    This property is \c true by default
*/
void FileSystemModel::setReadOnly(bool enable)
{
    d->readOnly = enable;
}

bool FileSystemModel::isReadOnly() const
{
    return d->readOnly;
}

/*!
    \internal
    \property FileSystemModel::nameFilterDisables
    \brief Whether files that don't pass the name filter are hidden or disabled

    This property is \c true by default
*/
void FileSystemModel::setNameFilterDisables(bool enable)
{
    if (d->nameFilterDisables == enable)
        return;
    d->nameFilterDisables = enable;
    d->forceSort = true;
    d->delayedSort();
}

bool FileSystemModel::nameFilterDisables() const
{
    return d->nameFilterDisables;
}

/*!
    Sets the name \a filters to apply against the existing files.
*/
void FileSystemModel::setNameFilters(const QStringList &filters)
{
    if (!d->bypassFilters.isEmpty()) {
        // update the bypass filter to only bypass the stuff that must be kept around
        d->bypassFilters.clear();
        // We guarantee that rootPath will stick around
        // TODO: root looks unused - does it really guarantee anything?
        QPersistentModelIndex root(index(rootPath()));
        const QModelIndexList persistentList = persistentIndexList();
        for (const auto &persistentIndex : persistentList) {
            FileSystemNode *node = d->node(persistentIndex);
            while (node) {
                if (d->bypassFilters.contains(node))
                    break;
                if (node->isDir())
                    d->bypassFilters[node] = true;
                node = node->parent;
            }
        }
    }

    d->nameFilters = filters;
    d->rebuildNameFilterRegexps();
    d->forceSort = true;
    d->delayedSort();
}

/*!
    Returns a list of filters applied to the names in the model.
*/
QStringList FileSystemModel::nameFilters() const
{
    return d->nameFilters;
}

bool FileSystemModel::event(QEvent *event)
{
    if (useFileSystemWatcher() && event->type() == QEvent::LanguageChange) {
        d->root.retranslateStrings(d->fileInfoGatherer.iconProvider(), QString());
        return true;
    }
    return QAbstractItemModel::event(event);
}

bool FileSystemModel::rmdir(const QModelIndex &aindex)
{
    QString path = filePath(aindex);
    const bool success = QDir().rmdir(path);
    if (useFileSystemWatcher() && success) {
        d->fileInfoGatherer.removePath(path);
    }
    return success;
}

QString FileSystemModel::fileName(const QModelIndex &aindex) const
{
    return aindex.data(Qt::DisplayRole).toString();
}

QIcon FileSystemModel::fileIcon(const QModelIndex &aindex) const
{
    return qvariant_cast<QIcon>(aindex.data(Qt::DecorationRole));
}

/*!
     \internal

    Performed quick listing and see if any files have been added or removed,
    then fetch more information on visible files.
 */
void FileSystemModelPrivate::_q_directoryChanged(const QString &directory, const QStringList &files)
{
    FileSystemNode *parentNode = node(directory, false);
    if (parentNode->children.count() == 0)
        return;
    QStringList toRemove;
    QStringList newFiles = files;
    std::sort(newFiles.begin(), newFiles.end());
    for (auto i = parentNode->children.constBegin(), cend = parentNode->children.constEnd(); i != cend; ++i) {
        QStringList::iterator iterator = std::lower_bound(newFiles.begin(), newFiles.end(), i.value()->fileName.data);
        if ((iterator == newFiles.end()) || (i.value()->fileName < PathKey(*iterator, parentNode->caseSensitivity())))
            toRemove.append(i.value()->fileName.data);
    }
    for (int i = 0 ; i < toRemove.count() ; ++i )
        removeNode(parentNode, PathKey(toRemove[i], parentNode->caseSensitivity()));
}

static QString volumeName(const QString &path)
{
#if defined(Q_OS_WIN)
    IShellItem *item = nullptr;
    const QString native = QDir::toNativeSeparators(path);
    HRESULT hr = SHCreateItemFromParsingName(reinterpret_cast<const wchar_t *>(native.utf16()),
                                             nullptr, IID_IShellItem,
                                             reinterpret_cast<void **>(&item));
    if (FAILED(hr))
        return QString();
    LPWSTR name = nullptr;
    hr = item->GetDisplayName(SIGDN_NORMALDISPLAY, &name);
    if (FAILED(hr))
        return QString();
    QString result = QString::fromWCharArray(name);
    CoTaskMemFree(name);
    item->Release();
    return result;
#else
    Q_UNUSED(path)
    QTC_CHECK(false);
    return {};
#endif // Q_OS_WIN
}

/*!
    \internal

    Adds a new file to the children of parentNode

    *WARNING* this will change the count of children
*/
FileSystemNode* FileSystemModelPrivate::addNode(FileSystemNode *parentNode, const PathKey &fileName, const QFileInfo& info)
{
    // In the common case, itemLocation == count() so check there first
    FileSystemNode *node = new FileSystemNode(fileName, parentNode);
    if (useFileSystemWatcher())
        node->populate(info);

    // The parentNode is "" so we are listing the drives
    if (HostOsInfo::isWindowsHost() && parentNode->fileName.data.isEmpty())
        node->volumeName = volumeName(fileName.data);
    Q_ASSERT(!parentNode->children.contains(fileName));
    parentNode->children.insert(fileName, node);
    return node;
}

/*!
    \internal

    File at parentNode->children(itemLocation) has been removed, remove from the lists
    and emit signals if necessary

    *WARNING* this will change the count of children and could change visibleChildren
 */
void FileSystemModelPrivate::removeNode(FileSystemNode *parentNode, const PathKey &name)
{
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);

    int vLocation = parentNode->visibleLocation(name);
    if (vLocation >= 0 && !indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                                       translateVisibleLocation(parentNode, vLocation));
    FileSystemNode * node = parentNode->children.take(name);
    delete node;
    // cleanup sort files after removing rather then re-sorting which is O(n)
    if (vLocation >= 0)
        parentNode->visibleChildren.removeAt(vLocation);
    if (vLocation >= 0 && !indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    File at parentNode->children(itemLocation) was not visible before, but now should be
    and emit signals if necessary.

    *WARNING* this will change the visible count
 */
void FileSystemModelPrivate::addVisibleFiles(FileSystemNode *parentNode, const PathKeys &newFiles)
{
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden) {
        q->beginInsertRows(parent, parentNode->visibleChildren.count() , parentNode->visibleChildren.count() + newFiles.count() - 1);
    }

    if (parentNode->dirtyChildrenIndex == -1)
        parentNode->dirtyChildrenIndex = parentNode->visibleChildren.count();

    for (const PathKey &newFile : newFiles) {
        parentNode->visibleChildren.append(newFile);
        FileSystemNode *node = parentNode->children.value(newFile);
        QTC_ASSERT(node, continue);
        node->isVisible = true;
    }
    if (!indexHidden)
      q->endInsertRows();
}

/*!
    \internal

    File was visible before, but now should NOT be

    *WARNING* this will change the visible count
 */
void FileSystemModelPrivate::removeVisibleFile(FileSystemNode *parentNode, int vLocation)
{
    if (vLocation == -1)
        return;
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                                       translateVisibleLocation(parentNode, vLocation));
    parentNode->children.value(parentNode->visibleChildren.at(vLocation))->isVisible = false;
    parentNode->visibleChildren.removeAt(vLocation);
    if (!indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    The thread has received new information about files,
    update and emit dataChanged if it has actually changed.
 */
void FileSystemModelPrivate::_q_fileSystemChanged(const QString &path,
                                                   const QList<QPair<QString, QFileInfo>> &updates)
{
    QTC_CHECK(useFileSystemWatcher());

    PathKeys rowsToUpdate;
    PathKeys newFiles;
    FileSystemNode *parentNode = node(path, false);
    QModelIndex parentIndex = index(parentNode);
    for (const auto &update : updates) {
        PathKey fileName{update.first, parentNode->caseSensitivity()};
        Q_ASSERT(!fileName.data.isEmpty());
        ExtendedInformation info = fileInfoGatherer.getInfo(update.second);
        bool previouslyHere = parentNode->children.contains(fileName);
        if (!previouslyHere) {
            addNode(parentNode, fileName, info.fileInfo());
        }
        FileSystemNode * node = parentNode->children.value(fileName);
        if (node->fileName != fileName)
            continue;
        const bool isCaseSensitive = parentNode->caseSensitive();
        if (isCaseSensitive) {
            Q_ASSERT(node->fileName == fileName);
        } else {
            node->fileName = fileName;
        }

        if (*node != info ) {
            node->populate(info);
            bypassFilters.remove(node);
            // brand new information.
            if (filtersAcceptsNode(node)) {
                if (!node->isVisible) {
                    newFiles.append(fileName);
                } else {
                    rowsToUpdate.append(fileName);
                }
            } else {
                if (node->isVisible) {
                    int visibleLocation = parentNode->visibleLocation(fileName);
                    removeVisibleFile(parentNode, visibleLocation);
                } else {
                    // The file is not visible, don't do anything
                }
            }
        }
    }

    // bundle up all of the changed signals into as few as possible.
    std::sort(rowsToUpdate.begin(), rowsToUpdate.end());
    PathKey min;
    PathKey max;
    for (const PathKey &value : std::as_const(rowsToUpdate)) {
        //##TODO is there a way to bundle signals with QString as the content of the list?
        /*if (min.isEmpty()) {
            min = value;
            if (i != rowsToUpdate.count() - 1)
                continue;
        }
        if (i != rowsToUpdate.count() - 1) {
            if ((value == min + 1 && max.isEmpty()) || value == max + 1) {
                max = value;
                continue;
            }
        }*/
        max = value;
        min = value;
        int visibleMin = parentNode->visibleLocation(min);
        int visibleMax = parentNode->visibleLocation(max);
        if (visibleMin >= 0
            && visibleMin < parentNode->visibleChildren.count()
            && parentNode->visibleChildren.at(visibleMin) == min
            && visibleMax >= 0) {
            QModelIndex bottom = q->index(translateVisibleLocation(parentNode, visibleMin), 0, parentIndex);
            QModelIndex top = q->index(translateVisibleLocation(parentNode, visibleMax), 3, parentIndex);
            emit q->dataChanged(bottom, top);
        }

        /*min = QString();
        max = QString();*/
    }

    if (newFiles.count() > 0) {
        addVisibleFiles(parentNode, newFiles);
    }

    if (newFiles.count() > 0 || (sortColumn != 0 && rowsToUpdate.count() > 0)) {
        forceSort = true;
        delayedSort();
    }
}

void FileSystemModelPrivate::_q_resolvedName(const QString &fileName, const QString &resolvedName)
{
    resolvedSymLinks[fileName] = resolvedName;
}

// Remove file system watchers at/below the index and return a list of previously
// watched files. This should be called prior to operations like rename/remove
// which might fail due to watchers on platforms like Windows. The watchers
// should be restored on failure.
QStringList FileSystemModelPrivate::unwatchPathsAt(const QModelIndex &index)
{
    QTC_CHECK(HostOsInfo::isWindowsHost());
    QTC_CHECK(useFileSystemWatcher());
    const FileSystemNode *indexNode = node(index);
    if (indexNode == nullptr)
        return {};
    const Qt::CaseSensitivity caseSensitivity = indexNode->caseSensitive()
        ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const QString path = indexNode->fileInfo().absoluteFilePath();

    QStringList result;
    const auto filter = [path, caseSensitivity] (const QString &watchedPath)
    {
        const int pathSize = path.size();
        if (pathSize == watchedPath.size()) {
            return path.compare(watchedPath, caseSensitivity) == 0;
        } else if (watchedPath.size() > pathSize) {
            return watchedPath.at(pathSize) == QLatin1Char('/')
                && watchedPath.startsWith(path, caseSensitivity);
        }
        return false;
    };

    const QStringList &watchedFiles = fileInfoGatherer.watchedFiles();
    std::copy_if(watchedFiles.cbegin(), watchedFiles.cend(),
                 std::back_inserter(result), filter);

    const QStringList &watchedDirectories = fileInfoGatherer.watchedDirectories();
    std::copy_if(watchedDirectories.cbegin(), watchedDirectories.cend(),
                 std::back_inserter(result), filter);

    fileInfoGatherer.unwatchPaths(result);
    return result;
}

void FileSystemModelPrivate::init()
{
    delayedSortTimer.setSingleShot(true);

    qRegisterMetaType<QList<QPair<QString, QFileInfo>>>();
    if (useFileSystemWatcher()) {
        QObject::connect(&fileInfoGatherer, &FileInfoGatherer::newListOfFiles,
                   &mySlots, &FileSystemModelSlots::_q_directoryChanged);
        QObject::connect(&fileInfoGatherer, &FileInfoGatherer::updates,
                   &mySlots, &FileSystemModelSlots::_q_fileSystemChanged);
        QObject::connect(&fileInfoGatherer, &FileInfoGatherer::nameResolved,
                   &mySlots, &FileSystemModelSlots::_q_resolvedName);
        q->connect(&fileInfoGatherer, SIGNAL(directoryLoaded(QString)),
                   q, SIGNAL(directoryLoaded(QString)));
    }
    QObject::connect(&delayedSortTimer, &QTimer::timeout,
                     &mySlots, &FileSystemModelSlots::_q_performDelayedSort,
                     Qt::QueuedConnection);
}

/*!
    \internal

    Returns \c false if node doesn't pass the filters otherwise true

    QDir::Modified is not supported
    QDir::Drives is not supported
*/
bool FileSystemModelPrivate::filtersAcceptsNode(const FileSystemNode *node) const
{
    // always accept drives
    if (node->parent == &root || bypassFilters.contains(node))
        return true;

    // If we don't know anything yet don't accept it
    if (!node->hasInformation())
        return false;

    const bool filterPermissions = ((filters & QDir::PermissionMask)
                                   && (filters & QDir::PermissionMask) != QDir::PermissionMask);
    const bool hideDirs          = !(filters & (QDir::Dirs | QDir::AllDirs));
    const bool hideFiles         = !(filters & QDir::Files);
    const bool hideReadable      = !(!filterPermissions || (filters & QDir::Readable));
    const bool hideWritable      = !(!filterPermissions || (filters & QDir::Writable));
    const bool hideExecutable    = !(!filterPermissions || (filters & QDir::Executable));
    const bool hideHidden        = !(filters & QDir::Hidden);
    const bool hideSystem        = !(filters & QDir::System);
    const bool hideSymlinks      = (filters & QDir::NoSymLinks);
    const bool hideDot           = (filters & QDir::NoDot);
    const bool hideDotDot        = (filters & QDir::NoDotDot);

    // Note that we match the behavior of entryList and not QFileInfo on this.
    bool isDot    = (node->fileName.data == ".");
    bool isDotDot = (node->fileName.data == "..");
    if (   (hideHidden && !(isDot || isDotDot) && node->isHidden())
        || (hideSystem && node->isSystem())
        || (hideDirs && node->isDir())
        || (hideFiles && node->isFile())
        || (hideSymlinks && node->isSymLink())
        || (hideReadable && node->isReadable())
        || (hideWritable && node->isWritable())
        || (hideExecutable && node->isExecutable())
        || (hideDot && isDot)
        || (hideDotDot && isDotDot))
        return false;

    return nameFilterDisables || passNameFilters(node);
}

/*
    \internal

    Returns \c true if node passes the name filters and should be visible.
 */

static QRegularExpression QRegularExpression_fromWildcard(QStringView pattern, Qt::CaseSensitivity cs)
{
    auto reOptions = cs == Qt::CaseSensitive ? QRegularExpression::NoPatternOption :
                                             QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern.toString()), reOptions);
}

bool FileSystemModelPrivate::passNameFilters(const FileSystemNode *node) const
{
    if (nameFilters.isEmpty())
        return true;

    // Check the name regularexpression filters
    if (!(node->isDir() && (filters & QDir::AllDirs))) {
        const auto matchesNodeFileName = [node](const QRegularExpression &re)
        {
            return node->fileName.data.contains(re);
        };
        return std::any_of(nameFiltersRegexps.begin(),
                           nameFiltersRegexps.end(),
                           matchesNodeFileName);
    }
    return true;
}

void FileSystemModelPrivate::rebuildNameFilterRegexps()
{
    nameFiltersRegexps.clear();
    nameFiltersRegexps.reserve(nameFilters.size());
    const auto cs = (filters & QDir::CaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const auto convertWildcardToRegexp = [cs](const QString &nameFilter)
    {
        return QRegularExpression_fromWildcard(nameFilter, cs);
    };
    std::transform(nameFilters.constBegin(),
                   nameFilters.constEnd(),
                   std::back_inserter(nameFiltersRegexps),
                   convertWildcardToRegexp);
}

void FileSystemModelSlots::_q_directoryChanged(const QString &directory, const QStringList &list)
{
    owner->_q_directoryChanged(directory, list);
}

void FileSystemModelSlots::_q_performDelayedSort()
{
    owner->_q_performDelayedSort();
}

void FileSystemModelSlots::_q_fileSystemChanged(const QString &path,
                              const QList<QPair<QString, QFileInfo>> &list)
{
    owner->_q_fileSystemChanged(path, list);
}

void FileSystemModelSlots::_q_resolvedName(const QString &fileName, const QString &resolvedName)
{
    owner->_q_resolvedName(fileName, resolvedName);
}

void FileSystemModelSlots::directoryLoaded(const QString &path)
{
    q_owner->directoryLoaded(path);
}

} // Utils

#include "moc_filesystemmodel.cpp"
#include "filesystemmodel.moc"
