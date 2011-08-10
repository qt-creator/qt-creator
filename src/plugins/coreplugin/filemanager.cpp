/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "filemanager.h"

#include "editormanager.h"
#include "ieditor.h"
#include "icore.h"
#include "ifile.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "saveitemsdialog.h"
#include "vcsmanager.h"
#include "coreconstants.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/reloadpromptutils.h>

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QDateTime>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>

/*!
  \class Core::FileManager
  \mainclass
  \inheaderfile filemanager.h
  \brief Manages a set of IFile objects.

  The FileManager service monitors a set of IFile's. Plugins should register
  files they work with at the service. The files the IFile's point to will be
  monitored at filesystem level. If a file changes, the status of the IFile's
  will be adjusted accordingly. Furthermore, on application exit the user will
  be asked to save all modified files.

  Different IFile objects in the set can point to the same file in the
  filesystem. The monitoring for a IFile can be blocked by blockFileChange(), and
  enabled again by unblockFileChange().

  The functions expectFileChange() and unexpectFileChange() mark a file change
  as expected. On expected file changes all IFile objects are notified to reload
  themselves.

  The FileManager service also provides two convenience methods for saving
  files: saveModifiedFiles() and saveModifiedFilesSilently(). Both take a list
  of FileInterfaces as an argument, and return the list of files which were
  _not_ saved.

  The service also manages the list of recent files to be shown to the user
  (see addToRecentFiles() and recentFiles()).
 */

static const char settingsGroupC[] = "RecentFiles";
static const char filesKeyC[] = "Files";
static const char editorsKeyC[] = "EditorIds";

static const char directoryGroupC[] = "Directories";
static const char projectDirectoryKeyC[] = "Projects";
static const char useProjectDirectoryKeyC[] = "UseProjectsDirectory";

namespace Core {
namespace Internal {

struct FileStateItem
{
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileState
{
    QMap<IFile *, FileStateItem> lastUpdatedState;
    FileStateItem expected;
};


struct FileManagerPrivate {
    explicit FileManagerPrivate(FileManager *q, QMainWindow *mw);
    QFileSystemWatcher *fileWatcher();
    QFileSystemWatcher *linkWatcher();

    static FileManager *m_instance;
    QMap<QString, FileState> m_states;
    QSet<QString> m_changedFiles;
    QList<IFile *> m_filesWithoutWatch;
    QMap<IFile *, QStringList> m_filesWithWatch;
    QSet<QString> m_expectedFileNames;

    QList<FileManager::RecentFile> m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QString m_currentFile;

