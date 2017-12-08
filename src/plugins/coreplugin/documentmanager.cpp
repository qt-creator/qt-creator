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

#include "documentmanager.h"

#include "icore.h"
#include "idocument.h"
#include "idocumentfactory.h"
#include "coreconstants.h"

#include <coreplugin/diffservice.h>
#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/dialogs/saveitemsdialog.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/editormanager/editorview.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/reloadpromptutils.h>

#include <QStringList>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QSettings>
#include <QTimer>
#include <QAction>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

Q_LOGGING_CATEGORY(log, "qtc.core.documentmanager")

/*!
  \class Core::DocumentManager
  \mainclass
  \inheaderfile documentmanager.h
  \brief The DocumentManager class manages a set of IDocument objects.

  The DocumentManager service monitors a set of IDocument objects. Plugins
  should register files they work with at the service. The files the IDocument
  objects point to will be monitored at filesystem level. If a file changes,
  the status of the IDocument object
  will be adjusted accordingly. Furthermore, on application exit the user will
  be asked to save all modified files.

  Different IDocument objects in the set can point to the same file in the
  filesystem. The monitoring for an IDocument can be blocked by
  \c blockFileChange(), and enabled again by \c unblockFileChange().

  The functions \c expectFileChange() and \c unexpectFileChange() mark a file change
  as expected. On expected file changes all IDocument objects are notified to reload
  themselves.

  The DocumentManager service also provides two convenience functions for saving
  files: \c saveModifiedFiles() and \c saveModifiedFilesSilently(). Both take a list
  of FileInterfaces as an argument, and return the list of files which were
  _not_ saved.

  The service also manages the list of recent files to be shown to the user.

  \sa addToRecentFiles(), recentFiles()
 */

static const char settingsGroupC[] = "RecentFiles";
static const char filesKeyC[] = "Files";
static const char editorsKeyC[] = "EditorIds";

static const char directoryGroupC[] = "Directories";
static const char projectDirectoryKeyC[] = "Projects";
static const char useProjectDirectoryKeyC[] = "UseProjectsDirectory";
static const char buildDirectoryKeyC[] = "BuildDirectory.Template";

using namespace Utils;

namespace Core {

static void readSettings();

static bool saveModifiedFilesHelper(const QList<IDocument *> &documents,
                                    const QString &message,
                                    bool *cancelled, bool silently,
                                    const QString &alwaysSaveMessage,
                                    bool *alwaysSave, QList<IDocument *> *failedToSave);

namespace Internal {

struct FileStateItem
{
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileState
{
    QString watchedFilePath;
    QMap<IDocument *, FileStateItem> lastUpdatedState;
    FileStateItem expected;
};


class DocumentManagerPrivate : public QObject
{
    Q_OBJECT
public:
    DocumentManagerPrivate();
    QFileSystemWatcher *fileWatcher();
    QFileSystemWatcher *linkWatcher();

    void checkOnNextFocusChange();
    void onApplicationFocusChange();

    QMap<QString, FileState> m_states; // filePathKey -> FileState
    QSet<QString> m_changedFiles; // watched file paths collected from file watcher notifications
    QList<IDocument *> m_documentsWithoutWatch;
    QMap<IDocument *, QStringList> m_documentsWithWatch; // document -> list of filePathKeys
    QSet<QString> m_expectedFileNames; // set of file names without normalization

    QList<DocumentManager::RecentFile> m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QFileSystemWatcher *m_fileWatcher = nullptr; // Delayed creation.
    QFileSystemWatcher *m_linkWatcher = nullptr; // Delayed creation (only UNIX/if a link is seen).
    bool m_blockActivated = false;
    bool m_checkOnFocusChange = false;
    QString m_lastVisitedDirectory = QDir::currentPath();
    QString m_defaultLocationForNewFiles;
    FileName m_projectsDirectory;
    bool m_useProjectsDirectory = true;
    QString m_buildDirectory;
    // When we are calling into an IDocument
    // we don't want to receive a changed()
    // signal
    // That makes the code easier
    IDocument *m_blockedIDocument = nullptr;
};

static DocumentManager *m_instance;
static DocumentManagerPrivate *d;

QFileSystemWatcher *DocumentManagerPrivate::fileWatcher()
{
    if (!m_fileWatcher) {
        m_fileWatcher= new QFileSystemWatcher(m_instance);
        QObject::connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                         m_instance, &DocumentManager::changedFile);
    }
    return m_fileWatcher;
}

QFileSystemWatcher *DocumentManagerPrivate::linkWatcher()
{
    if (HostOsInfo::isAnyUnixHost()) {
        if (!m_linkWatcher) {
            m_linkWatcher = new QFileSystemWatcher(m_instance);
            m_linkWatcher->setObjectName(QLatin1String("_qt_autotest_force_engine_poller"));
            QObject::connect(m_linkWatcher, &QFileSystemWatcher::fileChanged,
                             m_instance, &DocumentManager::changedFile);
        }
        return m_linkWatcher;
    }

    return fileWatcher();
}

void DocumentManagerPrivate::checkOnNextFocusChange()
{
    m_checkOnFocusChange = true;
}

void DocumentManagerPrivate::onApplicationFocusChange()
{
    if (!m_checkOnFocusChange)
        return;
    m_checkOnFocusChange = false;
    m_instance->checkForReload();
}

DocumentManagerPrivate::DocumentManagerPrivate()
{
    connect(qApp, &QApplication::focusChanged, this, &DocumentManagerPrivate::onApplicationFocusChange);
}

} // namespace Internal
} // namespace Core

