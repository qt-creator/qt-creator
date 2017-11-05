/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/id.h>

#include <QObject>
#include <QPair>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace Utils { class FileName; }

namespace Core {

class IDocument;

namespace Internal {
class DocumentManagerPrivate;
class MainWindow;
}

class CORE_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    enum ResolveMode {
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
    static QString cleanAbsoluteFilePath(const QString &filePath, ResolveMode resolveMode);
    static QString filePathKey(const QString &filePath, ResolveMode resolveMode);

    static bool saveDocument(IDocument *document,
                             const QString &fileName = QString(),
                             bool *isReadOnly = nullptr);

    static QString allDocumentFactoryFiltersString(QString *allFilesFilter);

    static QStringList getOpenFileNames(const QString &filters,
                                        const QString &path = QString(),
                                        QString *selectedFilter = nullptr);
    static QString getSaveFileName(const QString &title,
                                   const QString &pathIn,
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr);
    static QString getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                         const QString &filter);
    static QString getSaveAsFileName(const IDocument *document);

    static bool saveAllModifiedDocumentsSilently(bool *canceled = nullptr,
                                                 QList<IDocument *> *failedToClose = nullptr);
    static bool saveModifiedDocumentsSilently(const QList<IDocument *> &documents,
                                              bool *canceled = nullptr,
                                              QList<IDocument *> *failedToClose = nullptr);
    static bool saveModifiedDocumentSilently(IDocument *document,
                                             bool *canceled = nullptr,
                                             QList<IDocument *> *failedToClose = nullptr);

    static bool saveAllModifiedDocuments(const QString &message = QString(),
                                         bool *canceled = nullptr,
                                         const QString &alwaysSaveMessage = QString(),
                                         bool *alwaysSave = nullptr,
                                         QList<IDocument *> *failedToClose = nullptr);
    static bool saveModifiedDocuments(const QList<IDocument *> &documents,
                                      const QString &message = QString(),
                                      bool *canceled = nullptr,
                                      const QString &alwaysSaveMessage = QString(),
                                      bool *alwaysSave = nullptr,
                                      QList<IDocument *> *failedToClose = nullptr);
    static bool saveModifiedDocument(IDocument *document,
                                     const QString &message = QString(),
                                     bool *canceled = nullptr,
                                     const QString &alwaysSaveMessage = QString(),
                                     bool *alwaysSave = nullptr,
                                     QList<IDocument *> *failedToClose = nullptr);

    static QString fileDialogLastVisitedDirectory();
    static void setFileDialogLastVisitedDirectory(const QString &);

    static QString fileDialogInitialDirectory();

    static QString defaultLocationForNewFiles();
    static void setDefaultLocationForNewFiles(const QString &location);

    static bool useProjectsDirectory();
    static void setUseProjectsDirectory(bool);

    static Utils::FileName projectsDirectory();
    static void setProjectsDirectory(const Utils::FileName &directory);

    static QString buildDirectory();
    static void setBuildDirectory(const QString &directory);

    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    static void notifyFilesChangedInternally(const QStringList &files);

signals:
    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    void filesChangedInternally(const QStringList &files);
    /// emitted if all documents changed their name e.g. due to the file changing on disk
    void allDocumentsRenamed(const QString &from, const QString &to);
    /// emitted if one document changed its name e.g. due to save as
    void documentRenamed(Core::IDocument *document, const QString &from, const QString &to);
    void projectsDirectoryChanged(const Utils::FileName &directory);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private:
    explicit DocumentManager(QObject *parent);
    ~DocumentManager();

    void documentDestroyed(QObject *obj);
    void checkForNewFileName();
    void checkForReload();
    void changedFile(const QString &file);
    void filePathChanged(const Utils::FileName &oldName, const Utils::FileName &newName);

    friend class Core::Internal::MainWindow;
    friend class Core::Internal::DocumentManagerPrivate;
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