    QMainWindow *m_mainWindow;
    QFileSystemWatcher *m_fileWatcher; // Delayed creation.
    QFileSystemWatcher *m_linkWatcher; // Delayed creation (only UNIX/if a link is seen).
    bool m_blockActivated;
    QString m_lastVisitedDirectory;
    QString m_projectsDirectory;
    bool m_useProjectsDirectory;
    // When we are callling into a IFile
    // we don't want to receive a changed()
    // signal
    // That makes the code easier
    IFile *m_blockedIFile;
};

QFileSystemWatcher *FileManagerPrivate::fileWatcher()
{
    if (!m_fileWatcher) {
        m_fileWatcher= new QFileSystemWatcher(m_instance);
        QObject::connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
                         m_instance, SLOT(changedFile(QString)));
    }
    return m_fileWatcher;
}

QFileSystemWatcher *FileManagerPrivate::linkWatcher()
{
#ifdef Q_OS_UNIX
    if (!m_linkWatcher) {
        m_linkWatcher = new QFileSystemWatcher(m_instance);
        m_linkWatcher->setObjectName(QLatin1String("_qt_autotest_force_engine_poller"));
        QObject::connect(m_linkWatcher, SIGNAL(fileChanged(QString)),
                         m_instance, SLOT(changedFile(QString)));
    }
    return m_linkWatcher;
#else
    return fileWatcher();
#endif
}

FileManager *FileManagerPrivate::m_instance = 0;

FileManagerPrivate::FileManagerPrivate(FileManager *q, QMainWindow *mw) :
    m_mainWindow(mw),
    m_fileWatcher(0),
    m_linkWatcher(0),
    m_blockActivated(false),
    m_lastVisitedDirectory(QDir::currentPath()),
#ifdef Q_OS_MAC  // Creator is in bizarre places when launched via finder.
    m_useProjectsDirectory(true),
#else
    m_useProjectsDirectory(false),
#endif
    m_blockedIFile(0)
{
    m_instance = q;
}

} // namespace Internal

FileManager::FileManager(QMainWindow *mw)
  : QObject(mw),
    d(new Internal::FileManagerPrivate(this, mw))
{
    Core::ICore *core = Core::ICore::instance();
    connect(d->m_mainWindow, SIGNAL(windowActivated()),
        this, SLOT(mainWindowActivated()));
    connect(core, SIGNAL(contextChanged(Core::IContext*,Core::Context)),
        this, SLOT(syncWithEditor(Core::IContext*)));

    readSettings();
}

FileManager::~FileManager()
{
    delete d;
}

FileManager *FileManager::instance()
{
    return Internal::FileManagerPrivate::m_instance;
}

/*!
    \fn bool FileManager::addFiles(const QList<IFile *> &files, bool addWatcher)

    Adds a list of IFile's to the collection. If \a addWatcher is true (the default),
    the files are added to a file system watcher that notifies the file manager
    about file changes.
*/
void FileManager::addFiles(const QList<IFile *> &files, bool addWatcher)
{
    if (!addWatcher) {
        // We keep those in a separate list

        foreach (IFile *file, files) {
            if (file && !d->m_filesWithoutWatch.contains(file)) {
                connect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
                d->m_filesWithoutWatch.append(file);
            }
        }
        return;
    }

    foreach (IFile *file, files) {
        if (file && !d->m_filesWithWatch.contains(file)) {
            connect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
            connect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
            addFileInfo(file);
        }
    }
}

/* Adds the IFile's file and possibly it's final link target to both m_states
   (if it's file name is not empty), and the m_filesWithWatch list,
   and adds a file watcher for each if not already done.
   (The added file names are guaranteed to be absolute and cleaned.) */
void FileManager::addFileInfo(IFile *file)
{
    const QString fixedName = fixFileName(file->fileName(), KeepLinks);
    const QString fixedResolvedName = fixFileName(file->fileName(), ResolveLinks);
    addFileInfo(fixedResolvedName, file, false);
    if (fixedName != fixedResolvedName)
        addFileInfo(fixedName, file, true);
}

/* only called from addFileInfo(IFile *) */
void FileManager::addFileInfo(const QString &fileName, IFile *file, bool isLink)
{
    Internal::FileStateItem state;
    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        state.modified = fi.lastModified();
        state.permissions = fi.permissions();
        // Add watcher if we don't have that already
        if (!d->m_states.contains(fileName)) {
            d->m_states.insert(fileName, Internal::FileState());

        }
        QFileSystemWatcher *watcher = 0;
        if (isLink)
            watcher = d->linkWatcher();
        else
            watcher = d->fileWatcher();
        if (!watcher->files().contains(fileName))
            watcher->addPath(fileName);

        d->m_states[fileName].lastUpdatedState.insert(file, state);
    }
    d->m_filesWithWatch[file].append(fileName); // inserts a new QStringList if not already there
}

/// Dumps the state of the file manager's map
/// For debugging purposes
void FileManager::dump()
{
    qDebug() << "======== dumping state map";
    QMap<QString, Internal::FileState>::const_iterator it, end;
    it = d->m_states.constBegin();
    end = d->m_states.constEnd();
    for (; it != end; ++it) {
        qDebug() << it.key();
        qDebug() << "   expected:" << it.value().expected.modified;

        QMap<IFile *, Internal::FileStateItem>::const_iterator jt, jend;
        jt = it.value().lastUpdatedState.constBegin();
        jend = it.value().lastUpdatedState.constEnd();
        for (; jt != jend; ++jt) {
            qDebug() << "  " << jt.key()->fileName() << jt.value().modified;
        }
    }
    qDebug() << "------- dumping files with watch list";
    foreach (IFile *key, d->m_filesWithWatch.keys()) {
        qDebug() << key->fileName() << d->m_filesWithWatch.value(key);
    }
    qDebug() << "------- dumping watch list";
    if (d->m_fileWatcher)
        qDebug() << d->m_fileWatcher->files();
    qDebug() << "------- dumping link watch list";
    if (d->m_linkWatcher)
        qDebug() << d->m_linkWatcher->files();
}