namespace Core {

using namespace Internal;

DocumentManager::DocumentManager(QObject *parent)
  : QObject(parent)
{
    d = new DocumentManagerPrivate;
    m_instance = this;
    qApp->installEventFilter(this);

    readSettings();

    if (d->m_useProjectsDirectory)
        setFileDialogLastVisitedDirectory(d->m_projectsDirectory.toString());
}

DocumentManager::~DocumentManager()
{
    delete d;
}

DocumentManager *DocumentManager::instance()
{
    return m_instance;
}

/* only called from addFileInfo(IDocument *) */
static void addFileInfo(IDocument *document, const QString &filePath,
                        const QString &filePathKey, bool isLink)
{
    FileStateItem state;
    if (!filePath.isEmpty()) {
        qCDebug(log) << "adding document for" << filePath << "(" << filePathKey << ")";
        const QFileInfo fi(filePath);
        state.modified = fi.lastModified();
        state.permissions = fi.permissions();
        // Add state if we don't have already
        if (!d->m_states.contains(filePathKey)) {
            FileState state;
            state.watchedFilePath = filePath;
            d->m_states.insert(filePathKey, state);
        }
        // Add or update watcher on file path
        // This is also used to update the watcher in case of saved (==replaced) files or
        // update link targets, even if there are multiple documents registered for it
        const QString watchedFilePath = d->m_states.value(filePathKey).watchedFilePath;
        qCDebug(log) << "adding (" << (isLink ? "link" : "full") << ") watch for"
                     << watchedFilePath;
        QFileSystemWatcher *watcher = nullptr;
        if (isLink)
            watcher = d->linkWatcher();
        else
            watcher = d->fileWatcher();
        watcher->addPath(watchedFilePath);

        d->m_states[filePathKey].lastUpdatedState.insert(document, state);
    }
    d->m_documentsWithWatch[document].append(filePathKey); // inserts a new QStringList if not already there
}

/* Adds the IDocument's file and possibly it's final link target to both m_states
   (if it's file name is not empty), and the m_filesWithWatch list,
   and adds a file watcher for each if not already done.
   (The added file names are guaranteed to be absolute and cleaned.) */
static void addFileInfo(IDocument *document)
{
    const QString documentFilePath = document->filePath().toString();
    const QString filePath = DocumentManager::cleanAbsoluteFilePath(
                documentFilePath, DocumentManager::KeepLinks);
    const QString filePathKey = DocumentManager::filePathKey(
                documentFilePath, DocumentManager::KeepLinks);
    const QString resolvedFilePath = DocumentManager::cleanAbsoluteFilePath(
                documentFilePath, DocumentManager::ResolveLinks);
    const QString resolvedFilePathKey = DocumentManager::filePathKey(
                documentFilePath, DocumentManager::ResolveLinks);
    const bool isLink = filePath != resolvedFilePath;
    addFileInfo(document, filePath, filePathKey, isLink);
    if (isLink)
        addFileInfo(document, resolvedFilePath, resolvedFilePathKey, false);
}

/*!
    Adds a list of IDocument's to the collection. If \a addWatcher is true (the default),
    the files are added to a file system watcher that notifies the file manager
    about file changes.
*/
void DocumentManager::addDocuments(const QList<IDocument *> &documents, bool addWatcher)
{
    if (!addWatcher) {
        // We keep those in a separate list

        foreach (IDocument *document, documents) {
            if (document && !d->m_documentsWithoutWatch.contains(document)) {
                connect(document, &QObject::destroyed,
                        m_instance, &DocumentManager::documentDestroyed);
                connect(document, &IDocument::filePathChanged,
                        m_instance, &DocumentManager::filePathChanged);
                d->m_documentsWithoutWatch.append(document);
            }
        }
        return;
    }

    foreach (IDocument *document, documents) {
        if (document && !d->m_documentsWithWatch.contains(document)) {
            connect(document, &IDocument::changed, m_instance, &DocumentManager::checkForNewFileName);
            connect(document, &QObject::destroyed, m_instance, &DocumentManager::documentDestroyed);
            connect(document, &IDocument::filePathChanged,
                    m_instance, &DocumentManager::filePathChanged);
            addFileInfo(document);
        }
    }
}


/* Removes all occurrences of the IDocument from m_filesWithWatch and m_states.
   If that results in a file no longer being referenced by any IDocument, this
   also removes the file watcher.
*/
static void removeFileInfo(IDocument *document)
{
    if (!d->m_documentsWithWatch.contains(document))
        return;
    foreach (const QString &fileName, d->m_documentsWithWatch.value(document)) {
        if (!d->m_states.contains(fileName))
            continue;
        qCDebug(log) << "removing document (" << fileName << ")";
        d->m_states[fileName].lastUpdatedState.remove(document);
        if (d->m_states.value(fileName).lastUpdatedState.isEmpty()) {
            const QString &watchedFilePath = d->m_states.value(fileName).watchedFilePath;
            if (d->m_fileWatcher && d->m_fileWatcher->files().contains(watchedFilePath)) {
                qCDebug(log) << "removing watch for" << watchedFilePath;
                d->m_fileWatcher->removePath(watchedFilePath);
            }
            if (d->m_linkWatcher && d->m_linkWatcher->files().contains(watchedFilePath)) {
                qCDebug(log) << "removing watch for" << watchedFilePath;
                d->m_linkWatcher->removePath(watchedFilePath);
            }
            d->m_states.remove(fileName);
        }
    }
    d->m_documentsWithWatch.remove(document);
}

/// Dumps the state of the file manager's map
/// For debugging purposes
/*
static void dump()
{
    qDebug() << "======== dumping state map";
    QMap<QString, FileState>::const_iterator it, end;
    it = d->m_states.constBegin();
    end = d->m_states.constEnd();
    for (; it != end; ++it) {
        qDebug() << it.key();
        qDebug() << "   expected:" << it.value().expected.modified;

        QMap<IDocument *, FileStateItem>::const_iterator jt, jend;
        jt = it.value().lastUpdatedState.constBegin();
        jend = it.value().lastUpdatedState.constEnd();
        for (; jt != jend; ++jt) {
            qDebug() << "  " << jt.key()->fileName() << jt.value().modified;
        }
    }
    qDebug() << "------- dumping files with watch list";
    foreach (IDocument *key, d->m_filesWithWatch.keys()) {
        qDebug() << key->fileName() << d->m_filesWithWatch.value(key);
    }
    qDebug() << "------- dumping watch list";
    if (d->m_fileWatcher)
        qDebug() << d->m_fileWatcher->files();
    qDebug() << "------- dumping link watch list";
    if (d->m_linkWatcher)
        qDebug() << d->m_linkWatcher->files();
}
*/

/*!
    Tells the file manager that a file has been renamed on disk from within \QC.

    Needs to be called right after the actual renaming on disk (that is, before
    the file system
    watcher can report the event during the next event loop run). \a from needs to be an absolute file path.
    This will notify all IDocument objects pointing to that file of the rename
    by calling \c IDocument::rename(), and update the cached time and permission
    information to avoid annoying the user with "file has been removed"
    popups.
*/
void DocumentManager::renamedFile(const QString &from, const QString &to)
{
    const QString &fromKey = filePathKey(from, KeepLinks);

    // gather the list of IDocuments
    QList<IDocument *> documentsToRename;
    QMapIterator<IDocument *, QStringList> it(d->m_documentsWithWatch);
    while (it.hasNext()) {
        it.next();
        if (it.value().contains(fromKey))
            documentsToRename.append(it.key());
    }

    // rename the IDocuments
    foreach (IDocument *document, documentsToRename) {
        d->m_blockedIDocument = document;
        removeFileInfo(document);
        document->setFilePath(FileName::fromString(to));
        addFileInfo(document);
        d->m_blockedIDocument = nullptr;
    }
    emit m_instance->allDocumentsRenamed(from, to);
}

void DocumentManager::filePathChanged(const FileName &oldName, const FileName &newName)
{
    IDocument *doc = qobject_cast<IDocument *>(sender());
    QTC_ASSERT(doc, return);
    if (doc == d->m_blockedIDocument)
        return;
    emit m_instance->documentRenamed(doc, oldName.toString(), newName.toString());
}

/*!
    Adds an IDocument object to the collection. If \a addWatcher is \c true
    (the default),
    the file is added to a file system watcher that notifies the file manager
    about file changes.
*/
void DocumentManager::addDocument(IDocument *document, bool addWatcher)
{
    addDocuments(QList<IDocument *>() << document, addWatcher);
}

void DocumentManager::documentDestroyed(QObject *obj)
{
    IDocument *document = static_cast<IDocument*>(obj);
    // Check the special unwatched first:
    if (!d->m_documentsWithoutWatch.removeOne(document))
        removeFileInfo(document);
}

/*!
    Removes an IDocument object from the collection.

    Returns \c true if the file specified by \a document had the \a addWatcher
    argument to \a addDocument() set.
*/
bool DocumentManager::removeDocument(IDocument *document)
{
    QTC_ASSERT(document, return false);

    bool addWatcher = false;
    // Special casing unwatched files
    if (!d->m_documentsWithoutWatch.removeOne(document)) {
        addWatcher = true;
        removeFileInfo(document);
        disconnect(document, &IDocument::changed, m_instance, &DocumentManager::checkForNewFileName);
    }
    disconnect(document, &QObject::destroyed, m_instance, &DocumentManager::documentDestroyed);
    return addWatcher;
}

/* Slot reacting on IDocument::changed. We need to check if the signal was sent
   because the file was saved under different name. */
void DocumentManager::checkForNewFileName()
{
    IDocument *document = qobject_cast<IDocument *>(sender());
    // We modified the IDocument
    // Trust the other code to also update the m_states map
    if (document == d->m_blockedIDocument)
        return;
    QTC_ASSERT(document, return);
    QTC_ASSERT(d->m_documentsWithWatch.contains(document), return);

    // Maybe the name has changed or file has been deleted and created again ...
    // This also updates the state to the on disk state
    removeFileInfo(document);
    addFileInfo(document);
}

/*!
    Returns a guaranteed cleaned absolute file path for \a filePath in portable form.
    Resolves symlinks if \a resolveMode is ResolveLinks.
*/
QString DocumentManager::cleanAbsoluteFilePath(const QString &filePath, ResolveMode resolveMode)
{
    QFileInfo fi(QDir::fromNativeSeparators(filePath));
    if (fi.exists() && resolveMode == ResolveLinks) {
        // if the filePath is no link, we want this method to return the same for both ResolveModes
        // so wrap with absoluteFilePath because that forces drive letters upper case
        return QFileInfo(fi.canonicalFilePath()).absoluteFilePath();
    }
    return QDir::cleanPath(fi.absoluteFilePath());
}

/*!
    Returns a representation of \a filePath that can be used as a key for maps.
    (A cleaned absolute file path in portable form, that is all lowercase
    if the file system is case insensitive (in the host OS settings).)
    Resolves symlinks if \a resolveMode is ResolveLinks.
*/
QString DocumentManager::filePathKey(const QString &filePath, ResolveMode resolveMode)
{
    QString s = cleanAbsoluteFilePath(filePath, resolveMode);
    if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        s = s.toLower();
    return s;
}

/*!
    Returns the list of IDocuments that have been modified.
*/
QList<IDocument *> DocumentManager::modifiedDocuments()
{
    QList<IDocument *> modified;

    const auto docEnd = d->m_documentsWithWatch.keyEnd();
    for (auto docIt = d->m_documentsWithWatch.keyBegin(); docIt != docEnd; ++docIt) {
        IDocument *document = *docIt;
        if (document->isModified())
            modified << document;
    }

    foreach (IDocument *document, d->m_documentsWithoutWatch) {
        if (document->isModified())
            modified << document;
    }

    return modified;
}

/*!
    Any subsequent change to \a fileName is treated as an expected file change.

    \see DocumentManager::unexpectFileChange(const QString &fileName)
*/
void DocumentManager::expectFileChange(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    d->m_expectedFileNames.insert(fileName);
}

/* only called from unblock and unexpect file change functions */
static void updateExpectedState(const QString &filePathKey)
{
    if (filePathKey.isEmpty())
        return;
    if (d->m_states.contains(filePathKey)) {
        QFileInfo fi(d->m_states.value(filePathKey).watchedFilePath);
        d->m_states[filePathKey].expected.modified = fi.lastModified();
        d->m_states[filePathKey].expected.permissions = fi.permissions();
    }
}

/*!
    Any changes to \a fileName are unexpected again.

    \see DocumentManager::expectFileChange(const QString &fileName)
*/
void DocumentManager::unexpectFileChange(const QString &fileName)
{
    // We are updating the expected time of the file
    // And in changedFile we'll check if the modification time
    // is the same as the saved one here
    // If so then it's a expected change

    if (fileName.isEmpty())
        return;
    d->m_expectedFileNames.remove(fileName);
    const QString cleanAbsFilePath = cleanAbsoluteFilePath(fileName, KeepLinks);
    updateExpectedState(filePathKey(fileName, KeepLinks));
    const QString resolvedCleanAbsFilePath = cleanAbsoluteFilePath(fileName, ResolveLinks);
    if (cleanAbsFilePath != resolvedCleanAbsFilePath)
        updateExpectedState(filePathKey(fileName, ResolveLinks));
}

static bool saveModifiedFilesHelper(const QList<IDocument *> &documents,
                                    const QString &message, bool *cancelled, bool silently,
                                    const QString &alwaysSaveMessage, bool *alwaysSave,
                                    QList<IDocument *> *failedToSave)
{
    if (cancelled)
        (*cancelled) = false;

    QList<IDocument *> notSaved;
    QMap<IDocument *, QString> modifiedDocumentsMap;
    QList<IDocument *> modifiedDocuments;

    foreach (IDocument *document, documents) {
        if (document && document->isModified()) {
            QString name = document->filePath().toString();
            if (name.isEmpty())
                name = document->fallbackSaveAsFileName();

            // There can be several IDocuments pointing to the same file
            // Prefer one that is not readonly
            // (even though it *should* not happen that the IDocuments are inconsistent with readonly)
            if (!modifiedDocumentsMap.key(name, nullptr) || !document->isFileReadOnly())
                modifiedDocumentsMap.insert(document, name);
        }
    }
    modifiedDocuments = modifiedDocumentsMap.keys();
    if (!modifiedDocuments.isEmpty()) {
        QList<IDocument *> documentsToSave;
        if (silently) {
            documentsToSave = modifiedDocuments;
        } else {
            SaveItemsDialog dia(ICore::dialogParent(), modifiedDocuments);
            if (!message.isEmpty())
                dia.setMessage(message);
            if (!alwaysSaveMessage.isNull())
                dia.setAlwaysSaveMessage(alwaysSaveMessage);
            if (dia.exec() != QDialog::Accepted) {
                if (cancelled)
                    (*cancelled) = true;
                if (alwaysSave)
                    (*alwaysSave) = dia.alwaysSaveChecked();
                if (failedToSave)
                    (*failedToSave) = modifiedDocuments;
                const QStringList filesToDiff = dia.filesToDiff();
                if (!filesToDiff.isEmpty()) {
                    if (auto diffService = DiffService::instance())
                        diffService->diffModifiedFiles(filesToDiff);
                }
                return false;
            }
            if (alwaysSave)
                *alwaysSave = dia.alwaysSaveChecked();
            documentsToSave = dia.itemsToSave();
        }
        // Check for files without write permissions.
        QList<IDocument *> roDocuments;
        foreach (IDocument *document, documentsToSave) {
            if (document->isFileReadOnly())
                roDocuments << document;
        }
        if (!roDocuments.isEmpty()) {
            ReadOnlyFilesDialog roDialog(roDocuments, ICore::dialogParent());
            roDialog.setShowFailWarning(true, DocumentManager::tr(
                                            "Could not save the files.",
                                            "error message"));
            if (roDialog.exec() == ReadOnlyFilesDialog::RO_Cancel) {
                if (cancelled)
                    (*cancelled) = true;
                if (failedToSave)
                    (*failedToSave) = modifiedDocuments;
                return false;
            }
        }
        foreach (IDocument *document, documentsToSave) {
            if (!EditorManagerPrivate::saveDocument(document)) {
                if (cancelled)
                    *cancelled = true;
                notSaved.append(document);
            }
        }
    }
    if (failedToSave)
        (*failedToSave) = notSaved;
    return notSaved.isEmpty();
}

bool DocumentManager::saveDocument(IDocument *document, const QString &fileName, bool *isReadOnly)
{
    bool ret = true;
    QString effName = fileName.isEmpty() ? document->filePath().toString() : fileName;
    expectFileChange(effName); // This only matters to other IDocuments which refer to this file
    bool addWatcher = removeDocument(document); // So that our own IDocument gets no notification at all

    QString errorString;
    if (!document->save(&errorString, fileName, false)) {
        if (isReadOnly) {
            QFile ofi(effName);
            // Check whether the existing file is writable
            if (!ofi.open(QIODevice::ReadWrite) && ofi.open(QIODevice::ReadOnly)) {
                *isReadOnly = true;
                goto out;
            }
            *isReadOnly = false;
        }
        QMessageBox::critical(ICore::dialogParent(), tr("File Error"),
                              tr("Error while saving file: %1").arg(errorString));
      out:
        ret = false;
    }

    addDocument(document, addWatcher);
    unexpectFileChange(effName);
    return ret;
}

QString DocumentManager::allDocumentFactoryFiltersString(QString *allFilesFilter = 0)
{
    QSet<QString> uniqueFilters;

    for (IEditorFactory *factory : IEditorFactory::allEditorFactories()) {
        for (const QString &mt : factory->mimeTypes()) {
            const QString filter = mimeTypeForName(mt).filterString();
            if (!filter.isEmpty())
                uniqueFilters.insert(filter);
        }
    }

    for (IDocumentFactory *factory : IDocumentFactory::allDocumentFactories()) {
        for (const QString &mt : factory->mimeTypes()) {
            const QString filter = mimeTypeForName(mt).filterString();
            if (!filter.isEmpty())
                uniqueFilters.insert(filter);
        }
    }

    QStringList filters = uniqueFilters.toList();
    filters.sort();
    const QString allFiles = Utils::allFilesFilterString();
    if (allFilesFilter)
        *allFilesFilter = allFiles;
    filters.prepend(allFiles);
    return filters.join(QLatin1String(";;"));
}

QString DocumentManager::getSaveFileName(const QString &title, const QString &pathIn,
                                     const QString &filter, QString *selectedFilter)
{
    const QString &path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
    QString fileName;
    bool repeat;
    do {
        repeat = false;
        fileName = QFileDialog::getSaveFileName(
            ICore::dialogParent(), title, path, filter, selectedFilter, QFileDialog::DontConfirmOverwrite);
        if (!fileName.isEmpty()) {
            // If the selected filter is All Files (*) we leave the name exactly as the user
            // specified. Otherwise the suffix must be one available in the selected filter. If
            // the name already ends with such suffix nothing needs to be done. But if not, the
            // first one from the filter is appended.
            if (selectedFilter && *selectedFilter != Utils::allFilesFilterString()) {
                // Mime database creates filter strings like this: Anything here (*.foo *.bar)
                QRegExp regExp(QLatin1String(".*\\s+\\((.*)\\)$"));
                const int index = regExp.lastIndexIn(*selectedFilter);
                if (index != -1) {
                    bool suffixOk = false;
                    QString caption = regExp.cap(1);
                    caption.remove(QLatin1Char('*'));
                    const QVector<QStringRef> suffixes = caption.splitRef(QLatin1Char(' '));
                    foreach (const QStringRef &suffix, suffixes)
                        if (fileName.endsWith(suffix)) {
                            suffixOk = true;
                            break;
                        }
                    if (!suffixOk && !suffixes.isEmpty())
                        fileName.append(suffixes.at(0).toString());
                }
            }
            if (QFile::exists(fileName)) {
                if (QMessageBox::warning(ICore::dialogParent(), tr("Overwrite?"),
                    tr("An item named \"%1\" already exists at this location. "
                       "Do you want to overwrite it?").arg(QDir::toNativeSeparators(fileName)),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                    repeat = true;
                }
            }
        }
    } while (repeat);
    if (!fileName.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(fileName).absolutePath());
    return fileName;
}

QString DocumentManager::getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                                  const QString &filter)
{
    QString selected = filter;
    return getSaveFileName(title, pathIn, filter, &selected);
}

/*!
    Asks the user for a new file name (\gui {Save File As}) for \a document.
*/
QString DocumentManager::getSaveAsFileName(const IDocument *document)
{
    QTC_ASSERT(document, return QString());
    const QString filter = allDocumentFactoryFiltersString();
    const QString filePath = document->filePath().toString();
    QString selectedFilter;
    QString fileDialogPath = filePath;
    if (!filePath.isEmpty()) {
        selectedFilter = Utils::mimeTypeForFile(filePath).filterString();
    } else {
        const QString suggestedName = document->fallbackSaveAsFileName();
        if (!suggestedName.isEmpty()) {
            const QList<MimeType> types = Utils::mimeTypesForFileName(suggestedName);
            if (!types.isEmpty())
                selectedFilter = types.first().filterString();
        }
        const QString defaultPath = document->fallbackSaveAsPath();
        if (!defaultPath.isEmpty())
            fileDialogPath = defaultPath + (suggestedName.isEmpty()
                    ? QString()
                    : '/' + suggestedName);
    }
    if (selectedFilter.isEmpty())
        selectedFilter = Utils::mimeTypeForName(document->mimeType()).filterString();

    return getSaveFileName(tr("Save File As"),
                           fileDialogPath,
                           filter,
                           &selectedFilter);
}

/*!
    Silently saves all documents and will return true if all modified documents were saved
    successfully.

    This method will try to avoid showing dialogs to the user, but can do so anyway (e.g. if
    a file is not writeable).

    \a Canceled will be set if the user canceled any of the dialogs that he interacted with.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveAllModifiedDocumentsSilently(bool *canceled,
                                                       QList<IDocument *> *failedToClose)
{
    return saveModifiedDocumentsSilently(modifiedDocuments(), canceled, failedToClose);
}

/*!
    Silently saves \a documents and will return true if all of them were saved successfully.

    This method will try to avoid showing dialogs to the user, but can do so anyway (e.g. if
    a file is not writeable).

    \a Canceled will be set if the user canceled any of the dialogs that he interacted with.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveModifiedDocumentsSilently(const QList<IDocument *> &documents,
                                                    bool *canceled,
                                                    QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(documents,
                                   QString(),
                                   canceled,
                                   true,
                                   QString(),
                                   nullptr,
                                   failedToClose);
}

/*!
    Silently saves a \a document and will return true if it was saved successfully.

    This method will try to avoid showing dialogs to the user, but can do so anyway (e.g. if
    a file is not writeable).

    \a Canceled will be set if the user canceled any of the dialogs that he interacted with.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveModifiedDocumentSilently(IDocument *document, bool *canceled,
                                                   QList<IDocument *> *failedToClose)
{
    return saveModifiedDocumentsSilently(QList<IDocument *>() << document, canceled, failedToClose);
}

/*!
    Presents a dialog with all modified documents to the user and will ask him which of these
    should be saved.

    This method may show additional dialogs to the user, e.g. if a file is not writeable).

    The dialog text can be set using \a message. \a Canceled will be set if the user canceled any
    of the dialogs that he interacted with (the method will also return false in this case).
    The \a alwaysSaveMessage will show an additional checkbox asking in the dialog. The state of
    this checkbox will be written into \a alwaysSave if set.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveAllModifiedDocuments(const QString &message, bool *canceled,
                                               const QString &alwaysSaveMessage, bool *alwaysSave,
                                               QList<IDocument *> *failedToClose)
{
    return saveModifiedDocuments(modifiedDocuments(), message, canceled,
                                 alwaysSaveMessage, alwaysSave, failedToClose);
}

/*!
    Presents a dialog with \a documents to the user and will ask him which of these should be saved.

    This method may show additional dialogs to the user, e.g. if a file is not writeable).

    The dialog text can be set using \a message. \a Canceled will be set if the user canceled any
    of the dialogs that he interacted with (the method will also return false in this case).
    The \a alwaysSaveMessage will show an additional checkbox asking in the dialog. The state of
    this checkbox will be written into \a alwaysSave if set.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveModifiedDocuments(const QList<IDocument *> &documents,
                                            const QString &message, bool *canceled,
                                            const QString &alwaysSaveMessage, bool *alwaysSave,
                                            QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(documents, message, canceled, false,
                                   alwaysSaveMessage, alwaysSave, failedToClose);
}

/*!
    Presents a dialog with the one \a document to the user and will ask him whether he wants it
    saved.

    This method may show additional dialogs to the user, e.g. if the file is not writeable).

    The dialog text can be set using \a message. \a Canceled will be set if the user canceled any
    of the dialogs that he interacted with (the method will also return false in this case).
    The \a alwaysSaveMessage will show an additional checkbox asking in the dialog. The state of
    this checkbox will be written into \a alwaysSave if set.
    \a FailedToClose will contain a list of documents that could not be saved if passed into the
    method.
*/
bool DocumentManager::saveModifiedDocument(IDocument *document, const QString &message, bool *canceled,
                                           const QString &alwaysSaveMessage, bool *alwaysSave,
                                           QList<IDocument *> *failedToClose)
{
    return saveModifiedDocuments(QList<IDocument *>() << document, message, canceled,
                                 alwaysSaveMessage, alwaysSave, failedToClose);
}

/*!
    Asks the user for a set of file names to be opened. The \a filters
    and \a selectedFilter arguments are interpreted like in
    \c QFileDialog::getOpenFileNames(). \a pathIn specifies a path to open the
    dialog in if that is not overridden by the user's policy.
*/

QStringList DocumentManager::getOpenFileNames(const QString &filters,
                                              const QString &pathIn,
                                              QString *selectedFilter)
{
    const QString &path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
    const QStringList files = QFileDialog::getOpenFileNames(ICore::dialogParent(),
                                                      tr("Open File"),
                                                      path, filters,
                                                      selectedFilter);
    if (!files.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(files.front()).absolutePath());
    return files;
}

void DocumentManager::changedFile(const QString &fileName)
{
    const bool wasempty = d->m_changedFiles.isEmpty();

    if (d->m_states.contains(filePathKey(fileName, KeepLinks)))
        d->m_changedFiles.insert(fileName);
    qCDebug(log) << "file change notification for" << fileName;

    if (wasempty && !d->m_changedFiles.isEmpty())
        QTimer::singleShot(200, this, &DocumentManager::checkForReload);
}

void DocumentManager::checkForReload()
{
    if (d->m_changedFiles.isEmpty())
        return;
    if (QApplication::applicationState() != Qt::ApplicationActive)
        return;
    // If d->m_blockActivated is true, then it means that the event processing of either the
    // file modified dialog, or of loading large files, has delivered a file change event from
    // a watcher *and* the timer triggered. We may never end up here in a nested way, so
    // recheck later at the end of the checkForReload function.
    if (d->m_blockActivated)
        return;
    if (QApplication::activeModalWidget()) {
        // We do not want to prompt for modified file if we currently have some modal dialog open.
        // There is no really sensible way to get notified globally if a window closed,
        // so just check on every focus change.
        d->checkOnNextFocusChange();
        return;
    }

    d->m_blockActivated = true;

    IDocument::ReloadSetting defaultBehavior = EditorManager::reloadSetting();
    ReloadPromptAnswer previousReloadAnswer = ReloadCurrent;
    FileDeletedPromptAnswer previousDeletedAnswer = FileDeletedSave;

    QList<IDocument *> documentsToClose;
    QMap<IDocument*, QString> documentsToSave;

    // collect file information
    QMap<QString, FileStateItem> currentStates;
    QMap<QString, IDocument::ChangeType> changeTypes;
    QSet<IDocument *> changedIDocuments;
    foreach (const QString &fileName, d->m_changedFiles) {
        const QString fileKey = filePathKey(fileName, KeepLinks);
        qCDebug(log) << "handling file change for" << fileName << "(" << fileKey << ")";
        IDocument::ChangeType type = IDocument::TypeContents;
        FileStateItem state;
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            qCDebug(log) << "file was removed";
            type = IDocument::TypeRemoved;
        } else {
            state.modified = fi.lastModified();
            state.permissions = fi.permissions();
            qCDebug(log) << "file was modified, time:" << state.modified
                         << "permissions: " << state.permissions;
        }
        currentStates.insert(fileKey, state);
        changeTypes.insert(fileKey, type);
        foreach (IDocument *document, d->m_states.value(fileKey).lastUpdatedState.keys())
            changedIDocuments.insert(document);
    }

