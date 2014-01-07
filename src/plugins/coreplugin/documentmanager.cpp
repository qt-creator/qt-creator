/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "documentmanager.h"

#include "icore.h"
#include "idocument.h"
#include "mimedatabase.h"
#include "coreconstants.h"

#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/dialogs/saveitemsdialog.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/reloadpromptutils.h>

#include <QStringList>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QTimer>
#include <QAction>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

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

namespace Core {

static void readSettings();

static QList<IDocument *> saveModifiedFilesHelper(const QList<IDocument *> &documents,
                               bool *cancelled, bool silently,
                               const QString &message,
                               const QString &alwaysSaveMessage = QString(),
                               bool *alwaysSave = 0);

namespace Internal {

struct OpenWithEntry
{
    OpenWithEntry() : editorFactory(0), externalEditor(0) {}
    IEditorFactory *editorFactory;
    IExternalEditor *externalEditor;
    QString fileName;
};

struct FileStateItem
{
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileState
{
    QMap<IDocument *, FileStateItem> lastUpdatedState;
    FileStateItem expected;
};


struct DocumentManagerPrivate
{
    DocumentManagerPrivate();
    QFileSystemWatcher *fileWatcher();
    QFileSystemWatcher *linkWatcher();

    QMap<QString, FileState> m_states;
    QSet<QString> m_changedFiles;
    QList<IDocument *> m_documentsWithoutWatch;
    QMap<IDocument *, QStringList> m_documentsWithWatch;
    QSet<QString> m_expectedFileNames;

    QList<DocumentManager::RecentFile> m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QString m_currentFile;

    QFileSystemWatcher *m_fileWatcher; // Delayed creation.
    QFileSystemWatcher *m_linkWatcher; // Delayed creation (only UNIX/if a link is seen).
    bool m_blockActivated;
    QString m_lastVisitedDirectory;
    QString m_projectsDirectory;
    bool m_useProjectsDirectory;
    QString m_buildDirectory;
    // When we are calling into an IDocument
    // we don't want to receive a changed()
    // signal
    // That makes the code easier
    IDocument *m_blockedIDocument;
};

static DocumentManager *m_instance;
static Internal::DocumentManagerPrivate *d;

QFileSystemWatcher *DocumentManagerPrivate::fileWatcher()
{
    if (!m_fileWatcher) {
        m_fileWatcher= new QFileSystemWatcher(m_instance);
        QObject::connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
                         m_instance, SLOT(changedFile(QString)));
    }
    return m_fileWatcher;
}

QFileSystemWatcher *DocumentManagerPrivate::linkWatcher()
{
    if (Utils::HostOsInfo::isAnyUnixHost()) {
        if (!m_linkWatcher) {
            m_linkWatcher = new QFileSystemWatcher(m_instance);
            m_linkWatcher->setObjectName(QLatin1String("_qt_autotest_force_engine_poller"));
            QObject::connect(m_linkWatcher, SIGNAL(fileChanged(QString)),
                             m_instance, SLOT(changedFile(QString)));
        }
        return m_linkWatcher;
    }

    return fileWatcher();
}

DocumentManagerPrivate::DocumentManagerPrivate() :
    m_fileWatcher(0),
    m_linkWatcher(0),
    m_blockActivated(false),
    m_lastVisitedDirectory(QDir::currentPath()),
    m_useProjectsDirectory(Utils::HostOsInfo::isMacHost()), // Creator is in bizarre places when launched via finder.
    m_blockedIDocument(0)
{
}

} // namespace Internal
} // namespace Core

Q_DECLARE_METATYPE(Core::Internal::OpenWithEntry)

