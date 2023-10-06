// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QFileDialog>
#include <QObject>
#include <QPair>

namespace Utils { class FilePath; }

namespace Core {

class IDocument;

namespace Internal {
class DocumentManagerPrivate;
class ICorePrivate;
}

class CORE_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    enum ResolveMode {
        ResolveLinks,
        KeepLinks
    };

    using RecentFile = QPair<Utils::FilePath, Utils::Id>;

    static DocumentManager *instance();

    // file pool to monitor
    static void addDocuments(const QList<IDocument *> &documents, bool addWatcher = true);
    static void addDocument(IDocument *document, bool addWatcher = true);
    static bool removeDocument(IDocument *document);
    static QList<IDocument *> modifiedDocuments();

    static void renamedFile(const Utils::FilePath &from, const Utils::FilePath &to);

    static void expectFileChange(const Utils::FilePath &filePath);
    static void unexpectFileChange(const Utils::FilePath &filePath);

    // recent files
    static void addToRecentFiles(const Utils::FilePath &filePath, Utils::Id editorId = {});
    Q_SLOT void clearRecentFiles();
    static QList<RecentFile> recentFiles();

    static void saveSettings();

    // helper functions
    static Utils::FilePath filePathKey(const Utils::FilePath &filePath, ResolveMode resolveMode);

    static bool saveDocument(IDocument *document,
                             const Utils::FilePath &filePath = Utils::FilePath(),
                             bool *isReadOnly = nullptr);

    static QString allDocumentFactoryFiltersString(QString *allFilesFilter);

    static Utils::FilePaths getOpenFileNames(const QString &filters,
                                             const Utils::FilePath &path = {},
                                             QString *selectedFilter = nullptr,
                                             QFileDialog::Options options = {});
    static Utils::FilePath getSaveFileName(const QString &title,
                                           const Utils::FilePath &pathIn,
                                           const QString &filter = {},
                                           QString *selectedFilter = nullptr);
    static Utils::FilePath getSaveFileNameWithExtension(const QString &title,
                                                        const Utils::FilePath &pathIn,
                                                        const QString &filter);
    static Utils::FilePath getSaveAsFileName(const IDocument *document);

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
    static void showFilePropertiesDialog(const Utils::FilePath &filePath);

    static Utils::FilePath fileDialogLastVisitedDirectory();
    static void setFileDialogLastVisitedDirectory(const Utils::FilePath &);

    static Utils::FilePath fileDialogInitialDirectory();

    static Utils::FilePath defaultLocationForNewFiles();
    static void setDefaultLocationForNewFiles(const Utils::FilePath &location);

    static bool useProjectsDirectory();
    static void setUseProjectsDirectory(bool);

    static Utils::FilePath projectsDirectory();
    static void setProjectsDirectory(const Utils::FilePath &directory);

    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    static void notifyFilesChangedInternally(const Utils::FilePaths &filePaths);

    static void setFileDialogFilter(const QString &filter);

    static QString fileDialogFilter(QString *selectedFilter = nullptr);
    static QString allFilesFilterString();

signals:
    /* Used to notify e.g. the code model to update the given files. Does *not*
       lead to any editors to reload or any other editor manager actions. */
    void filesChangedInternally(const Utils::FilePaths &filePaths);
    /// emitted if all documents changed their name e.g. due to the file changing on disk
    void allDocumentsRenamed(const Utils::FilePath &from, const Utils::FilePath &to);
    /// emitted if one document changed its name e.g. due to save as
    void documentRenamed(Core::IDocument *document,
                         const Utils::FilePath &from,
                         const Utils::FilePath &to);
    void projectsDirectoryChanged(const Utils::FilePath &directory);

    void filesChangedExternally(const QSet<Utils::FilePath> &filePaths);

private:
    explicit DocumentManager(QObject *parent);
    ~DocumentManager() override;

    void documentDestroyed(QObject *obj);
    void checkForNewFileName(IDocument *document);
    void checkForReload();
    void changedFile(const QString &file);
    void updateSaveAll();
    static void registerSaveAllAction();

    friend class Internal::DocumentManagerPrivate;
    friend class Internal::ICorePrivate;
};

class CORE_EXPORT FileChangeBlocker
{
public:
    explicit FileChangeBlocker(const Utils::FilePath &filePath);
    ~FileChangeBlocker();
private:
    const Utils::FilePath m_filePath;
    Q_DISABLE_COPY(FileChangeBlocker)
};

} // namespace Core

Q_DECLARE_METATYPE(Core::DocumentManager::RecentFile)
