/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "directorywatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>

enum { debugWatcher = 0 };

namespace Qt4ProjectManager {
namespace Internal {

int FileWatcher::m_objectCount = 0;
QHash<QString,int> FileWatcher::m_fileCount;
QFileSystemWatcher *FileWatcher::m_watcher = 0;

FileWatcher::FileWatcher(QObject *parent) :
    QObject(parent)
{
    if (!m_watcher)
        m_watcher = new QFileSystemWatcher();
    ++m_objectCount;
    connect(m_watcher, SIGNAL(fileChanged(QString)),
            this, SLOT(slotFileChanged(QString)));
}

FileWatcher::~FileWatcher()
{
    foreach (const QString &file, m_files)
        removeFile(file);
    if (--m_objectCount == 0) {
        delete m_watcher;
        m_watcher = 0;
    }
}

void FileWatcher::slotFileChanged(const QString &file)
{
    if (m_files.contains(file))
        emit fileChanged(file);
}

QStringList FileWatcher::files()
{
    return m_files;
}

void FileWatcher::addFile(const QString &file)
{
    if (m_files.contains(file))
        return;
    m_files += file;
    if (m_fileCount[file] == 0)
        m_watcher->addPath(file);
    m_fileCount[file] += 1;
}

void FileWatcher::removeFile(const QString &file)
{
    m_files.removeOne(file);
    m_fileCount[file] -= 1;
    if (m_fileCount[file] == 0)
      m_watcher->removePath(file);
}

} // namespace Internal
} // namespace Qt4ProjectManager
