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

namespace Utils { class FileName; }

namespace Core {

class IContext;
class IDocument;

namespace Internal { class MainWindow; }

class CORE_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    enum FixMode {
        ResolveLinks,
        KeepLinks
    };

    typedef QPair<QString, Id> RecentFile;

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
    static void addToRecentFiles(const QString &fileName, Id editorId = Id());
    Q_SLOT void clearRecentFiles();
    static QList<RecentFile> recentFiles();

    static void saveSettings();

    // helper functions
    static QString fixFileName(const QString &fileName, FixMode fixmode);

    static bool saveDocument(IDocument *document, const QString &fileName = QString(), bool *isReadOnly = 0);

    static QStringList getOpenFileNames(const QString &filters,
                                        const QString &path = QString(),
                                        QString *selectedFilter = 0);
    static QString getSaveFileName(const QString &title, const QString &pathIn,
                            const QString &filter = QString(), QString *selectedFilter = 0);
    static QString getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                         const QString &filter);
    static QString getSaveAsFileName(const IDocument *document, const QString &filter = QString(),
                              QString *selectedFilter = 0);

    static bool saveAllModifiedDocumentsSilently(bool *canceled = 0,
                                                 QList<IDocument *> *failedToClose = 0);
    static bool saveModifiedDocumentsSilently(const QList<IDocument *> &documents, bool *canceled = 0,
                                              QList<IDocument *> *failedToClose = 0);
    static bool saveModifiedDocumentSilently(IDocument *document, bool *canceled = 0,
                                             QList<IDocument *> *failedToClose = 0);

    static bool saveAllModifiedDocuments(const QString &message = QString(), bool *canceled = 0,
                                         const QString &alwaysSaveMessage = QString(),
                                         bool *alwaysSave = 0,
                                         QList<IDocument *> *failedToClose = 0);
    static bool saveModifiedDocuments(const QList<IDocument *> &documents,
                                      const QString &message = QString(), bool *canceled = 0,
                                      const QString &alwaysSaveMessage = QString(),
                                      bool *alwaysSave = 0,
                                      QList<IDocument *> *failedToClose = 0);
    static bool saveModifiedDocument(IDocument *document,
                                     const QString &message = QString(), bool *canceled = 0,
                                     const QString &alwaysSaveMessage = QString(),
                                     bool *alwaysSave = 0,
                                     QList<IDocument *> *failedToClose = 0);

    static QString fileDialogLastVisitedDirectory();
    static void setFileDialogLastVisitedDirectory(const QString &);

    static QString fileDialogInitialDirectory();

    static QString defaultLocationForNewFiles();
    static void setDefaultLocationForNewFiles(const QString &location);

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

public slots:
    static void executeOpenWithMenuAction(QAction *action);

signals:
    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    void filesChangedInternally(const QStringList &files);
    /// emitted if all documents changed their name e.g. due to the file changing on disk
    void allDocumentsRenamed(const QString &from, const QString &to);
    /// emitted if one document changed its name e.g. due to save as
    void documentRenamed(Core::IDocument *document, const QString &from, const QString &to);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void documentDestroyed(QObject *obj);
    void checkForNewFileName();
    void checkForReload();
    void changedFile(const QString &file);

private:
    explicit DocumentManager(QObject *parent);
    ~DocumentManager();

    void filePathChanged(const Utils::FileName &oldName, const Utils::FileName &newName);

    friend class Core::Internal::MainWindow;
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