/*!
    \fn void FileManager::renamedFile(const QString &from, const QString &to)
    \brief Tells the file manager that a file has been renamed on disk from within Qt Creator.

    Needs to be called right after the actual renaming on disk (i.e. before the file system
    watcher can report the event during the next event loop run). \a from needs to be an absolute file path.
    This will notify all IFile objects pointing to that file of the rename
    by calling IFile::rename, and update the cached time and permission
    information to avoid annoying the user with "file has been removed"
    popups.
*/
void FileManager::renamedFile(const QString &from, const QString &to)
{
    const QString &fixedFrom = fixFileName(from, KeepLinks);

    // gather the list of IFiles
    QList<IFile *> filesToRename;
    QMapIterator<IFile *, QStringList> it(d->m_filesWithWatch);
    while (it.hasNext()) {
        it.next();
        if (it.value().contains(fixedFrom))
            filesToRename.append(it.key());
    }

    // rename the IFiles
    foreach (IFile *file, filesToRename) {
        d->m_blockedIFile = file;
        removeFileInfo(file);
        file->rename(to);
        addFileInfo(file);
        d->m_blockedIFile = 0;
    }
}

/* Removes all occurrences of the IFile from m_filesWithWatch and m_states.
   If that results in a file no longer being referenced by any IFile, this
   also removes the file watcher.
*/
void FileManager::removeFileInfo(IFile *file)
{
    if (!d->m_filesWithWatch.contains(file))
        return;
    foreach (const QString &fileName, d->m_filesWithWatch.value(file)) {
        if (!d->m_states.contains(fileName))
            continue;
        d->m_states[fileName].lastUpdatedState.remove(file);
        if (d->m_states.value(fileName).lastUpdatedState.isEmpty()) {
            if (d->m_fileWatcher && d->m_fileWatcher->files().contains(fileName))
                d->m_fileWatcher->removePath(fileName);
            if (d->m_linkWatcher && d->m_linkWatcher->files().contains(fileName))
                d->m_linkWatcher->removePath(fileName);
            d->m_states.remove(fileName);
        }
    }
    d->m_filesWithWatch.remove(file);
}

/*!
    \fn bool FileManager::addFile(IFile *files, bool addWatcher)

    Adds a IFile object to the collection. If \a addWatcher is true (the default),
    the file is added to a file system watcher that notifies the file manager
    about file changes.
*/
void FileManager::addFile(IFile *file, bool addWatcher)
{
    addFiles(QList<IFile *>() << file, addWatcher);
}

void FileManager::fileDestroyed(QObject *obj)
{
    IFile *file = static_cast<IFile*>(obj);
    // Check the special unwatched first:
    if (!d->m_filesWithoutWatch.removeOne(file))
        removeFileInfo(file);
}

/*!
    \fn bool FileManager::removeFile(IFile *file)

    Removes a IFile object from the collection.

    Returns true if the file specified by \a file had the addWatcher argument to addFile() set.
*/
bool FileManager::removeFile(IFile *file)
{
    QTC_ASSERT(file, return false);

    bool addWatcher = false;
    // Special casing unwatched files
    if (!d->m_filesWithoutWatch.removeOne(file)) {
        addWatcher = true;
        removeFileInfo(file);
        disconnect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
    }
    disconnect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
    return addWatcher;
}

/* Slot reacting on IFile::changed. We need to check if the signal was sent
   because the file was saved under different name. */
void FileManager::checkForNewFileName()
{
    IFile *file = qobject_cast<IFile *>(sender());
    // We modified the IFile
    // Trust the other code to also update the m_states map
    if (file == d->m_blockedIFile)
        return;
    QTC_ASSERT(file, return);
    QTC_ASSERT(d->m_filesWithWatch.contains(file), return);

    // Maybe the name has changed or file has been deleted and created again ...
    // This also updates the state to the on disk state
    removeFileInfo(file);
    addFileInfo(file);
}

/*!
    \fn QString FileManager::fixFileName(const QString &fileName, FixMode fixmode)
    Returns a guaranteed cleaned path in native form. If the file exists,
    it will either be a cleaned absolute file path (fixmode == KeepLinks), or
    a cleaned canonical file path (fixmode == ResolveLinks).
*/
QString FileManager::fixFileName(const QString &fileName, FixMode fixmode)
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
#ifdef Q_OS_WIN
    s = s.toLower();
#endif
    return s;
}

/*!
    \fn QList<IFile*> FileManager::modifiedFiles() const

    Returns the list of IFile's that have been modified.
*/
QList<IFile *> FileManager::modifiedFiles() const
{
    QList<IFile *> modifiedFiles;

    foreach (IFile *file, d->m_filesWithWatch.keys()) {
        if (file->isModified())
            modifiedFiles << file;
    }

    foreach(IFile *file, d->m_filesWithoutWatch) {
        if (file->isModified())
            modifiedFiles << file;
    }

    return modifiedFiles;
}

