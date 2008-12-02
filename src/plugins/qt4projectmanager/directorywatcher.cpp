/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "directorywatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>

enum { debugWatcher = 0 };

namespace Qt4ProjectManager {
namespace Internal {

int DirectoryWatcher::m_objectCount = 0;
QHash<QString,int> DirectoryWatcher::m_directoryCount;
QFileSystemWatcher *DirectoryWatcher::m_watcher = 0;

/*
 \class DirectoryWatcher

   A wrapper for QFileSystemWatcher that collects
   consecutive changes to a registered directory and emits directoryChanged() and fileChanged().

   Note that files added are only monitored if the parent directory is added, too.

   All instances of DirectoryWatcher share one QFileSystemWatcher object.
   That's because every QFileSystemWatcher object consumes a file descriptor,
   even if no files are watched.
*/
DirectoryWatcher::DirectoryWatcher(QObject *parent) :
    QObject(parent),
    m_timer(0)
{
    if (!m_watcher)
        m_watcher = new QFileSystemWatcher();
    ++m_objectCount;
    connect(m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(slotDirectoryChanged(QString)));
}

DirectoryWatcher::~DirectoryWatcher()
{
    foreach (const QString &dir, m_directories)
        removeDirectory(dir);
    if (--m_objectCount == 0) {
        delete m_watcher;
        m_watcher = 0;
    }
}

QStringList DirectoryWatcher::directories() const
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << m_directories;
    return m_directories;
}

void DirectoryWatcher::addDirectory(const QString &dir)
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << dir;
    if (m_directories.contains(dir))
        return;
    m_directories += dir;
    if (m_directoryCount[dir] == 0)
        m_watcher->addPath(dir);
    m_directoryCount[dir] += 1;
}

void DirectoryWatcher::removeDirectory(const QString &dir)
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << dir;
    m_directories.removeOne(dir);
    m_directoryCount[dir] -= 1;
    if (m_directoryCount[dir] == 0)
      m_watcher->removePath(dir);
}

QStringList DirectoryWatcher::files() const
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << m_files.keys();
    return m_files.keys();
}

void DirectoryWatcher::addFile(const QString &filePath)
{
    addFiles(QStringList() << filePath);
}

void DirectoryWatcher::addFiles(const QStringList &filePaths)
{
    foreach (const QString filePath, filePaths) {
        QFileInfo file(filePath);
        m_files.insert(file.absoluteFilePath(),file.lastModified());
    }
}

void DirectoryWatcher::removeFile(const QString &filePath)
{
    m_files.remove(filePath);
}

void DirectoryWatcher::slotDirectoryChanged(const QString &path)
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << path;
    if (!m_directories.contains(path)
        || m_pendingDirectories.contains(path))
        return;

    if (!m_timer) {
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(500); // delay for 0.5 sec
        connect(m_timer, SIGNAL(timeout()), this, SLOT(slotDelayedDirectoriesChanged()));
    }
    if (!m_timer->isActive())
        m_timer->start();
    m_pendingDirectories.push_back(path);
}

void DirectoryWatcher::slotDelayedDirectoriesChanged()
{
    if (debugWatcher)
        qDebug() << Q_FUNC_INFO << " emitting " << m_pendingDirectories;
    const QStringList::const_iterator cend = m_pendingDirectories.constEnd();
    for (QStringList::const_iterator it = m_pendingDirectories.constBegin(); it != cend; ++it) {
        const QString dir = *it;
        if (!QFileInfo(dir).exists())
            removeDirectory(*it);
        emit directoryChanged(*it);
        updateFileList(*it);
    }
    m_pendingDirectories.clear();
}

void DirectoryWatcher::updateFileList(const QString &dir)
{
    const QStringList monitoredFiles = m_files.keys();
    QStringList removedFiles = monitoredFiles;
    if (QFileInfo(dir).exists()) {
        // Compare directory contents and emit signals
        QFileInfoList entryInfoList
                = QDir(dir).entryInfoList(QDir::Files|QDir::CaseSensitive);
        // Loop over directory creating the new map of file->time, removing
        // the existing entries from the old map
        const QFileInfoList::const_iterator cend = entryInfoList.constEnd();
        for (QFileInfoList::const_iterator filIt = entryInfoList.constBegin();
            filIt != cend; ++filIt) {
            const QString path = filIt->absoluteFilePath();
            FileModificationTimeMap::iterator mapIt = m_files.find(path);
            if (mapIt != m_files.end()) {
                const QDateTime lastModified = filIt->lastModified();
                if (lastModified > mapIt.value()) {
                    if (debugWatcher)
                        qDebug() << Q_FUNC_INFO << "emitting file changed" << path;
                    emit fileChanged(path);
                    m_files[path] = lastModified;
                }
                removedFiles.removeOne(path);
            }
        }
    }

    if (!removedFiles.isEmpty()) {
        foreach (const QString &file, removedFiles)
            removeFile(file);
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
