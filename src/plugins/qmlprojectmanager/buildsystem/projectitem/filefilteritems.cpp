// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filefilteritems.h"

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QImageReader>
#include <QRegularExpression>

namespace QmlProjectManager {

FileFilterItem::FileFilterItem(){
    initTimer();
}

FileFilterItem::FileFilterItem(const QString &directory, const QStringList &filters)
{
    setDirectory(directory);
    setFilters(filters);
    initTimer();
}

void FileFilterItem::initTimer(){
    m_updateFileListTimer.setSingleShot(true);
    m_updateFileListTimer.setInterval(50);

    connect(&m_updateFileListTimer, &QTimer::timeout, this, &FileFilterItem::updateFileListNow);
}

Utils::FileSystemWatcher *FileFilterItem::dirWatcher()
{
    if (!m_dirWatcher) {
        m_dirWatcher = new Utils::FileSystemWatcher(1, this); // Separate id, might exceed OS limits.
        m_dirWatcher->setObjectName(QLatin1String("FileFilterBaseItemWatcher"));
        connect(m_dirWatcher, &Utils::FileSystemWatcher::directoryChanged,
                this, &FileFilterItem::updateFileList);
    }
    return m_dirWatcher;
}

QStringList FileFilterItem::watchedDirectories() const
{
    return m_dirWatcher ? m_dirWatcher->directories() : QStringList();
}

QString FileFilterItem::directory() const
{
    return m_rootDir;
}

void FileFilterItem::setDirectory(const QString &dirPath)
{
    if (m_rootDir == dirPath)
        return;
    m_rootDir = dirPath;
    emit directoryChanged();

    updateFileList();
}

void FileFilterItem::setDefaultDirectory(const QString &dirPath)
{
    if (m_defaultDir == dirPath)
        return;
    m_defaultDir = dirPath;

    updateFileListNow();
}

QStringList FileFilterItem::filters() const
{
    return m_filter;
}

void FileFilterItem::setFilters(const QStringList &filter)
{
    if (filter == m_filter)
        return;
    m_filter = filter;

    m_regExpList.clear();
    m_fileSuffixes.clear();

    for (const QString &pattern : filter) {
        if (pattern.isEmpty())
            continue;
        // decide if it's a canonical pattern like *.x
        if (pattern.startsWith(QLatin1String("*."))) {
            const QString suffix = pattern.right(pattern.size() - 1);
            if (!suffix.contains(QLatin1Char('*'))
                    && !suffix.contains(QLatin1Char('?'))
                    && !suffix.contains(QLatin1Char('['))) {
                m_fileSuffixes << suffix;
                continue;
            }
        }
        m_regExpList << QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern));
    }

    updateFileList();
}

bool FileFilterItem::recursive() const
{
    bool recursive;
    if (m_recurse == Recurse) {
        recursive = true;
    } else if (m_recurse == DoNotRecurse) {
        recursive = false;
    } else {  // RecurseDefault
        if (m_explicitFiles.isEmpty())
            recursive = true;
        else
            recursive = false;
    }
    return recursive;
}

void FileFilterItem::setRecursive(bool recurse)
{
    bool oldRecursive = recursive();

    if (recurse)
        m_recurse = Recurse;
    else
            m_recurse = DoNotRecurse;

    if (recurse != oldRecursive)
        updateFileList();
}

QStringList FileFilterItem::pathsProperty() const
{
    return m_explicitFiles;
}

void FileFilterItem::setPathsProperty(const QStringList &path)
{
    m_explicitFiles = path;
    updateFileList();
}

QStringList FileFilterItem::files() const
{
    return Utils::toList(m_files);
}

/**
  Check whether filter matches a file path - regardless whether the file already exists or not.

  @param filePath: absolute file path
  */