    // clean up. do this before we may enter the main loop, otherwise we would
    // lose consecutive notifications.
    d->m_changedFiles.clear();

    // collect information about "expected" file names
    // we can't do the "resolving" already in expectFileChange, because
    // if the resolved names are different when unexpectFileChange is called
    // we would end up with never-unexpected file names
    QSet<QString> expectedFileKeys;
    foreach (const QString &fileName, d->m_expectedFileNames) {
        const QString cleanAbsFilePath = cleanAbsoluteFilePath(fileName, KeepLinks);
        expectedFileKeys.insert(filePathKey(fileName, KeepLinks));
        const QString resolvedCleanAbsFilePath = cleanAbsoluteFilePath(fileName, ResolveLinks);
        if (cleanAbsFilePath != resolvedCleanAbsFilePath)
            expectedFileKeys.insert(filePathKey(fileName, ResolveLinks));
    }

    // handle the IDocuments
    QStringList errorStrings;
    QStringList filesToDiff;
    foreach (IDocument *document, changedIDocuments) {
        IDocument::ChangeTrigger trigger = IDocument::TriggerInternal;
        IDocument::ChangeType type = IDocument::TypePermissions;
        bool changed = false;
        // find out the type & behavior from the two possible files
        // behavior is internal if all changes are expected (and none removed)
        // type is "max" of both types (remove > contents > permissions)
        foreach (const QString &fileKey, d->m_documentsWithWatch.value(document)) {
            // was the file reported?
            if (!currentStates.contains(fileKey))
                continue;

            FileStateItem currentState = currentStates.value(fileKey);
            FileStateItem expectedState = d->m_states.value(fileKey).expected;
            FileStateItem lastState = d->m_states.value(fileKey).lastUpdatedState.value(document);

            // did the file actually change?
            if (lastState.modified == currentState.modified && lastState.permissions == currentState.permissions)
                continue;
            changed = true;

            // was it only a permission change?
            if (lastState.modified == currentState.modified)
                continue;

            // was the change unexpected?
            if ((currentState.modified != expectedState.modified || currentState.permissions != expectedState.permissions)
                    && !expectedFileKeys.contains(fileKey)) {
                trigger = IDocument::TriggerExternal;
            }

            // find out the type
            IDocument::ChangeType fileChange = changeTypes.value(fileKey);
            if (fileChange == IDocument::TypeRemoved)
                type = IDocument::TypeRemoved;
            else if (fileChange == IDocument::TypeContents && type == IDocument::TypePermissions)
                type = IDocument::TypeContents;
        }

        if (!changed) // probably because the change was blocked with (un)blockFileChange
            continue;

        // handle it!
        d->m_blockedIDocument = document;

        // Update file info, also handling if e.g. link target has changed.
        // We need to do that before the file is reloaded, because removing the watcher will
        // loose any pending change events. Loosing change events *before* the file is reloaded
        // doesn't matter, because in that case we then reload the new version of the file already
        // anyhow.
        removeFileInfo(document);
        addFileInfo(document);

        bool success = true;
        QString errorString;
        // we've got some modification
        // check if it's contents or permissions:
        if (type == IDocument::TypePermissions) {
            // Only permission change
            success = document->reload(&errorString, IDocument::FlagReload, IDocument::TypePermissions);
        // now we know it's a content change or file was removed
        } else if (defaultBehavior == IDocument::ReloadUnmodified
                   && type == IDocument::TypeContents && !document->isModified()) {
            // content change, but unmodified (and settings say to reload in this case)
            success = document->reload(&errorString, IDocument::FlagReload, type);
        // file was removed or it's a content change and the default behavior for
        // unmodified files didn't kick in
        } else if (defaultBehavior == IDocument::ReloadUnmodified
                   && type == IDocument::TypeRemoved && !document->isModified()) {
            // file removed, but unmodified files should be reloaded
            // so we close the file
            documentsToClose << document;
        } else if (defaultBehavior == IDocument::IgnoreAll) {
            // content change or removed, but settings say ignore
            success = document->reload(&errorString, IDocument::FlagIgnore, type);
        // either the default behavior is to always ask,
        // or the ReloadUnmodified default behavior didn't kick in,
        // so do whatever the IDocument wants us to do
        } else {
            // check if IDocument wants us to ask
            if (document->reloadBehavior(trigger, type) == IDocument::BehaviorSilent) {
                // content change or removed, IDocument wants silent handling
                if (type == IDocument::TypeRemoved)
                    documentsToClose << document;
                else
                    success = document->reload(&errorString, IDocument::FlagReload, type);
            // IDocument wants us to ask
            } else if (type == IDocument::TypeContents) {
                // content change, IDocument wants to ask user
                if (previousReloadAnswer == ReloadNone || previousReloadAnswer == ReloadNoneAndDiff) {
                    // answer already given, ignore
                    success = document->reload(&errorString, IDocument::FlagIgnore, IDocument::TypeContents);
                } else if (previousReloadAnswer == ReloadAll) {
                    // answer already given, reload
                    success = document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents);
                } else {
                    // Ask about content change
                    previousReloadAnswer = reloadPrompt(document->filePath(), document->isModified(),
                                                        DiffService::instance(),
                                                        ICore::dialogParent());
                    switch (previousReloadAnswer) {
                    case ReloadAll:
                    case ReloadCurrent:
                        success = document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents);
                        break;
                    case ReloadSkipCurrent:
                    case ReloadNone:
                    case ReloadNoneAndDiff:
                        success = document->reload(&errorString, IDocument::FlagIgnore, IDocument::TypeContents);
                        break;
                    case CloseCurrent:
                        documentsToClose << document;
                        break;
                    }
                }
                if (previousReloadAnswer == ReloadNoneAndDiff)
                    filesToDiff.append(document->filePath().toString());

            // IDocument wants us to ask, and it's the TypeRemoved case
            } else {
                // Ask about removed file
                bool unhandled = true;
                while (unhandled) {
                    if (previousDeletedAnswer != FileDeletedCloseAll) {
                        previousDeletedAnswer =
                                fileDeletedPrompt(document->filePath().toString(),
                                                         trigger == IDocument::TriggerExternal,
                                                         QApplication::activeWindow());
                    }
                    switch (previousDeletedAnswer) {
                    case FileDeletedSave:
                        documentsToSave.insert(document, document->filePath().toString());
                        unhandled = false;
                        break;
                    case FileDeletedSaveAs:
                    {
                        const QString &saveFileName = getSaveAsFileName(document);
                        if (!saveFileName.isEmpty()) {
                            documentsToSave.insert(document, saveFileName);
                            unhandled = false;
                        }
                        break;
                    }
                    case FileDeletedClose:
                    case FileDeletedCloseAll:
                        documentsToClose << document;
                        unhandled = false;
                        break;
                    }
                }
            }
        }
        if (!success) {
            if (errorString.isEmpty())
                errorStrings << tr("Cannot reload %1").arg(document->filePath().toUserOutput());
            else
                errorStrings << errorString;
        }

        d->m_blockedIDocument = nullptr;
    }

    if (!filesToDiff.isEmpty()) {
        if (auto diffService = DiffService::instance())
            diffService->diffModifiedFiles(filesToDiff);
    }

    if (!errorStrings.isEmpty())
        QMessageBox::critical(ICore::dialogParent(), tr("File Error"),
                              errorStrings.join(QLatin1Char('\n')));

    // handle deleted files
    EditorManager::closeDocuments(documentsToClose, false);
    QMapIterator<IDocument *, QString> it(documentsToSave);
    while (it.hasNext()) {
        it.next();
        saveDocument(it.key(), it.value());
        it.key()->checkPermissions();
    }

    d->m_blockActivated = false;
    // re-check in case files where modified while the dialog was open
    QTimer::singleShot(0, this, &DocumentManager::checkForReload);