namespace Core {

using namespace Internal;

DocumentManager::DocumentManager(QObject *parent)
  : QObject(parent)
{
    d = new DocumentManagerPrivate;
    m_instance = this;
    connect(ICore::instance(), SIGNAL(contextChanged(QList<Core::IContext*>,Core::Context)),
        this, SLOT(syncWithEditor(QList<Core::IContext*>)));
    qApp->installEventFilter(this);

    readSettings();
}

DocumentManager::~DocumentManager()
{
    delete d;
}

QObject *DocumentManager::instance()
{
    return m_instance;
}

/* only called from addFileInfo(IDocument *) */
static void addFileInfo(const QString &fileName, IDocument *document, bool isLink)
{
    FileStateItem state;
    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        state.modified = fi.lastModified();
        state.permissions = fi.permissions();
        // Add watcher if we don't have that already
        if (!d->m_states.contains(fileName))
            d->m_states.insert(fileName, FileState());

        QFileSystemWatcher *watcher = 0;
        if (isLink)
            watcher = d->linkWatcher();
        else
            watcher = d->fileWatcher();
        if (!watcher->files().contains(fileName))
            watcher->addPath(fileName);

        d->m_states[fileName].lastUpdatedState.insert(document, state);
    }
    d->m_documentsWithWatch[document].append(fileName); // inserts a new QStringList if not already there
}

/* Adds the IDocument's file and possibly it's final link target to both m_states
   (if it's file name is not empty), and the m_filesWithWatch list,
   and adds a file watcher for each if not already done.
   (The added file names are guaranteed to be absolute and cleaned.) */
