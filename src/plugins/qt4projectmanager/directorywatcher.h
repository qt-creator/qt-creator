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

#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QTimer;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class DirectoryWatcher : public QObject
{
    Q_DISABLE_COPY(DirectoryWatcher)
    Q_OBJECT
public:
    explicit DirectoryWatcher(QObject *parent = 0);
    virtual ~DirectoryWatcher();

    QStringList directories() const;
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);

    QStringList files() const;
    void addFile(const QString &filePath);
    void addFiles(const QStringList &filePaths);
    void removeFile(const QString &filePath);

signals:
    void directoryChanged(const QString &path);
    void fileChanged(const QString &path);

private slots:
    void slotDirectoryChanged(const QString &);
    void slotDelayedDirectoriesChanged();

private:
    void updateFileList(const QString &dir);

    static int m_objectCount;
    static QHash<QString,int> m_directoryCount;
    static QFileSystemWatcher *m_watcher;

    QTimer *m_timer;
    QStringList m_directories;
    QStringList m_pendingDirectories;

    typedef QHash<QString, QDateTime> FileModificationTimeMap;
    FileModificationTimeMap m_files;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // DIRECTORYWATCHER_H