//    dump();
}

/*!
    Adds the \a fileName to the list of recent files. Associates the file to
    be reopened with the editor that has the specified \a editorId, if possible.
    \a editorId defaults to the empty id, which lets \QC figure out
    the best editor itself.
*/
void DocumentManager::addToRecentFiles(const QString &fileName, Id editorId)
{
    if (fileName.isEmpty())
        return;
    QString fileKey = filePathKey(fileName, KeepLinks);
    QMutableListIterator<RecentFile > it(d->m_recentFiles);
    while (it.hasNext()) {
        RecentFile file = it.next();
        QString recentFileKey(filePathKey(file.first, DocumentManager::KeepLinks));
        if (fileKey == recentFileKey)
            it.remove();
    }
    if (d->m_recentFiles.count() > d->m_maxRecentFiles)
        d->m_recentFiles.removeLast();
    d->m_recentFiles.prepend(RecentFile(fileName, editorId));
}

/*!
    Clears the list of recent files. Should only be called by
    the core plugin when the user chooses to clear the list.
*/
void DocumentManager::clearRecentFiles()
{
    d->m_recentFiles.clear();
}

/*!
    Returns the list of recent files.
*/
QList<DocumentManager::RecentFile> DocumentManager::recentFiles()
{
    return d->m_recentFiles;
}