static void addFileInfo(IDocument *document)
{
    const QString fixedName = DocumentManager::fixFileName(document->filePath(), DocumentManager::KeepLinks);
    const QString fixedResolvedName = DocumentManager::fixFileName(document->filePath(), DocumentManager::ResolveLinks);
    addFileInfo(fixedResolvedName, document, false);
    if (fixedName != fixedResolvedName)
        addFileInfo(fixedName, document, true);
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
                connect(document, SIGNAL(destroyed(QObject*)), m_instance, SLOT(documentDestroyed(QObject*)));
                connect(document, SIGNAL(filePathChanged(QString,QString)), m_instance, SLOT(filePathChanged(QString,QString)));
                d->m_documentsWithoutWatch.append(document);
            }
        }
        return;
    }

    foreach (IDocument *document, documents) {
        if (document && !d->m_documentsWithWatch.contains(document)) {
            connect(document, SIGNAL(changed()), m_instance, SLOT(checkForNewFileName()));
            connect(document, SIGNAL(destroyed(QObject*)), m_instance, SLOT(documentDestroyed(QObject*)));
            connect(document, SIGNAL(filePathChanged(QString,QString)), m_instance, SLOT(filePathChanged(QString,QString)));
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
        d->m_states[fileName].lastUpdatedState.remove(document);
        if (d->m_states.value(fileName).lastUpdatedState.isEmpty()) {
            if (d->m_fileWatcher && d->m_fileWatcher->files().contains(fileName))
                d->m_fileWatcher->removePath(fileName);
            if (d->m_linkWatcher && d->m_linkWatcher->files().contains(fileName))
                d->m_linkWatcher->removePath(fileName);
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
    const QString &fixedFrom = fixFileName(from, KeepLinks);

    // gather the list of IDocuments
    QList<IDocument *> documentsToRename;
    QMapIterator<IDocument *, QStringList> it(d->m_documentsWithWatch);
    while (it.hasNext()) {
        it.next();
        if (it.value().contains(fixedFrom))
            documentsToRename.append(it.key());
    }

    // rename the IDocuments
    foreach (IDocument *document, documentsToRename) {
        d->m_blockedIDocument = document;
        removeFileInfo(document);
        document->setFilePath(to);
        addFileInfo(document);
        d->m_blockedIDocument = 0;
    }
    emit m_instance->allDocumentsRenamed(from, to);
}

void DocumentManager::filePathChanged(const QString &oldName, const QString &newName)
{
    IDocument *doc = qobject_cast<IDocument *>(sender());
    QTC_ASSERT(doc, return);
    if (doc == d->m_blockedIDocument)
        return;
    emit m_instance->documentRenamed(doc, oldName, newName);
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
        disconnect(document, SIGNAL(changed()), m_instance, SLOT(checkForNewFileName()));
    }
    disconnect(document, SIGNAL(destroyed(QObject*)), m_instance, SLOT(documentDestroyed(QObject*)));
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
    Returns a guaranteed cleaned path in native form. If the file exists,
    it will either be a cleaned absolute file path (fixmode == KeepLinks), or
    a cleaned canonical file path (fixmode == ResolveLinks).
*/
QString DocumentManager::fixFileName(const QString &fileName, FixMode fixmode)
{
    QString s = fileName;
    QFileInfo fi(s);
    if (fi.exists()) {
        if (fixmode == ResolveLinks)
            s = fi.canonicalFilePath();
        else
            s = QDir::cleanPath(fi.absoluteFilePath());
    } else {
        s = QDir::cleanPath(s);
    }
    s = QDir::toNativeSeparators(s);
    if (Utils::HostOsInfo::isWindowsHost())
        s = s.toLower();
    return s;
}

/*!
    Returns the list of IDocuments that have been modified.
*/
QList<IDocument *> DocumentManager::modifiedDocuments()
{
    QList<IDocument *> modified;

    foreach (IDocument *document, d->m_documentsWithWatch.keys()) {
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
static void updateExpectedState(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    if (d->m_states.contains(fileName)) {
        QFileInfo fi(fileName);
        d->m_states[fileName].expected.modified = fi.lastModified();
        d->m_states[fileName].expected.permissions = fi.permissions();
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
    const QString fixedName = fixFileName(fileName, KeepLinks);
    updateExpectedState(fixedName);
    const QString fixedResolvedName = fixFileName(fileName, ResolveLinks);
    if (fixedName != fixedResolvedName)
        updateExpectedState(fixedResolvedName);
}


/*!
    Tries to save the files listed in \a documents. The \a cancelled argument is set to true
    if the user cancelled the dialog. Returns the files that could not be saved. If the files
    listed in documents have no write permissions, an additional dialog will be
    displayed to
    query the user for these permissions.
*/
QList<IDocument *> DocumentManager::saveModifiedDocumentsSilently(const QList<IDocument *> &documents, bool *cancelled)
{
    return saveModifiedFilesHelper(documents, cancelled, true, QString());
}

/*!
    Asks the user whether to save the files listed in \a documents. Opens a
    dialog that displays the \a message, and additional text to ask the users
    if they want to enable automatic saving
    of modified files (in this context).

    The \a cancelled argument is set to true if the user cancels the dialog.
    \a alwaysSave is set to match the selection of the user if files should
    always automatically be saved. If the files listed in documents have no write
    permissions, an additional dialog will be displayed to query the user for
    these permissions.

    Returns the files that have not been saved.
*/
QList<IDocument *> DocumentManager::saveModifiedDocuments(const QList<IDocument *> &documents,
                                              bool *cancelled, const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    return saveModifiedFilesHelper(documents, cancelled, false, message, alwaysSaveMessage, alwaysSave);
}

static QList<IDocument *> saveModifiedFilesHelper(const QList<IDocument *> &documents,
                                              bool *cancelled,
                                              bool silently,
                                              const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    if (cancelled)
        (*cancelled) = false;

    QList<IDocument *> notSaved;
    QMap<IDocument *, QString> modifiedDocumentsMap;
    QList<IDocument *> modifiedDocuments;

    foreach (IDocument *document, documents) {
        if (document->isModified()) {
            QString name = document->filePath();
            if (name.isEmpty())
                name = document->suggestedFileName();

            // There can be several IDocuments pointing to the same file
            // Prefer one that is not readonly
            // (even though it *should* not happen that the IDocuments are inconsistent with readonly)
            if (!modifiedDocumentsMap.key(name, 0) || !document->isFileReadOnly())
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
                    *alwaysSave = dia.alwaysSaveChecked();
                notSaved = modifiedDocuments;
                return notSaved;
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
            if (roDialog.exec() == Core::Internal::ReadOnlyFilesDialog::RO_Cancel) {
                if (cancelled)
                    (*cancelled) = true;
                notSaved = modifiedDocuments;
                return notSaved;
            }
        }
        foreach (IDocument *document, documentsToSave) {
            if (!EditorManager::saveDocument(document)) {
                if (cancelled)
                    *cancelled = true;
                notSaved.append(document);
            }
        }
    }
    return notSaved;
}

bool DocumentManager::saveDocument(IDocument *document, const QString &fileName, bool *isReadOnly)
{
    bool ret = true;
    QString effName = fileName.isEmpty() ? document->filePath() : fileName;
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
            if (selectedFilter && *selectedFilter != QCoreApplication::translate(
                    "Core", Constants::ALL_FILES_FILTER)) {
                // Mime database creates filter strings like this: Anything here (*.foo *.bar)
                QRegExp regExp(QLatin1String(".*\\s+\\((.*)\\)$"));
                const int index = regExp.lastIndexIn(*selectedFilter);
                bool suffixOk = false;
                if (index != -1) {
                    const QStringList &suffixes = regExp.cap(1).remove(QLatin1Char('*')).split(QLatin1Char(' '));
                    foreach (const QString &suffix, suffixes)
                        if (fileName.endsWith(suffix)) {
                            suffixOk = true;
                            break;
                        }
                    if (!suffixOk && !suffixes.isEmpty())
                        fileName.append(suffixes.at(0));
                }
            }
            if (QFile::exists(fileName)) {
                if (QMessageBox::warning(ICore::dialogParent(), tr("Overwrite?"),
                    tr("An item named '%1' already exists at this location. "
                       "Do you want to overwrite it?").arg(fileName),
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
QString DocumentManager::getSaveAsFileName(const IDocument *document, const QString &filter, QString *selectedFilter)
{
    if (!document)
        return QLatin1String("");
    QString absoluteFilePath = document->filePath();
    const QFileInfo fi(absoluteFilePath);
    QString fileName = fi.fileName();
    QString path = fi.absolutePath();
    if (absoluteFilePath.isEmpty()) {
        fileName = document->suggestedFileName();
        const QString defaultPath = document->defaultPath();
        if (!defaultPath.isEmpty())
            path = defaultPath;
    }

    QString filterString;
    if (filter.isEmpty()) {
        if (const MimeType &mt = MimeDatabase::findByFile(fi))
            filterString = mt.filterString();
        selectedFilter = &filterString;
    } else {
        filterString = filter;
    }

    absoluteFilePath = getSaveFileName(tr("Save File As"),
        path + QDir::separator() + fileName,
        filterString,
        selectedFilter);
    return absoluteFilePath;
}

/*!
    Asks the user for a set of file names to be opened. The \a filters
    and \a selectedFilter arguments are interpreted like in
    \c QFileDialog::getOpenFileNames(). \a pathIn specifies a path to open the
    dialog in if that is not overridden by the user's policy.
*/

QStringList DocumentManager::getOpenFileNames(const QString &filters,
                                          const QString pathIn,
                                          QString *selectedFilter)
{
    QString path = pathIn;
    if (path.isEmpty()) {
        if (!d->m_currentFile.isEmpty())
            path = QFileInfo(d->m_currentFile).absoluteFilePath();
        if (path.isEmpty() && useProjectsDirectory())
            path = projectsDirectory();
    }
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

    if (d->m_states.contains(fileName))
        d->m_changedFiles.insert(fileName);

    if (wasempty && !d->m_changedFiles.isEmpty())
        QTimer::singleShot(200, this, SLOT(checkForReload()));
}

void DocumentManager::checkForReload()
{
    if (d->m_changedFiles.isEmpty())
        return;
    if (!QApplication::activeWindow() || QApplication::activeModalWidget())
        return;

    if (d->m_blockActivated)
        return;

    d->m_blockActivated = true;

    IDocument::ReloadSetting defaultBehavior = EditorManager::reloadSetting();
    Utils::ReloadPromptAnswer previousReloadAnswer = Utils::ReloadCurrent;
    Utils::FileDeletedPromptAnswer previousDeletedAnswer = Utils::FileDeletedSave;

    QList<IDocument *> documentsToClose;
    QMap<IDocument*, QString> documentsToSave;

    // collect file information
    QMap<QString, FileStateItem> currentStates;
    QMap<QString, IDocument::ChangeType> changeTypes;
    QSet<IDocument *> changedIDocuments;
    foreach (const QString &fileName, d->m_changedFiles) {
        IDocument::ChangeType type = IDocument::TypeContents;
        FileStateItem state;
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            type = IDocument::TypeRemoved;
        } else {
            state.modified = fi.lastModified();
            state.permissions = fi.permissions();
        }
        currentStates.insert(fileName, state);
        changeTypes.insert(fileName, type);
        foreach (IDocument *document, d->m_states.value(fileName).lastUpdatedState.keys())
            changedIDocuments.insert(document);
    }

    // clean up. do this before we may enter the main loop, otherwise we would
    // lose consecutive notifications.
    d->m_changedFiles.clear();

    // collect information about "expected" file names
    // we can't do the "resolving" already in expectFileChange, because
    // if the resolved names are different when unexpectFileChange is called
    // we would end up with never-unexpected file names
    QSet<QString> expectedFileNames;
    foreach (const QString &fileName, d->m_expectedFileNames) {
        const QString fixedName = fixFileName(fileName, KeepLinks);
        expectedFileNames.insert(fixedName);
        const QString fixedResolvedName = fixFileName(fileName, ResolveLinks);
        if (fixedName != fixedResolvedName)
            expectedFileNames.insert(fixedResolvedName);
    }

    // handle the IDocuments
    QStringList errorStrings;
    foreach (IDocument *document, changedIDocuments) {
        IDocument::ChangeTrigger trigger = IDocument::TriggerInternal;
        IDocument::ChangeType type = IDocument::TypePermissions;
        bool changed = false;
        // find out the type & behavior from the two possible files
        // behavior is internal if all changes are expected (and none removed)
        // type is "max" of both types (remove > contents > permissions)
        foreach (const QString & fileName, d->m_documentsWithWatch.value(document)) {
            // was the file reported?
            if (!currentStates.contains(fileName))
                continue;

            FileStateItem currentState = currentStates.value(fileName);
            FileStateItem expectedState = d->m_states.value(fileName).expected;
            FileStateItem lastState = d->m_states.value(fileName).lastUpdatedState.value(document);

            // did the file actually change?
            if (lastState.modified == currentState.modified && lastState.permissions == currentState.permissions)
                continue;
            changed = true;

            // was it only a permission change?
            if (lastState.modified == currentState.modified)
                continue;

            // was the change unexpected?
            if ((currentState.modified != expectedState.modified || currentState.permissions != expectedState.permissions)
                    && !expectedFileNames.contains(fileName)) {
                trigger = IDocument::TriggerExternal;
            }

            // find out the type
            IDocument::ChangeType fileChange = changeTypes.value(fileName);
            if (fileChange == IDocument::TypeRemoved)
                type = IDocument::TypeRemoved;
            else if (fileChange == IDocument::TypeContents && type == IDocument::TypePermissions)
                type = IDocument::TypeContents;
        }

        if (!changed) // probably because the change was blocked with (un)blockFileChange
            continue;

        // handle it!
        d->m_blockedIDocument = document;

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
                if (previousReloadAnswer == Utils::ReloadNone) {
                    // answer already given, ignore
                    success = document->reload(&errorString, IDocument::FlagIgnore, IDocument::TypeContents);
                } else if (previousReloadAnswer == Utils::ReloadAll) {
                    // answer already given, reload
                    success = document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents);
                } else {
                    // Ask about content change
                    previousReloadAnswer = Utils::reloadPrompt(document->filePath(), document->isModified(),
                                                               ICore::dialogParent());
                    switch (previousReloadAnswer) {
                    case Utils::ReloadAll:
                    case Utils::ReloadCurrent:
                        success = document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents);
                        break;
                    case Utils::ReloadSkipCurrent:
                    case Utils::ReloadNone:
                        success = document->reload(&errorString, IDocument::FlagIgnore, IDocument::TypeContents);
                        break;
                    case Utils::CloseCurrent:
                        documentsToClose << document;
                        break;
                    }
                }
            // IDocument wants us to ask, and it's the TypeRemoved case
            } else {
                // Ask about removed file
                bool unhandled = true;
                while (unhandled) {
                    if (previousDeletedAnswer != Utils::FileDeletedCloseAll) {
                        previousDeletedAnswer =
                                Utils::fileDeletedPrompt(document->filePath(),
                                                         trigger == IDocument::TriggerExternal,
                                                         QApplication::activeWindow());
                    }
                    switch (previousDeletedAnswer) {
                    case Utils::FileDeletedSave:
                        documentsToSave.insert(document, document->filePath());
                        unhandled = false;
                        break;
                    case Utils::FileDeletedSaveAs:
                    {
                        const QString &saveFileName = getSaveAsFileName(document);
                        if (!saveFileName.isEmpty()) {
                            documentsToSave.insert(document, saveFileName);
                            unhandled = false;
                        }
                        break;
                    }
                    case Utils::FileDeletedClose:
                    case Utils::FileDeletedCloseAll:
                        documentsToClose << document;
                        unhandled = false;
                        break;
                    }
                }
            }
        }
        if (!success) {
            if (errorString.isEmpty())
                errorStrings << tr("Cannot reload %1").arg(QDir::toNativeSeparators(document->filePath()));
            else
                errorStrings << errorString;
        }

        // update file info, also handling if e.g. link target has changed
        removeFileInfo(document);
        addFileInfo(document);
        d->m_blockedIDocument = 0;
    }
    if (!errorStrings.isEmpty())
        QMessageBox::critical(ICore::dialogParent(), tr("File Error"),
                              errorStrings.join(QLatin1String("\n")));

    // handle deleted files
    EditorManager::closeDocuments(documentsToClose, false);
    QMapIterator<IDocument *, QString> it(documentsToSave);
    while (it.hasNext()) {
        it.next();
        saveDocument(it.key(), it.value());
        it.key()->checkPermissions();
    }

    d->m_blockActivated = false;

//    dump();
}

void DocumentManager::syncWithEditor(const QList<Core::IContext *> &context)
{
    if (context.isEmpty())
        return;

    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (!editor || editor->document()->isTemporary())
        return;
    foreach (IContext *c, context) {
        if (editor->widget() == c->widget()) {
            setCurrentFile(editor->document()->filePath());
            break;
        }
    }
}

/*!
    Adds the \a fileName to the list of recent files. Associates the file to
    be reopened with the editor that has the specified \a editorId, if possible.
    \a editorId defaults to the empty id, which lets \QC figure out
    the best editor itself.
*/
void DocumentManager::addToRecentFiles(const QString &fileName, const Id &editorId)
{
    if (fileName.isEmpty())
        return;
    QString unifiedForm(fixFileName(fileName, KeepLinks));
    QMutableListIterator<RecentFile > it(d->m_recentFiles);
    while (it.hasNext()) {
        RecentFile file = it.next();
        QString recentUnifiedForm(fixFileName(file.first, DocumentManager::KeepLinks));
        if (unifiedForm == recentUnifiedForm)
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

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(filesKeyC), recentFiles);
    s->setValue(QLatin1String(editorsKeyC), recentEditorIds);
    s->endGroup();
    s->beginGroup(QLatin1String(directoryGroupC));
    s->setValue(QLatin1String(projectDirectoryKeyC), d->m_projectsDirectory);
    s->setValue(QLatin1String(useProjectDirectoryKeyC), d->m_useProjectsDirectory);
    s->setValue(QLatin1String(buildDirectoryKeyC), d->m_buildDirectory);
    s->endGroup();
}

void readSettings()
{
    QSettings *s = Core::ICore::settings();
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
    const QString settingsProjectDir = s->value(QLatin1String(projectDirectoryKeyC),
                                                QString()).toString();
    if (!settingsProjectDir.isEmpty() && QFileInfo(settingsProjectDir).isDir())
        d->m_projectsDirectory = settingsProjectDir;
    else
        d->m_projectsDirectory = Utils::PathChooser::homePath();
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

  The current file is the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active.

  \sa currentFile
  */
void DocumentManager::setCurrentFile(const QString &filePath)
{
    if (d->m_currentFile == filePath)
        return;
    d->m_currentFile = filePath;
    emit m_instance->currentFileChanged(d->m_currentFile);
}

/*!
  Returns the absolute path of the current file.

  The current file is the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active.

  \sa setCurrentFile
  */
QString DocumentManager::currentFile()
{
    return d->m_currentFile;
}

/*!

  Returns the initial directory for a new file dialog. If there is
  a current file, uses that, otherwise uses the last visited directory.

  \sa setFileDialogLastVisitedDirectory
*/

QString DocumentManager::fileDialogInitialDirectory()
{
    if (!d->m_currentFile.isEmpty())
        return QFileInfo(d->m_currentFile).absolutePath();
    return d->m_lastVisitedDirectory;
}

/*!

  Returns the directory for projects. Defaults to HOME.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

QString DocumentManager::projectsDirectory()
{
    return d->m_projectsDirectory;
}

/*!

  Sets the directory for projects.

  \sa projectsDirectory, useProjectsDirectory
*/

void DocumentManager::setProjectsDirectory(const QString &dir)
{
    d->m_projectsDirectory = dir;
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

void DocumentManager::populateOpenWithMenu(QMenu *menu, const QString &fileName)
{
    typedef QList<IEditorFactory*> EditorFactoryList;
    typedef QList<IExternalEditor*> ExternalEditorList;

    menu->clear();

    bool anyMatches = false;

    if (const MimeType mt = MimeDatabase::findByFile(QFileInfo(fileName))) {
        const EditorFactoryList factories = EditorManager::editorFactories(mt, false);
        const ExternalEditorList externalEditors = EditorManager::externalEditors(mt, false);
        anyMatches = !factories.empty() || !externalEditors.empty();
        if (anyMatches) {
            // Add all suitable editors
            foreach (IEditorFactory *editorFactory, factories) {
                // Add action to open with this very editor factory
                QString const actionTitle = editorFactory->displayName();
                QAction * const action = menu->addAction(actionTitle);
                OpenWithEntry entry;
                entry.editorFactory = editorFactory;
                entry.fileName = fileName;
                action->setData(qVariantFromValue(entry));
            }
            // Add all suitable external editors
            foreach (IExternalEditor *externalEditor, externalEditors) {
                QAction * const action = menu->addAction(externalEditor->displayName());
                OpenWithEntry entry;
                entry.externalEditor = externalEditor;
                entry.fileName = fileName;
                action->setData(qVariantFromValue(entry));
            }
        }
    }
    menu->setEnabled(anyMatches);
}

void DocumentManager::executeOpenWithMenuAction(QAction *action)
{
    QTC_ASSERT(action, return);
    const QVariant data = action->data();
    OpenWithEntry entry = qvariant_cast<OpenWithEntry>(data);
    if (entry.editorFactory) {
        // close any open editors that have this file open, but have a different type.
        QList<IEditor *> editorsOpenForFile
                = EditorManager::documentModel()->editorsForFilePath(entry.fileName);
        if (!editorsOpenForFile.isEmpty()) {
            foreach (IEditor *openEditor, editorsOpenForFile) {
                if (entry.editorFactory->id() == openEditor->id())
                    editorsOpenForFile.removeAll(openEditor);
            }
            if (!EditorManager::closeEditors(editorsOpenForFile)) // don't open if cancel was pressed
                return;
        }

        EditorManager::openEditor(entry.fileName, entry.editorFactory->id());
        return;
    }
    if (entry.externalEditor)
        EditorManager::openExternalEditor(entry.fileName, entry.externalEditor->id());
}

void DocumentManager::slotExecuteOpenWithMenuAction(QAction *action)
{
    executeOpenWithMenuAction(action);
}

bool DocumentManager::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == qApp && e->type() == QEvent::ApplicationActivate) {
        // activeWindow is not necessarily set yet, do checkForReload asynchronously
        QTimer::singleShot(0, this, SLOT(checkForReload()));
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
