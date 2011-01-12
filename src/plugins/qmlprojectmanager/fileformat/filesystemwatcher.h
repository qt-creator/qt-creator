/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FSWATCHER_H
#define FSWATCHER_H

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QTimer;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace QmlProjectManager {

class FileSystemWatcher : public QObject
{
    Q_DISABLE_COPY(FileSystemWatcher)
    Q_OBJECT
public:
    explicit FileSystemWatcher(QObject *parent = 0);
    virtual ~FileSystemWatcher();

    void addFile(const QString &file);
    void addFiles(const QStringList &files);

    void removeFile(const QString &file);
    void removeFiles(const QStringList &files);

    QStringList files() const;

    void addDirectory(const QString &file);
    void addDirectories(const QStringList &files);

    void removeDirectory(const QString &file);
    void removeDirectories(const QStringList &files);

    QStringList directories() const;

private slots:
    void slotFileChanged(const QString &path);
    void slotDirectoryChanged(const QString &path);

signals:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    QStringList m_files;
    QStringList m_directories;

    static int m_objectCount;
    static QHash<QString, int> m_fileCount;
    static QHash<QString, int> m_directoryCount;
    static QFileSystemWatcher *m_watcher;
};

} // namespace QmlProjectManager

#endif // FSWATCHER_H