void DocumentManager::saveSettings()
{
    QStringList recentFiles;
    QStringList recentEditorIds;
    foreach (const RecentFile &file, d->m_recentFiles) {
        recentFiles.append(file.first);
        recentEditorIds.append(file.second.toString());
    }

    QSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(filesKeyC), recentFiles);
    s->setValue(QLatin1String(editorsKeyC), recentEditorIds);
    s->endGroup();
    s->beginGroup(QLatin1String(directoryGroupC));
    s->setValue(QLatin1String(projectDirectoryKeyC), d->m_projectsDirectory.toString());
    s->setValue(QLatin1String(useProjectDirectoryKeyC), d->m_useProjectsDirectory);
    s->setValue(QLatin1String(buildDirectoryKeyC), d->m_buildDirectory);
    s->endGroup();
}

void readSettings()
{
    QSettings *s = ICore::settings();
    d->m_recentFiles.clear();
    s->beginGroup(QLatin1String(settingsGroupC));
    QStringList recentFiles = s->value(QLatin1String(filesKeyC)).toStringList();
    QStringList recentEditorIds = s->value(QLatin1String(editorsKeyC)).toStringList();
    s->endGroup();
    // clean non-existing files
    QStringListIterator ids(recentEditorIds);
    foreach (const QString &fileName, recentFiles) {
        QString editorId;
        if (ids.hasNext()) // guard against old or weird settings
            editorId = ids.next();
        if (QFileInfo(fileName).isFile())
            d->m_recentFiles.append(DocumentManager::RecentFile(QDir::fromNativeSeparators(fileName), // from native to guard against old settings
                                               Id::fromString(editorId)));
    }

    s->beginGroup(QLatin1String(directoryGroupC));
    const FileName settingsProjectDir = FileName::fromString(s->value(QLatin1String(projectDirectoryKeyC),
                                                QString()).toString());
    if (!settingsProjectDir.isEmpty() && settingsProjectDir.toFileInfo().isDir())
        d->m_projectsDirectory = settingsProjectDir;
    else
        d->m_projectsDirectory = FileName::fromString(PathChooser::homePath());
    d->m_useProjectsDirectory = s->value(QLatin1String(useProjectDirectoryKeyC),
                                         d->m_useProjectsDirectory).toBool();

    const QString settingsShadowDir = s->value(QLatin1String(buildDirectoryKeyC),
                                               QString()).toString();
    if (!settingsShadowDir.isEmpty())
        d->m_buildDirectory = settingsShadowDir;
    else
        d->m_buildDirectory = QLatin1String(Constants::DEFAULT_BUILD_DIRECTORY);

    s->endGroup();
}

