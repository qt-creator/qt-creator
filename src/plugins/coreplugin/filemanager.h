/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <coreplugin/id.h>

#include <QObject>
#include <QStringList>
#include <QPair>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QAction;
class QMainWindow;
class QMenu;
QT_END_NAMESPACE

namespace Core {

class IContext;
class IFile;
class IVersionControl;

class CORE_EXPORT FileManager : public QObject
{
    Q_OBJECT
public:
    enum FixMode {
        ResolveLinks,
        KeepLinks
    };

    typedef QPair<QString, Id> RecentFile;

    explicit FileManager(QMainWindow *ew);
    virtual ~FileManager();

    static FileManager *instance();

    // file pool to monitor
    static void addFiles(const QList<IFile *> &files, bool addWatcher = true);
    static void addFile(IFile *file, bool addWatcher = true);
    static bool removeFile(IFile *file);
    static QList<IFile *> modifiedFiles();

    static void renamedFile(const QString &from, const QString &to);

    static void expectFileChange(const QString &fileName);
    static void unexpectFileChange(const QString &fileName);

    // recent files
    static void addToRecentFiles(const QString &fileName, const Id &editorId = Id());
    Q_SLOT void clearRecentFiles();
    static QList<RecentFile> recentFiles();

    static void saveSettings();

    // current file
    static void setCurrentFile(const QString &filePath);
    static QString currentFile();

    // helper methods
    static QString fixFileName(const QString &fileName, FixMode fixmode);

    static bool saveFile(IFile *file, const QString &fileName = QString(), bool *isReadOnly = 0);

    static QStringList getOpenFileNames(const QString &filters,
                                 const QString path = QString(),
                                 QString *selectedFilter = 0);
    static QString getSaveFileName(const QString &title, const QString &pathIn,
                            const QString &filter = QString(), QString *selectedFilter = 0);
    static QString getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                         const QString &filter);
    static QString getSaveAsFileName(IFile *file, const QString &filter = QString(),
                              QString *selectedFilter = 0);

    static QList<IFile *> saveModifiedFilesSilently(const QList<IFile *> &files, bool *cancelled = 0);
    static QList<IFile *> saveModifiedFiles(const QList<IFile *> &files,
                                     bool *cancelled = 0,
                                     const QString &message = QString(),
                                     const QString &alwaysSaveMessage = QString(),
                                     bool *alwaysSave = 0);


    // Helper to display a message dialog when encountering a read-only
    // file, prompting the user about how to make it writeable.
    enum ReadOnlyAction { RO_Cancel, RO_OpenVCS, RO_MakeWriteable, RO_SaveAs };
    static ReadOnlyAction promptReadOnlyFile(const QString &fileName,
                                             const IVersionControl *versionControl,
                                             QWidget *parent,
                                             bool displaySaveAsButton = false);

    static QString fileDialogLastVisitedDirectory();
    static void setFileDialogLastVisitedDirectory(const QString &);

    static QString fileDialogInitialDirectory();

    static bool useProjectsDirectory();
    static void setUseProjectsDirectory(bool);

    static QString projectsDirectory();
    static void setProjectsDirectory(const QString &);

    static void populateOpenWithMenu(QMenu *menu, const QString &fileName);

    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    static void notifyFilesChangedInternally(const QStringList &files);

    static void executeOpenWithMenuAction(QAction *action);

public slots:
    void slotExecuteOpenWithMenuAction(QAction *action);

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
