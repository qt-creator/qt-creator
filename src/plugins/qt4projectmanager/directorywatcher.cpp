/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
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