/*!

  Returns the initial directory for a new file dialog. If there is
  a current file, uses that, otherwise if there is a default location for
  new files, uses that, otherwise uses the last visited directory.

  \sa setFileDialogLastVisitedDirectory
  \sa setDefaultLocationForNewFiles
*/

QString DocumentManager::fileDialogInitialDirectory()
{
    IDocument *doc = EditorManager::currentDocument();
    if (doc && !doc->isTemporary() && !doc->filePath().isEmpty())
        return doc->filePath().toFileInfo().absolutePath();
    if (!d->m_defaultLocationForNewFiles.isEmpty())
        return d->m_defaultLocationForNewFiles;
    return d->m_lastVisitedDirectory;
}

/*!

  Sets the default location for new files

  \sa fileDialogInitialDirectory
*/
QString DocumentManager::defaultLocationForNewFiles()
{
    return d->m_defaultLocationForNewFiles;
}

/*!
 Returns the default location for new files
 */
void DocumentManager::setDefaultLocationForNewFiles(const QString &location)
{
    d->m_defaultLocationForNewFiles = location;
}

/*!

  Returns the directory for projects. Defaults to HOME.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

FileName DocumentManager::projectsDirectory()
{
    return d->m_projectsDirectory;
}

/*!

  Sets the directory for projects.

  \sa projectsDirectory, useProjectsDirectory
*/

