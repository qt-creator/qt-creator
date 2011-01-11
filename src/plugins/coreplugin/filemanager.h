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

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QMainWindow;
QT_END_NAMESPACE

namespace Core {

class ICore;
class IContext;
class IFile;
class IVersionControl;

namespace Internal {
struct FileManagerPrivate;
}

class CORE_EXPORT FileManager : public QObject
{
    Q_OBJECT
public:
    enum FixMode {
        ResolveLinks,
        KeepLinks
    };

    typedef QPair<QString, QString> RecentFile;

    explicit FileManager(QMainWindow *ew);
    virtual ~FileManager();

    static FileManager *instance();

    // file pool to monitor
    void addFiles(const QList<IFile *> &files, bool addWatcher = true);
    void addFile(IFile *file, bool addWatcher = true);
    void removeFile(IFile *file);
    QList<IFile *> modifiedFiles() const;

    void renamedFile(const QString &from, const QString &to);

    void blockFileChange(IFile *file);
    void unblockFileChange(IFile *file);

    void expectFileChange(const QString &fileName);
    void unexpectFileChange(const QString &fileName);

    // recent files
    void addToRecentFiles(const QString &fileName, const QString &editorId = QString());
    QList<RecentFile> recentFiles() const;
    void saveSettings();

    // current file
    void setCurrentFile(const QString &filePath);
    QString currentFile() const;

    // helper methods
    static QString fixFileName(const QString &fileName, FixMode fixmode);

    QStringList getOpenFileNames(const QString &filters,
                                 const QString path = QString(),
                                 QString *selectedFilter = 0);
    QString getSaveFileName(const QString &title, const QString &pathIn,
                            const QString &filter = QString(), QString *selectedFilter = 0);
    QString getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                         const QString &filter);
    QString getSaveAsFileName(IFile *file, const QString &filter = QString(),
                              QString *selectedFilter = 0);

    QList<IFile *> saveModifiedFilesSilently(const QList<IFile *> &files);
    QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                                     bool *cancelled = 0,
                                     const QString &message = QString(),
                                     const QString &alwaysSaveMessage = QString::null,
                                     bool *alwaysSave = 0);


    // Helper to display a message dialog when encountering a read-only
    // file, prompting the user about how to make it writeable.
    enum ReadOnlyAction { RO_Cancel, RO_OpenVCS, RO_MakeWriteable, RO_SaveAs };
    static ReadOnlyAction promptReadOnlyFile(const QString &fileName,
                                             const IVersionControl *versionControl,
                                             QWidget *parent,
                                             bool displaySaveAsButton = false);

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
    void readSettings();
    void dump();
    void addFileInfo(IFile *file);
    void addFileInfo(const QString &fileName, IFile *file, bool isLink);
    void removeFileInfo(IFile *file);

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
    explicit FileChangeBlocker(const QString &fileName);
    ~FileChangeBlocker();
private:
    const QString m_fileName;
    Q_DISABLE_COPY(FileChangeBlocker)
};

} // namespace Core

Q_DECLARE_METATYPE(Core::FileManager::RecentFile)

#endif // FILEMANAGER_H
