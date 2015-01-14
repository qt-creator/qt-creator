/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILESYSTEMWATCHER_H
#define FILESYSTEMWATCHER_H

#include "utils_global.h"

#include <QObject>

namespace Utils {
class FileSystemWatcherPrivate;

// Documentation inside.
class QTCREATOR_UTILS_EXPORT FileSystemWatcher : public QObject
{
    Q_OBJECT

public:
    enum WatchMode
    {
        WatchModifiedDate,
        WatchAllChanges
    };

    explicit FileSystemWatcher(QObject *parent = 0);
    explicit FileSystemWatcher(int id, QObject *parent = 0);
    virtual ~FileSystemWatcher();

    void addFile(const QString &file, WatchMode wm);
    void addFiles(const QStringList &files, WatchMode wm);

    void removeFile(const QString &file);
    void removeFiles(const QStringList &files);

    bool watchesFile(const QString &file) const;
    QStringList files() const;

    void addDirectory(const QString &file, WatchMode wm);
    void addDirectories(const QStringList &files, WatchMode wm);

    void removeDirectory(const QString &file);
    void removeDirectories(const QStringList &files);

    bool watchesDirectory(const QString &file) const;
    QStringList directories() const;

private slots:
    void slotFileChanged(const QString &path);
    void slotDirectoryChanged(const QString &path);

signals:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    void init();

    FileSystemWatcherPrivate *d;
};

} // namespace Utils

#endif // FILESYSTEMWATCHER_H