/*!
    \fn void FileManager::expectFileChange(const QString &fileName)

    Any subsequent change to \a fileName is treated as a expected file change.

    \see FileManager::unexpectFileChange(const QString &fileName)
*/
void FileManager::expectFileChange(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    d->m_expectedFileNames.insert(fileName);
}

/*!
    \fn void FileManager::unexpectFileChange(const QString &fileName)

    Any change to \a fileName are unexpected again.

    \see FileManager::expectFileChange(const QString &fileName)
*/
void FileManager::unexpectFileChange(const QString &fileName)
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

/* only called from unblock and unexpect file change methods */
void FileManager::updateExpectedState(const QString &fileName)
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
    \fn QList<IFile*> FileManager::saveModifiedFilesSilently(const QList<IFile*> &files)

    Tries to save the files listed in \a files . Returns the files that could not be saved.
*/
QList<IFile *> FileManager::saveModifiedFilesSilently(const QList<IFile *> &files)
{
    return saveModifiedFiles(files, 0, true, QString());
}

/*!
    \fn QList<IFile*> FileManager::saveModifiedFiles(const QList<IFile *> &files, bool *cancelled, const QString &message, const QString &alwaysSaveMessage, bool *alwaysSave)

    Asks the user whether to save the files listed in \a files .
    Opens a dialog with the given \a message, and a additional
    text that should be used to ask if the user wants to enabled automatic save
    of modified files (in this context).
    The \a cancelled argument is set to true if the user cancelled the dialog,
    \a alwaysSave is set to match the selection of the user, if files should
    always automatically be saved.
    Returns the files that have not been saved.
*/
QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                              bool *cancelled, const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    return saveModifiedFiles(files, cancelled, false, message, alwaysSaveMessage, alwaysSave);
}

QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                              bool *cancelled,
                                              bool silently,
                                              const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    if (cancelled)
        (*cancelled) = false;

    QList<IFile *> notSaved;
    QMap<IFile *, QString> modifiedFilesMap;
    QList<IFile *> modifiedFiles;

    foreach (IFile *file, files) {
        if (file->isModified()) {
            QString name = file->fileName();
            if (name.isEmpty())
                name = file->suggestedFileName();

            // There can be several FileInterfaces pointing to the same file
            // Select one that is not readonly.
            if (!(modifiedFilesMap.key(name, 0)
                    && file->isReadOnly()))
                modifiedFilesMap.insert(file, name);
        }
    }
    modifiedFiles = modifiedFilesMap.keys();
    if (!modifiedFiles.isEmpty()) {
        QList<IFile *> filesToSave;
        if (silently) {
            filesToSave = modifiedFiles;
        } else {
            Internal::SaveItemsDialog dia(d->m_mainWindow, modifiedFiles);
            if (!message.isEmpty())
                dia.setMessage(message);
            if (!alwaysSaveMessage.isNull())
                dia.setAlwaysSaveMessage(alwaysSaveMessage);
            if (dia.exec() != QDialog::Accepted) {
                if (cancelled)
                    (*cancelled) = true;
                if (alwaysSave)
                    *alwaysSave = dia.alwaysSaveChecked();
                notSaved = modifiedFiles;
                return notSaved;
            }
            if (alwaysSave)
                *alwaysSave = dia.alwaysSaveChecked();
            filesToSave = dia.itemsToSave();
        }

        foreach (IFile *file, filesToSave) {
            if (!EditorManager::instance()->saveFile(file)) {
                if (cancelled)
                    *cancelled = true;
                notSaved.append(file);
            }
        }
    }
    return notSaved;
}

bool FileManager::saveFile(IFile *file, const QString &fileName, bool *isReadOnly)
{
    bool ret = true;
    QString effName = fileName.isEmpty() ? file->fileName() : fileName;
    expectFileChange(effName); // This only matters to other IFiles which refer to this file
    bool addWatcher = removeFile(file); // So that our own IFile gets no notification at all

    QString errorString;
    if (!file->save(&errorString, fileName, false)) {
        if (isReadOnly) {
            QFile ofi(effName);
            // Check whether the existing file is writable
            if (!ofi.open(QIODevice::ReadWrite) && ofi.open(QIODevice::ReadOnly)) {
                *isReadOnly = true;
                goto out;
            }
            *isReadOnly = false;
        }
        QMessageBox::critical(d->m_mainWindow, tr("File Error"), errorString);
      out:
        ret = false;
    }

    addFile(file, addWatcher);
    unexpectFileChange(effName);
    return ret;
}

