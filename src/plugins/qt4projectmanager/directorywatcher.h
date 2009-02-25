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

class FileWatcher : public QObject
{
    Q_DISABLE_COPY(FileWatcher)
    Q_OBJECT
public:
    explicit FileWatcher(QObject *parent = 0);
    virtual ~FileWatcher();

    QStringList files();
    void addFile(const QString &file);
    void removeFile(const QString &file);
signals:
    void fileChanged(const QString &path);
    void debugOutout(const QString &path);

private slots:
    void slotFileChanged(const QString&);

private:
    static int m_objectCount;
    static QHash<QString, int> m_fileCount;
    static QFileSystemWatcher *m_watcher;
    QStringList m_files;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // DIRECTORYWATCHER_H
