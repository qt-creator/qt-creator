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

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Core {

class ICore;
class IContext;
class IFile;

namespace Internal {
class MainWindow;
}

class CORE_EXPORT FileManager : public QObject
{
    Q_OBJECT

    struct FileInfo
    {
        QString fileName;
        QDateTime modified;
        QFile::Permissions permissions;
    };

public:
    explicit FileManager(Internal::MainWindow *ew);

    // file pool to monitor
    bool addFiles(const QList<IFile *> &files);
    bool addFile(IFile *file);
    bool removeFile(IFile *file);
    bool isFileManaged(const QString &fileName) const;
    QList<IFile *> managedFiles(const QString &fileName) const;
    QList<IFile *> modifiedFiles() const;

    void blockFileChange(IFile *file);
    void unblockFileChange(IFile *file);

    // recent files
    void addToRecentFiles(const QString &fileName);
    QStringList recentFiles() const;
    void saveRecentFiles();

    // current file
    void setCurrentFile(const QString &filePath);
    QString currentFile() const;

    // helper methods
    static QString fixFileName(const QString &fileName);

    QString getSaveFileNameWithExtension(const QString &title, const QString &path,
                                    const QString &fileFilter, const QString &extension);
    QString getSaveAsFileName(IFile *file);

    QList<IFile *> saveModifiedFilesSilently(const QList<IFile *> &files);
    QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                                     bool *cancelled = 0,
                                     const QString &message = QString());

signals:
    void currentFileChanged(const QString &filePath);

private slots:
    void fileDestroyed(QObject *obj);
    void checkForNewFileName();
    void checkForReload();
    void changedFile(const QString &file);
    void mainWindowActivated();
    void syncWithEditor(Core::IContext *context);

private:
    void addWatch(const QString &filename);
    void removeWatch(const QString &filename);
    void updateFileInfo(IFile *file);

    QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                               bool *cancelled, bool silently, const QString &message);

    QMap<IFile*, FileInfo> m_managedFiles;

    QStringList m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QString m_currentFile;

    Internal::MainWindow *m_mainWindow;
    QFileSystemWatcher *m_fileWatcher;
    QList<QPointer<IFile> > m_changedFiles;
    bool m_blockActivated;
};

} // namespace Core

#endif // FILEMANAGER_H
