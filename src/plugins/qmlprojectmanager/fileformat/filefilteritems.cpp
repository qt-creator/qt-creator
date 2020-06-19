/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

FileFilterBaseItem::FileFilterBaseItem(QObject *parent) :
        QmlProjectContentItem(parent)
{
    m_updateFileListTimer.setSingleShot(true);
    m_updateFileListTimer.setInterval(50);

    connect(&m_updateFileListTimer, &QTimer::timeout, this, &FileFilterBaseItem::updateFileListNow);
}

Utils::FileSystemWatcher *FileFilterBaseItem::dirWatcher()
{
    if (!m_dirWatcher) {
        m_dirWatcher = new Utils::FileSystemWatcher(1, this); // Separate id, might exceed OS limits.
        m_dirWatcher->setObjectName(QLatin1String("FileFilterBaseItemWatcher"));
        connect(m_dirWatcher, &Utils::FileSystemWatcher::directoryChanged,
                this, &FileFilterBaseItem::updateFileList);
    }
    return m_dirWatcher;
}

QStringList FileFilterBaseItem::watchedDirectories() const
{
    return m_dirWatcher ? m_dirWatcher->directories() : QStringList();
}

QString FileFilterBaseItem::directory() const
{
    return m_rootDir;
}

void FileFilterBaseItem::setDirectory(const QString &dirPath)
{
    if (m_rootDir == dirPath)
        return;
    m_rootDir = dirPath;
    emit directoryChanged();

    updateFileList();
}

void FileFilterBaseItem::setDefaultDirectory(const QString &dirPath)
{
    if (m_defaultDir == dirPath)
        return;
    m_defaultDir = dirPath;

    updateFileListNow();
}

QString FileFilterBaseItem::filter() const
{
    return m_filter;
}

void FileFilterBaseItem::setFilter(const QString &filter)
{
    if (filter == m_filter)
        return;
    m_filter = filter;

    m_regExpList.clear();
    m_fileSuffixes.clear();

    for (const QString &pattern : filter.split(QLatin1Char(';'))) {
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

bool FileFilterBaseItem::recursive() const
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

void FileFilterBaseItem::setRecursive(bool recurse)
{
    bool oldRecursive = recursive();

    if (recurse)
        m_recurse = Recurse;
    else
            m_recurse = DoNotRecurse;

    if (recurse != oldRecursive)
        updateFileList();
}

QStringList FileFilterBaseItem::pathsProperty() const
{
    return m_explicitFiles;
}

void FileFilterBaseItem::setPathsProperty(const QStringList &path)
{
    m_explicitFiles = path;
    updateFileList();
}

QStringList FileFilterBaseItem::files() const
{
    return Utils::toList(m_files);
}

/**
  Check whether filter matches a file path - regardless whether the file already exists or not.

  @param filePath: absolute file path
  */
bool FileFilterBaseItem::matchesFile(const QString &filePath) const
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

QString FileFilterBaseItem::absolutePath(const QString &path) const
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QDir(absoluteDir()).absoluteFilePath(path);
}

QString FileFilterBaseItem::absoluteDir() const
{
    QString absoluteDir;
    if (QFileInfo(m_rootDir).isAbsolute())
        absoluteDir = m_rootDir;
    else if (!m_defaultDir.isEmpty())
        absoluteDir = m_defaultDir + QLatin1Char('/') + m_rootDir;

    return QDir::cleanPath(absoluteDir);
}

void FileFilterBaseItem::updateFileList()
{
    if (!m_updateFileListTimer.isActive())
        m_updateFileListTimer.start();
}

void FileFilterBaseItem::updateFileListNow()
{
    if (m_updateFileListTimer.isActive())
        m_updateFileListTimer.stop();

    const QString projectDir = absoluteDir();
    if (projectDir.isEmpty())
        return;

    QSet<QString> dirsToBeWatched;
    QSet<QString> newFiles;
    for (const QString &explicitPath : qAsConst(m_explicitFiles))
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

bool FileFilterBaseItem::fileMatches(const QString &fileName) const
{
    for (const QString &suffix : qAsConst(m_fileSuffixes)) {
        if (fileName.endsWith(suffix, Qt::CaseInsensitive))
            return true;
    }

    for (const QRegularExpression &filter : qAsConst(m_regExpList)) {
        if (filter.match(fileName).hasMatch())
            return true;
    }

    return false;
}

QSet<QString> FileFilterBaseItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
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

ImageFileFilterItem::ImageFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    QString filter;
    // supported image formats according to
    const QList<QByteArray> extensions = QImageReader::supportedImageFormats();
    for (const QByteArray &extension : extensions)
        filter.append(QString::fromLatin1("*.%1;").arg(QString::fromLatin1(extension)));
    setFilter(filter);
}

FileFilterItem::FileFilterItem(const QString &fileFilter, QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(fileFilter);
}

} // namespace QmlProjectManager

