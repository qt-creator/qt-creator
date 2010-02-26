/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QMainWindow;
QT_END_NAMESPACE

namespace Core {

class ICore;
class IContext;
class IFile;

namespace Internal {
struct FileManagerPrivate;
}

class CORE_EXPORT FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QMainWindow *ew);
    virtual ~FileManager();

    // file pool to monitor
    bool addFiles(const QList<IFile *> &files, bool addWatcher = true);
    bool addFile(IFile *file, bool addWatcher = true);
    bool removeFile(IFile *file);
    bool isFileManaged(const QString &fileName) const;
    QList<IFile *> modifiedFiles() const;

    void blockFileChange(IFile *file);
    void unblockFileChange(IFile *file);

    void expectFileChange(const QString &fileName);
    void unexpectFileChange(const QString &fileName);

    // recent files
    void addToRecentFiles(const QString &fileName);
    QStringList recentFiles() const;
    void saveRecentFiles();

    // current file
    void setCurrentFile(const QString &filePath);
    QString currentFile() const;

    // helper methods
    static QString fixFileName(const QString &fileName);

    QStringList getOpenFileNames(const QString &filters,
                                 const QString path = QString(),
                                 QString *selectedFilter = 0);

    QString getSaveFileNameWithExtension(const QString &title, const QString &path,
                                    const QString &fileFilter, const QString &extension);
    QString getSaveAsFileName(IFile *file);

    QList<IFile *> saveModifiedFilesSilently(const QList<IFile *> &files);
    QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                                     bool *cancelled = 0,
                                     const QString &message = QString(),
                                     const QString &alwaysSaveMessage = QString::null,
                                     bool *alwaysSave = 0);


    QString fileDialogLastVisitedDirectory() const;
    void setFileDialogLastVisitedDirectory(const QString &);

    QString fileDialogInitialDirectory() const;

    bool useProjectsDirectory() const;
    void setUseProjectsDirectory(bool);

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &);

public slots:
    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    void notifyFilesChangedInternally(const QStringList &files);

signals:
    void currentFileChanged(const QString &filePath);
    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    void filesChangedInternally(const QStringList &files);

private slots:
    void fileDestroyed(QObject *obj);
    void checkForNewFileName();
    void checkForReload();
    void changedFile(const QString &file);
    void mainWindowActivated();
    void syncWithEditor(Core::IContext *context);

private:
    void addFileInfo(IFile *file, bool addWatcher = true);
    void removeFileInfo(IFile *file);
    void removeFileInfo(const QString &fileName, IFile *file);

    void addWatch(const QString &filename);
    void removeWatch(const QString &filename);
    void updateFileInfo(IFile *file);
    void updateExpectedState(const QString &fileName);

    QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                               bool *cancelled, bool silently,
                               const QString &message,
                               const QString &alwaysSaveMessage = QString::null,
                               bool *alwaysSave = 0);

    Internal::FileManagerPrivate *d;
};

/*! The FileChangeBlocker blocks all change notifications to all IFile * that
    match the given filename. And unblocks in the destructor.

    To also reload the IFile in the destructor class set modifiedReload to true

  */
class CORE_EXPORT FileChangeBlocker
{
public:
    FileChangeBlocker(const QString &fileName);
    ~FileChangeBlocker();
private:
    QString m_fileName;
    Q_DISABLE_COPY(FileChangeBlocker)
};

} // namespace Core

#endif // FILEMANAGER_H
