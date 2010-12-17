/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "filewatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>

enum { debugWatcher = 0 };

using namespace ProjectExplorer;

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
    QStringList keys = m_files.keys();
    foreach(const QString &file, keys)
        removeFile(file);

    if (--m_objectCount == 0) {
        delete m_watcher;
        m_watcher = 0;
    }
}

void FileWatcher::slotFileChanged(const QString &file)
{
    if (m_files.contains(file)) {
        QDateTime lastModified = QFileInfo(file).lastModified();
        if (lastModified != m_files.value(file)) {
            m_files[file] = lastModified;
            emit fileChanged(file);
        }
    }
}

void FileWatcher::addFile(const QString &file)
{
    const int count = ++m_fileCount[file];
    Q_ASSERT(count > 0);

    m_files.insert(file, QFileInfo(file).lastModified());

    if (count == 1)
        m_watcher->addPath(file);
}

void FileWatcher::removeFile(const QString &file)
{
    if (!m_files.contains(file))
        return;

    const int count = --m_fileCount[file];
    Q_ASSERT(count >= 0);

    m_files.remove(file);

    if (!count) {
        m_watcher->removePath(file);
        m_fileCount.remove(file);
    }
}
