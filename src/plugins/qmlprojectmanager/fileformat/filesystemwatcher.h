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