QString FileManager::getSaveFileName(const QString &title, const QString &pathIn,
                                     const QString &filter, QString *selectedFilter)
{
    const QString &path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
    QString fileName;
    bool repeat;
    do {
        repeat = false;
        fileName = QFileDialog::getSaveFileName(
            d->m_mainWindow, title, path, filter, selectedFilter, QFileDialog::DontConfirmOverwrite);
        if (!fileName.isEmpty()) {
            // If the selected filter is All Files (*) we leave the name exactly as the user
            // specified. Otherwise the suffix must be one available in the selected filter. If
            // the name already ends with such suffix nothing needs to be done. But if not, the
            // first one from the filter is appended.
            if (selectedFilter && *selectedFilter != QCoreApplication::translate(
                    "Core", Constants::ALL_FILES_FILTER)) {
                // Mime database creates filter strings like this: Anything here (*.foo *.bar)
                QRegExp regExp(".*\\s+\\((.*)\\)$");
                const int index = regExp.lastIndexIn(*selectedFilter);
                bool suffixOk = false;
                if (index != -1) {
                    const QStringList &suffixes = regExp.cap(1).remove('*').split(' ');
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
                if (QMessageBox::warning(d->m_mainWindow, tr("Overwrite?"),
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

QString FileManager::getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                                  const QString &filter)
{
    QString selected = filter;
    return getSaveFileName(title, pathIn, filter, &selected);
}

/*!
    \fn QString FileManager::getSaveAsFileName(IFile *file, const QString &filter, QString *selectedFilter)

    Asks the user for a new file name (Save File As) for /arg file.
*/
QString FileManager::getSaveAsFileName(IFile *file, const QString &filter, QString *selectedFilter)
{
    if (!file)
        return QLatin1String("");
    QString absoluteFilePath = file->fileName();
    const QFileInfo fi(absoluteFilePath);
    QString fileName = fi.fileName();
    QString path = fi.absolutePath();
    if (absoluteFilePath.isEmpty()) {
        fileName = file->suggestedFileName();
        const QString defaultPath = file->defaultPath();
        if (!defaultPath.isEmpty())
            path = defaultPath;
    }

    QString filterString;
    if (filter.isEmpty()) {
        if (const MimeType &mt = Core::ICore::instance()->mimeDatabase()->findByFile(fi))
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
    \fn QStringList FileManager::getOpenFileNames(const QString &filters,
                                                  const QString pathIn,
                                                  QString *selectedFilter)

    Asks the user for a set of file names to be opened. The \a filters
    and \a selectedFilter parameters is interpreted like in
    QFileDialog::getOpenFileNames(), \a pathIn specifies a path to open the dialog
    in, if that is not overridden by the users policy.
*/

QStringList FileManager::getOpenFileNames(const QString &filters,
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
    const QStringList files = QFileDialog::getOpenFileNames(d->m_mainWindow,
                                                      tr("Open File"),
                                                      path, filters,
                                                      selectedFilter);
    if (!files.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(files.front()).absolutePath());
    return files;
}

FileManager::ReadOnlyAction
    FileManager::promptReadOnlyFile(const QString &fileName,
                                      const IVersionControl *versionControl,
                                      QWidget *parent,
                                      bool displaySaveAsButton)
{
    // Version Control: If automatic open is desired, open right away.
    bool promptVCS = false;
    if (versionControl && versionControl->supportsOperation(IVersionControl::OpenOperation)) {
        if (versionControl->settingsFlags() & IVersionControl::AutoOpen)
            return RO_OpenVCS;
        promptVCS = true;
    }

    // Create message box.
    QMessageBox msgBox(QMessageBox::Question, tr("File is Read Only"),
                       tr("The file <i>%1</i> is read only.").arg(QDir::toNativeSeparators(fileName)),
                       QMessageBox::Cancel, parent);

    QPushButton *vcsButton = 0;
    if (promptVCS)
        vcsButton = msgBox.addButton(tr("Open with VCS (%1)").arg(versionControl->displayName()), QMessageBox::AcceptRole);

    QPushButton *makeWritableButton =  msgBox.addButton(tr("Make writable"), QMessageBox::AcceptRole);

    QPushButton *saveAsButton = 0;
    if (displaySaveAsButton)
        saveAsButton = msgBox.addButton(tr("Save as..."), QMessageBox::ActionRole);

    msgBox.setDefaultButton(vcsButton ? vcsButton : makeWritableButton);
    msgBox.exec();

    QAbstractButton *clickedButton = msgBox.clickedButton();
    if (clickedButton == vcsButton)
        return RO_OpenVCS;
    if (clickedButton == makeWritableButton)
        return RO_MakeWriteable;
    if (displaySaveAsButton && clickedButton == saveAsButton)
        return RO_SaveAs;
    return  RO_Cancel;
}

void FileManager::changedFile(const QString &fileName)
{
    const bool wasempty = d->m_changedFiles.isEmpty();

    if (d->m_states.contains(fileName))
        d->m_changedFiles.insert(fileName);

    if (wasempty && !d->m_changedFiles.isEmpty()) {
        QTimer::singleShot(200, this, SLOT(checkForReload()));
    }
}

void FileManager::mainWindowActivated()
{
    //we need to do this asynchronously because
    //opening a dialog ("Reload?") in a windowactivated event
    //freezes on Mac
    QTimer::singleShot(0, this, SLOT(checkForReload()));
}

void FileManager::checkForReload()
{
    if (d->m_changedFiles.isEmpty())
        return;
    if (QApplication::activeWindow() != d->m_mainWindow)
        return;

    if (d->m_blockActivated)
        return;

    d->m_blockActivated = true;

    IFile::ReloadSetting defaultBehavior = EditorManager::instance()->reloadSetting();
    Utils::ReloadPromptAnswer previousAnswer = Utils::ReloadCurrent;

    QList<IEditor*> editorsToClose;
    QMap<IFile*, QString> filesToSave;

    // collect file information
    QMap<QString, Internal::FileStateItem> currentStates;
    QMap<QString, IFile::ChangeType> changeTypes;
    QSet<IFile *> changedIFiles;
    foreach (const QString &fileName, d->m_changedFiles) {
        IFile::ChangeType type = IFile::TypeContents;
        Internal::FileStateItem state;
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            type = IFile::TypeRemoved;
        } else {
            state.modified = fi.lastModified();
            state.permissions = fi.permissions();
        }
        currentStates.insert(fileName, state);
        changeTypes.insert(fileName, type);
        foreach (IFile *file, d->m_states.value(fileName).lastUpdatedState.keys())
            changedIFiles.insert(file);
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

    // handle the IFiles
    QStringList errorStrings;
    foreach (IFile *file, changedIFiles) {
        IFile::ChangeTrigger trigger = IFile::TriggerInternal;
        IFile::ChangeType type = IFile::TypePermissions;
        bool changed = false;
        // find out the type & behavior from the two possible files
        // behavior is internal if all changes are expected (and none removed)
        // type is "max" of both types (remove > contents > permissions)
        foreach (const QString & fileName, d->m_filesWithWatch.value(file)) {
            // was the file reported?
            if (!currentStates.contains(fileName))
                continue;

            Internal::FileStateItem currentState = currentStates.value(fileName);
            Internal::FileStateItem expectedState = d->m_states.value(fileName).expected;
            Internal::FileStateItem lastState = d->m_states.value(fileName).lastUpdatedState.value(file);

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
                trigger = IFile::TriggerExternal;
            }

            // find out the type
            IFile::ChangeType fileChange = changeTypes.value(fileName);
            if (fileChange == IFile::TypeRemoved) {
                type = IFile::TypeRemoved;
            } else if (fileChange == IFile::TypeContents && type == IFile::TypePermissions) {
                type = IFile::TypeContents;
            }
        }

        if (!changed) // probably because the change was blocked with (un)blockFileChange
            continue;

        // handle it!
        d->m_blockedIFile = file;

        bool success = true;
        QString errorString;
        // we've got some modification
        // check if it's contents or permissions:
        if (type == IFile::TypePermissions) {
            // Only permission change
            success = file->reload(&errorString, IFile::FlagReload, IFile::TypePermissions);
        // now we know it's a content change or file was removed
        } else if (defaultBehavior == IFile::ReloadUnmodified
                   && type == IFile::TypeContents && !file->isModified()) {
            // content change, but unmodified (and settings say to reload in this case)
            success = file->reload(&errorString, IFile::FlagReload, type);
        // file was removed or it's a content change and the default behavior for
        // unmodified files didn't kick in
        } else if (defaultBehavior == IFile::ReloadUnmodified
                   && type == IFile::TypeRemoved && !file->isModified()) {
            // file removed, but unmodified files should be reloaded
            // so we close the file
            editorsToClose << EditorManager::instance()->editorsForFile(file);
        } else if (defaultBehavior == IFile::IgnoreAll) {
            // content change or removed, but settings say ignore
            success = file->reload(&errorString, IFile::FlagIgnore, type);
        // either the default behavior is to always ask,
        // or the ReloadUnmodified default behavior didn't kick in,
        // so do whatever the IFile wants us to do
        } else {
            // check if IFile wants us to ask
            if (file->reloadBehavior(trigger, type) == IFile::BehaviorSilent) {
                // content change or removed, IFile wants silent handling
                success = file->reload(&errorString, IFile::FlagReload, type);
            // IFile wants us to ask
            } else if (type == IFile::TypeContents) {
                // content change, IFile wants to ask user
                if (previousAnswer == Utils::ReloadNone) {
                    // answer already given, ignore
                    success = file->reload(&errorString, IFile::FlagIgnore, IFile::TypeContents);
                } else if (previousAnswer == Utils::ReloadAll) {
                    // answer already given, reload
                    success = file->reload(&errorString, IFile::FlagReload, IFile::TypeContents);
                } else {
                    // Ask about content change
                    previousAnswer = Utils::reloadPrompt(file->fileName(), file->isModified(), QApplication::activeWindow());
                    switch (previousAnswer) {
                    case Utils::ReloadAll:
                    case Utils::ReloadCurrent:
                        success = file->reload(&errorString, IFile::FlagReload, IFile::TypeContents);
                        break;
                    case Utils::ReloadSkipCurrent:
                    case Utils::ReloadNone:
                        success = file->reload(&errorString, IFile::FlagIgnore, IFile::TypeContents);
                        break;
                    }
                }
            // IFile wants us to ask, and it's the TypeRemoved case
            } else {
                // Ask about removed file
                bool unhandled = true;
                while (unhandled) {
                    switch (Utils::fileDeletedPrompt(file->fileName(), trigger == IFile::TriggerExternal, QApplication::activeWindow())) {
                    case Utils::FileDeletedSave:
                        filesToSave.insert(file, file->fileName());
                        unhandled = false;
                        break;
                    case Utils::FileDeletedSaveAs:
                    {
                        const QString &saveFileName = getSaveAsFileName(file);
                        if (!saveFileName.isEmpty()) {
                            filesToSave.insert(file, saveFileName);
                            unhandled = false;
                        }
                        break;
                    }
                    case Utils::FileDeletedClose:
                        editorsToClose << EditorManager::instance()->editorsForFile(file);
                        unhandled = false;
                        break;
                    }
                }
            }
        }
        if (!success) {
            if (errorString.isEmpty())
                errorStrings << tr("Cannot reload %1").arg(QDir::toNativeSeparators(file->fileName()));
            else
                errorStrings << errorString;
        }

        // update file info, also handling if e.g. link target has changed
        removeFileInfo(file);
        addFileInfo(file);
        d->m_blockedIFile = 0;
    }
    if (!errorStrings.isEmpty())
        QMessageBox::critical(d->m_mainWindow, tr("File Error"),
                              errorStrings.join(QLatin1String("\n")));

    // handle deleted files
    EditorManager::instance()->closeEditors(editorsToClose, false);
    QMapIterator<IFile *, QString> it(filesToSave);
    while (it.hasNext()) {
        it.next();
        saveFile(it.key(), it.value());
        it.key()->checkPermissions();
    }

    d->m_blockActivated = false;

//    dump();
}

void FileManager::syncWithEditor(Core::IContext *context)
{
    if (!context)
        return;

    Core::IEditor *editor = Core::EditorManager::instance()->currentEditor();
    if (editor && (editor->widget() == context->widget()) &&
        !editor->isTemporary())
        setCurrentFile(editor->file()->fileName());
}

/*!
    \fn void FileManager::addToRecentFiles(const QString &fileName, const QString &editorId)

    Adds the \a fileName to the list of recent files. Associates the file to
    be reopened with an editor of the given \a editorId, if possible.
    \a editorId defaults to the empty id, which means to let the system figure out
    the best editor itself.
*/
void FileManager::addToRecentFiles(const QString &fileName, const QString &editorId)
{
    if (fileName.isEmpty())
        return;
    QString unifiedForm(fixFileName(fileName, KeepLinks));
    QMutableListIterator<RecentFile > it(d->m_recentFiles);
    while (it.hasNext()) {
        RecentFile file = it.next();
        QString recentUnifiedForm(fixFileName(file.first, KeepLinks));
        if (unifiedForm == recentUnifiedForm)
            it.remove();
    }
    if (d->m_recentFiles.count() > d->m_maxRecentFiles)
        d->m_recentFiles.removeLast();
    d->m_recentFiles.prepend(RecentFile(fileName, editorId));
}

/*!
    \fn void FileManager::clearRecentFiles()

    Clears the list of recent files. Should only be called by
    the core plugin when the user chooses to clear it.
*/
void FileManager::clearRecentFiles()
{
    d->m_recentFiles.clear();
}

/*!
    \fn QStringList FileManager::recentFiles() const

    Returns the list of recent files.
*/
QList<FileManager::RecentFile> FileManager::recentFiles() const
{
    return d->m_recentFiles;
}

void FileManager::saveSettings()
{
    QStringList recentFiles;
    QStringList recentEditorIds;
    foreach (const RecentFile &file, d->m_recentFiles) {
        recentFiles.append(file.first);
        recentEditorIds.append(file.second);
    }

    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(filesKeyC), recentFiles);
    s->setValue(QLatin1String(editorsKeyC), recentEditorIds);
    s->endGroup();
    s->beginGroup(QLatin1String(directoryGroupC));
    s->setValue(QLatin1String(projectDirectoryKeyC), d->m_projectsDirectory);
    s->setValue(QLatin1String(useProjectDirectoryKeyC), d->m_useProjectsDirectory);
    s->endGroup();
}

void FileManager::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
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
            d->m_recentFiles.append(RecentFile(QDir::fromNativeSeparators(fileName), // from native to guard against old settings
                                               editorId));
    }

    s->beginGroup(QLatin1String(directoryGroupC));
    const QString settingsProjectDir = s->value(QLatin1String(projectDirectoryKeyC),
                                                QString()).toString();
    if (!settingsProjectDir.isEmpty() && QFileInfo(settingsProjectDir).isDir()) {
        d->m_projectsDirectory = settingsProjectDir;
    } else {
        d->m_projectsDirectory = Utils::PathChooser::homePath();
    }
    d->m_useProjectsDirectory = s->value(QLatin1String(useProjectDirectoryKeyC),
                                         d->m_useProjectsDirectory).toBool();
    s->endGroup();
}

/*!

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \sa currentFile
  */
void FileManager::setCurrentFile(const QString &filePath)
{
    if (d->m_currentFile == filePath)
        return;
    d->m_currentFile = filePath;
    emit currentFileChanged(d->m_currentFile);
}

/*!
  Returns the absolute path of the current file

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \sa setCurrentFile
  */
QString FileManager::currentFile() const
{
    return d->m_currentFile;
}

/*!

  Returns the initial directory for a new file dialog. If there is
  a current file, use that, else use last visited directory.

  \sa setFileDialogLastVisitedDirectory
*/

QString FileManager::fileDialogInitialDirectory() const
{
    if (!d->m_currentFile.isEmpty())
        return QFileInfo(d->m_currentFile).absolutePath();
    return d->m_lastVisitedDirectory;
}

/*!

  Returns the directory for projects. Defaults to HOME.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

QString FileManager::projectsDirectory() const
{
    return d->m_projectsDirectory;
}

/*!

  Set the directory for projects.

  \sa projectsDirectory, useProjectsDirectory
*/

void FileManager::setProjectsDirectory(const QString &dir)
{
    d->m_projectsDirectory = dir;
}

/*!

  Returns whether the directory for projects is to be
  used or the user wants the current directory.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

bool FileManager::useProjectsDirectory() const
{
    return d->m_useProjectsDirectory;
}

/*!

  Sets whether the directory for projects is to be used.

  \sa projectsDirectory, useProjectsDirectory
*/

void FileManager::setUseProjectsDirectory(bool useProjectsDirectory)
{
    d->m_useProjectsDirectory = useProjectsDirectory;
}

/*!

  Returns last visited directory of a file dialog.

  \sa setFileDialogLastVisitedDirectory, fileDialogInitialDirectory

*/

QString FileManager::fileDialogLastVisitedDirectory() const
{
    return d->m_lastVisitedDirectory;
}

/*!

  Set the last visited directory of a file dialog that will be remembered
  for the next one.

  \sa fileDialogLastVisitedDirectory, fileDialogInitialDirectory

  */

void FileManager::setFileDialogLastVisitedDirectory(const QString &directory)
{
    d->m_lastVisitedDirectory = directory;
}

void FileManager::notifyFilesChangedInternally(const QStringList &files)
{
    emit filesChangedInternally(files);
}

// -------------- FileChangeBlocker

FileChangeBlocker::FileChangeBlocker(const QString &fileName)
    : m_fileName(fileName)
{
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    fm->expectFileChange(fileName);
}

FileChangeBlocker::~FileChangeBlocker()
{
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    fm->unexpectFileChange(m_fileName);
}

} // namespace Core