void DocumentManager::setProjectsDirectory(const FileName &directory)
{
    if (d->m_projectsDirectory != directory) {
        d->m_projectsDirectory = directory;
        emit m_instance->projectsDirectoryChanged(d->m_projectsDirectory);
    }
}

/*!
    Returns the default build directory.

    \sa setBuildDirectory
*/
QString DocumentManager::buildDirectory()
{
    return d->m_buildDirectory;
}

/*!
    Sets the shadow build directory to \a directory.

    \sa buildDirectory
*/
void DocumentManager::setBuildDirectory(const QString &directory)
{
    d->m_buildDirectory = directory;
}

/*!

    Returns whether the directory for projects is to be used or whether the user
    chose to use the current directory.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

bool DocumentManager::useProjectsDirectory()
{
    return d->m_useProjectsDirectory;
}

/*!

  Sets whether the directory for projects is to be used.

  \sa projectsDirectory, useProjectsDirectory
*/

void DocumentManager::setUseProjectsDirectory(bool useProjectsDirectory)
{
    d->m_useProjectsDirectory = useProjectsDirectory;
}

/*!

  Returns the last visited directory of a file dialog.

  \sa setFileDialogLastVisitedDirectory, fileDialogInitialDirectory

*/

QString DocumentManager::fileDialogLastVisitedDirectory()
{
    return d->m_lastVisitedDirectory;
}

/*!

  Sets the last visited directory of a file dialog that will be remembered
  for the next one.

  \sa fileDialogLastVisitedDirectory, fileDialogInitialDirectory

  */

void DocumentManager::setFileDialogLastVisitedDirectory(const QString &directory)
{
    d->m_lastVisitedDirectory = directory;
}

void DocumentManager::notifyFilesChangedInternally(const QStringList &files)
{
    emit m_instance->filesChangedInternally(files);
}

bool DocumentManager::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == qApp && e->type() == QEvent::ApplicationStateChange) {
        QTimer::singleShot(0, this, &DocumentManager::checkForReload);
    }
    return false;
}

// -------------- FileChangeBlocker

FileChangeBlocker::FileChangeBlocker(const QString &fileName)
    : m_fileName(fileName)
{
    DocumentManager::expectFileChange(fileName);
}

FileChangeBlocker::~FileChangeBlocker()
{
    DocumentManager::unexpectFileChange(m_fileName);
}

} // namespace Core

#include "documentmanager.moc"
