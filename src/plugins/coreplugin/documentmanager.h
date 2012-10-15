/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DOCUMENTMANAGER_H
#define DOCUMENTMANAGER_H

#include <coreplugin/id.h>

#include <QObject>
#include <QPair>

QT_BEGIN_NAMESPACE
class QStringList;
class QAction;
class QMainWindow;
class QMenu;
QT_END_NAMESPACE

namespace Core {

class IContext;
class IDocument;

class CORE_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    enum FixMode {
        ResolveLinks,
        KeepLinks
    };

    typedef QPair<QString, Id> RecentFile;

    explicit DocumentManager(QMainWindow *ew);
    virtual ~DocumentManager();

    static DocumentManager *instance();

    // file pool to monitor
    static void addDocuments(const QList<IDocument *> &documents, bool addWatcher = true);
    static void addDocument(IDocument *document, bool addWatcher = true);
    static bool removeDocument(IDocument *document);
    static QList<IDocument *> modifiedDocuments();

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

    static bool saveDocument(IDocument *document, const QString &fileName = QString(), bool *isReadOnly = 0);

    static QStringList getOpenFileNames(const QString &filters,
                                 const QString path = QString(),
                                 QString *selectedFilter = 0);
    static QString getSaveFileName(const QString &title, const QString &pathIn,
                            const QString &filter = QString(), QString *selectedFilter = 0);
    static QString getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                         const QString &filter);
    static QString getSaveAsFileName(const IDocument *document, const QString &filter = QString(),
                              QString *selectedFilter = 0);

    static QList<IDocument *> saveModifiedDocumentsSilently(const QList<IDocument *> &documents, bool *cancelled = 0);
    static QList<IDocument *> saveModifiedDocuments(const QList<IDocument *> &documents,
                                     bool *cancelled = 0,
                                     const QString &message = QString(),
                                     const QString &alwaysSaveMessage = QString(),
                                     bool *alwaysSave = 0);

    static QString fileDialogLastVisitedDirectory();
    static void setFileDialogLastVisitedDirectory(const QString &);

    static QString fileDialogInitialDirectory();

    static bool useProjectsDirectory();
    static void setUseProjectsDirectory(bool);

    static QString projectsDirectory();
    static void setProjectsDirectory(const QString &);

    static QString buildDirectory();
    static void setBuildDirectory(const QString &directory);

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
    /// emitted if all documents changed their name e.g. due to the file changing on disk
    void allDocumentsRenamed(const QString &from, const QString &to);
    /// emitted if one document changed its name e.g. due to save as
    void documentRenamed(Core::IDocument *document, const QString &from, const QString &to);

private slots:
    void documentDestroyed(QObject *obj);
    void fileNameChanged(const QString &oldName, const QString &newName);
    void checkForNewFileName();
    void checkForReload();
    void changedFile(const QString &file);
    void mainWindowActivated();
    void syncWithEditor(Core::IContext *context);
};

/*! The FileChangeBlocker blocks all change notifications to all IDocument * that
    match the given filename. And unblocks in the destructor.

    To also reload the IDocument in the destructor class set modifiedReload to true

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

Q_DECLARE_METATYPE(Core::DocumentManager::RecentFile)

#endif // DOCUMENTMANAGER_H