bool FileFilterItem::matchesFile(const QString &filePath) const
{
    for (const QString &explicitFile : m_explicitFiles) {
        if (absolutePath(explicitFile) == filePath)
            return true;
    }

    const QString &fileName = Utils::FilePath::fromString(filePath).fileName();

    if (!fileMatches(fileName))
        return false;

    const QDir fileDir = QFileInfo(filePath).absoluteDir();
    for (const QString &watchedDirectory : watchedDirectories()) {
        if (QDir(watchedDirectory) == fileDir)
            return true;
    }

    return false;
}

QString FileFilterItem::absolutePath(const QString &path) const
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QDir(absoluteDir()).absoluteFilePath(path);
}

QString FileFilterItem::absoluteDir() const
{
    QString absoluteDir;
    if (QFileInfo(m_rootDir).isAbsolute())
        absoluteDir = m_rootDir;
    else if (!m_defaultDir.isEmpty())
        absoluteDir = m_defaultDir + QLatin1Char('/') + m_rootDir;

    return QDir::cleanPath(absoluteDir);
}

void FileFilterItem::updateFileList()
{
#ifndef TESTS_ENABLED_QMLPROJECTITEM
    if (!m_updateFileListTimer.isActive())
        m_updateFileListTimer.start();
#endif
}

void FileFilterItem::updateFileListNow()
{
    if (m_updateFileListTimer.isActive())
        m_updateFileListTimer.stop();

    const QString projectDir = absoluteDir();
    if (projectDir.isEmpty())
        return;

    QSet<QString> dirsToBeWatched;
    QSet<QString> newFiles;
    for (const QString &explicitPath : std::as_const(m_explicitFiles))
        newFiles << absolutePath(explicitPath);

    if ((!m_fileSuffixes.isEmpty() || !m_regExpList.isEmpty()) && m_explicitFiles.isEmpty())
        newFiles += filesInSubTree(QDir(m_defaultDir), QDir(projectDir), &dirsToBeWatched);

    if (newFiles != m_files) {
        QSet<QString> addedFiles = newFiles;
        QSet<QString> removedFiles = m_files;
        QSet<QString> unchanged = newFiles;
        unchanged.intersect(m_files);
        addedFiles.subtract(unchanged);
        removedFiles.subtract(unchanged);

        m_files = newFiles;
        emit filesChanged(addedFiles, removedFiles);
    }

    // update watched directories
    const QSet<QString> oldDirs = Utils::toSet(watchedDirectories());
    const QSet<QString> unwatchDirs = oldDirs - dirsToBeWatched;
    const QSet<QString> watchDirs = dirsToBeWatched - oldDirs;

    if (!unwatchDirs.isEmpty()) {
        QTC_ASSERT(m_dirWatcher, return);
        m_dirWatcher->removeDirectories(Utils::toList(unwatchDirs));
    }
    if (!watchDirs.isEmpty())
        dirWatcher()->addDirectories(Utils::toList(watchDirs), Utils::FileSystemWatcher::WatchAllChanges);
}

bool FileFilterItem::fileMatches(const QString &fileName) const
{
    for (const QString &suffix : std::as_const(m_fileSuffixes)) {
        if (fileName.endsWith(suffix, Qt::CaseInsensitive))
            return true;
    }

    for (const QRegularExpression &filter : std::as_const(m_regExpList)) {
        if (filter.match(fileName).hasMatch())
            return true;
    }

    return false;
}

QSet<QString> FileFilterItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
{
    QSet<QString> fileSet;

    if (parsedDirs)
        parsedDirs->insert(dir.absolutePath());

    for (const QFileInfo &file : dir.entryInfoList(QDir::Files)) {
        const QString fileName = file.fileName();

        if (fileMatches(fileName))
            fileSet.insert(file.absoluteFilePath());
    }

    if (recursive()) {
        for (const QFileInfo &subDir : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            fileSet += filesInSubTree(rootDir, QDir(subDir.absoluteFilePath()), parsedDirs);
        }
    }
    return fileSet;
}

} // namespace QmlProjectManager

