/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "filesystemwatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>

enum { debug = false };

namespace QmlProjectManager {

int FileSystemWatcher::m_objectCount = 0;
QHash<QString,int> FileSystemWatcher::m_fileCount;
QHash<QString,int> FileSystemWatcher::m_directoryCount;
QFileSystemWatcher *FileSystemWatcher::m_watcher = 0;

FileSystemWatcher::FileSystemWatcher(QObject *parent) :
    QObject(parent)
{
    if (!m_watcher)
        m_watcher = new QFileSystemWatcher();
    ++m_objectCount;
    connect(m_watcher, SIGNAL(fileChanged(QString)),
            this, SLOT(slotFileChanged(QString)));
    connect(m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(slotDirectoryChanged(QString)));
}

FileSystemWatcher::~FileSystemWatcher()
{
    removeFiles(files());
    removeDirectories(directories());

    if (--m_objectCount == 0) {
        delete m_watcher;
        m_watcher = 0;
    }
}

void FileSystemWatcher::addFile(const QString &file)
{
    addFiles(QStringList(file));
}

void FileSystemWatcher::addFiles(const QStringList &files)
{
    QStringList toAdd;

    if (debug)
        qDebug() << Q_FUNC_INFO << files.count();

    foreach (const QString &file, files) {
        if (m_files.contains(file)) {
            qWarning() << "FileSystemWatcher: File" << file << "is already being watched";
            continue;
        }
        m_files.append(file);

        const int count = ++m_fileCount[file];
        Q_ASSERT(count > 0);

        if (count == 1)
            toAdd << file;
    }

    if (!toAdd.isEmpty())
        m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeFile(const QString &file)
{
    removeFiles(QStringList(file));
}

void FileSystemWatcher::removeFiles(const QStringList &files)
{
    QStringList toRemove;

    if (debug)
        qDebug() << Q_FUNC_INFO << files.count();

    foreach (const QString &file, files) {
        if (!m_files.contains(file)) {
            qWarning() << "FileSystemWatcher: File" << file << "is not watched";
            continue;
        }
        m_files.removeOne(file);
        const int count = --m_fileCount[file];
        Q_ASSERT(count >= 0);

        if (!count) {
            toRemove << file;
        }
    }

    if (!toRemove.isEmpty())
        m_watcher->removePaths(toRemove);
}

QStringList FileSystemWatcher::files() const
{
    return m_files;
}

void FileSystemWatcher::addDirectory(const QString &directory)
{
    addDirectories(QStringList(directory));
}

void FileSystemWatcher::addDirectories(const QStringList &directories)
{
    QStringList toAdd;

    if (debug)
        qDebug() << Q_FUNC_INFO << directories.count();

    foreach (const QString &directory, directories) {
        if (m_directories.contains(directory)) {
            qWarning() << "Directory" << directory << "is already being watched";
            continue;
        }
        m_directories.append(directory);

        const int count = ++m_directoryCount[directory];
        Q_ASSERT(count > 0);

        if (count == 1)
            toAdd << directory;
    }

    if (!toAdd.isEmpty())
        m_watcher->addPaths(toAdd);
}

void FileSystemWatcher::removeDirectory(const QString &directory)
{
    removeDirectories(QStringList(directory));
}

void FileSystemWatcher::removeDirectories(const QStringList &directories)
{
    QStringList toRemove;

    if (debug)
        qDebug() << Q_FUNC_INFO << directories.count();

    foreach (const QString &directory, directories) {
        if (!m_directories.contains(directory)) {
            qWarning() << "FileSystemWatcher: Directory" << directory << "is not watched";
            continue;
        }
        m_directories.removeOne(directory);

        const int count = --m_directoryCount[directory];
        Q_ASSERT(count >= 0);

        if (!count) {
            toRemove << directory;
        }
    }
    if (!toRemove.isEmpty())
        m_watcher->removePaths(toRemove);
}

QStringList FileSystemWatcher::directories() const
{
    return m_directories;
}

void FileSystemWatcher::slotFileChanged(const QString &path)
{
    if (m_files.contains(path))
        emit fileChanged(path);
}

void FileSystemWatcher::slotDirectoryChanged(const QString &path)
{
    if (m_directories.contains(path))
        emit directoryChanged(path);
}

} // namespace QmlProjectManager
