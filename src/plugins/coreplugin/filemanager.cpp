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

#include "filemanager.h"

#include "editormanager.h"
#include "ieditor.h"
#include "icore.h"
#include "ifile.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "saveitemsdialog.h"
#include "vcsmanager.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QDebug>
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

/*!
  \class FileManager
  \mainclass
  \inheaderfile filemanager.h
  \brief Manages a set of IFile objects.

  The FileManager service monitors a set of IFile's. Plugins should register
  files they work with at the service. The files the IFile's point to will be
  monitored at filesystem level. If a file changes, the status of the IFile's
  will be adjusted accordingly. Furthermore, on application exit the user will
  be asked to save all modified files.

  Different IFile objects in the set can point to the same file in the
  filesystem. The monitoring of a file can be blocked by blockFileChange(), and
  enabled again by unblockFileChange().

  The FileManager service also provides two convenience methods for saving
  files: saveModifiedFiles() and saveModifiedFilesSilently(). Both take a list
  of FileInterfaces as an argument, and return the list of files which were
  _not_ saved.

  The service also manages the list of recent files to be shown to the user
  (see addToRecentFiles() and recentFiles()).
 */

static const char settingsGroupC[] = "RecentFiles";
static const char filesKeyC[] = "Files";

static const char directoryGroupC[] = "Directories";
static const char projectDirectoryKeyC[] = "Projects";
static const char useProjectDirectoryKeyC[] = "UseProjectsDirectory";

namespace Core {
namespace Internal {

struct FileInfo
{
    QString fileName;
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileManagerPrivate {
    explicit FileManagerPrivate(QObject *q, QMainWindow *mw);

    QMap<IFile*, FileInfo> m_managedFiles;

    QStringList m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QString m_currentFile;

    QMainWindow *m_mainWindow;
    QFileSystemWatcher *m_fileWatcher;
    QList<QPointer<IFile> > m_changedFiles;
    bool m_blockActivated;
    QString m_lastVisitedDirectory;
    QString m_projectsDirectory;
    bool m_useProjectsDirectory;
};

FileManagerPrivate::FileManagerPrivate(QObject *q, QMainWindow *mw) :
    m_mainWindow(mw),
    m_fileWatcher(new QFileSystemWatcher(q)),
    m_blockActivated(false),
    m_lastVisitedDirectory(QDir::currentPath()),
    m_projectsDirectory(Utils::PathChooser::homePath()),
#ifdef Q_OS_MAC  // Creator is in bizarre places when launched via finder.
    m_useProjectsDirectory(true)
#else
    m_useProjectsDirectory(false)
#endif
{
}

} // namespace Internal

FileManager::FileManager(QMainWindow *mw)
  : QObject(mw),
    d(new Internal::FileManagerPrivate(this, mw))
{
    Core::ICore *core = Core::ICore::instance();
    connect(d->m_fileWatcher, SIGNAL(fileChanged(QString)),
        this, SLOT(changedFile(QString)));
    connect(d->m_mainWindow, SIGNAL(windowActivated()),
        this, SLOT(mainWindowActivated()));
    connect(core, SIGNAL(contextChanged(Core::IContext*)),
        this, SLOT(syncWithEditor(Core::IContext*)));

    const QSettings *s = core->settings();
    d->m_recentFiles = s->value(QLatin1String(settingsGroupC) + QLatin1Char('/') + QLatin1String(filesKeyC), QStringList()).toStringList();
    for (QStringList::iterator it = d->m_recentFiles.begin(); it != d->m_recentFiles.end(); ) {
        if (QFileInfo(*it).isFile()) {
            ++it;
        } else {
            it = d->m_recentFiles.erase(it);
        }
    }
    const QString directoryGroup = QLatin1String(directoryGroupC) + QLatin1Char('/');
    d->m_projectsDirectory = s->value(directoryGroup + QLatin1String(projectDirectoryKeyC), QString()).toString();
    d->m_useProjectsDirectory = s->value(directoryGroup + QLatin1String(useProjectDirectoryKeyC),
                                         d->m_useProjectsDirectory).toBool();
}

FileManager::~FileManager()
{
    delete d;
}

/*!
    \fn bool FileManager::addFiles(const QList<IFile *> &files)

    Adds a list of IFile's to the collection.

    Returns true if the file specified by \a files have not been yet part of the file list.
*/
bool FileManager::addFiles(const QList<IFile *> &files)
{
    bool filesAdded = false;
    foreach (IFile *file, files) {
        if (!file || d->m_managedFiles.contains(file))
            continue;
        connect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
        connect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
        filesAdded = true;
        addWatch(fixFileName(file->fileName()));
        updateFileInfo(file);
    }
    return filesAdded;
}

/*!
    \fn bool FileManager::addFile(IFile *files)

    Adds a IFile object to the collection.

    Returns true if the file specified by \a file has not been yet part of the file list.
*/
bool FileManager::addFile(IFile *file)
{
    return addFiles(QList<IFile *>() << file);
}

void FileManager::fileDestroyed(QObject *obj)
{
    // we can't use qobject_cast here, because meta data is already destroyed
    IFile *file = static_cast<IFile*>(obj);
    const QString filename = d->m_managedFiles.value(file).fileName;
    d->m_managedFiles.remove(file);
    removeWatch(filename);
}

/*!
    \fn bool FileManager::removeFile(IFile *file)

    Removes a IFile object from the collection.

    Returns true if the file specified by \a file has been part of the file list.
*/
bool FileManager::removeFile(IFile *file)
{
    if (!file)
        return false;

    disconnect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
    disconnect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));

    if (!d->m_managedFiles.contains(file))
        return false;
    const Internal::FileInfo info = d->m_managedFiles.take(file);
    const QString filename = info.fileName;
    removeWatch(filename);
    return true;
}

void FileManager::addWatch(const QString &filename)
{
    if (!filename.isEmpty() && managedFiles(filename).isEmpty())
        d->m_fileWatcher->addPath(filename);
}

void FileManager::removeWatch(const QString &filename)
{
    if (!filename.isEmpty() && managedFiles(filename).isEmpty())
        d->m_fileWatcher->removePath(filename);
}

void FileManager::checkForNewFileName()
{
    IFile *file = qobject_cast<IFile *>(sender());
    QTC_ASSERT(file, return);
    const QString newfilename = fixFileName(file->fileName());
    const QString oldfilename = d->m_managedFiles.value(file).fileName;
    if (!newfilename.isEmpty() && newfilename != oldfilename) {
        d->m_managedFiles[file].fileName = newfilename;
        removeWatch(oldfilename);
        addWatch(newfilename);
    }
}

// TODO Rename to nativeFileName
QString FileManager::fixFileName(const QString &fileName)
{
    QString s = fileName;
#ifdef Q_OS_WIN
    s = s.toLower();
#endif
    if (!QFile::exists(s))
        return QDir::toNativeSeparators(s);
    return QFileInfo(QDir::toNativeSeparators(s)).canonicalFilePath();
}

/*!
    \fn bool FileManager::isFileManaged(const QString  &fileName) const

    Returns true if at least one IFile in the set points to \a fileName.
*/
bool FileManager::isFileManaged(const QString &fileName) const
{
    if (fileName.isEmpty())
        return false;

    return !managedFiles(fixFileName(fileName)).isEmpty();
}

/*!
    \fn QList<IFile*> FileManager::modifiedFiles() const

    Returns the list of IFile's that have been modified.
*/
QList<IFile *> FileManager::modifiedFiles() const
{
    QList<IFile *> modifiedFiles;

    const QMap<IFile*, Internal::FileInfo>::const_iterator cend =  d->m_managedFiles.constEnd();
    for (QMap<IFile*, Internal::FileInfo>::const_iterator i = d->m_managedFiles.constBegin(); i != cend; ++i) {
        IFile *fi = i.key();
        if (fi->isModified())
            modifiedFiles << fi;
    }
    return modifiedFiles;
}

/*!
    \fn void FileManager::blockFileChange(IFile *file)

    Blocks the monitoring of the file the \a file argument points to.
*/
void FileManager::blockFileChange(IFile *file)
{
    if (!file->fileName().isEmpty())
        d->m_fileWatcher->removePath(file->fileName());
}

/*!
    \fn void FileManager::unblockFileChange(IFile *file)

    Enables the monitoring of the file the \a file argument points to, and update the status of the corresponding IFile's.
*/
void FileManager::unblockFileChange(IFile *file)
{
    foreach (IFile *managedFile, managedFiles(file->fileName()))
        updateFileInfo(managedFile);
    if (!file->fileName().isEmpty())
        d->m_fileWatcher->addPath(file->fileName());
}

void FileManager::updateFileInfo(IFile *file)
{
    const QString fixedname = fixFileName(file->fileName());
    const QFileInfo fi(file->fileName());
    Internal::FileInfo info;
    info.fileName = fixedname;
    info.modified = fi.lastModified();
    info.permissions = fi.permissions();
    d->m_managedFiles.insert(file, info);
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
    \fn QList<IFile*> FileManager::saveModifiedFiles(const QList<IFile*> &files, bool *cancelled, const QString &message)

    Asks the user whether to save the files listed in \a files . Returns the files that have not been saved.
*/
QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                              bool *cancelled, const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    return saveModifiedFiles(files, cancelled, false, message, alwaysSaveMessage, alwaysSave);
}

static QMessageBox::StandardButton skipFailedPrompt(QWidget *parent, const QString &fileName)
{
    return QMessageBox::question(parent,
                                 FileManager::tr("Cannot save file"),
                                 FileManager::tr("Cannot save changes to '%1'. Do you want to continue and lose your changes?").arg(fileName),
                                 QMessageBox::YesToAll| QMessageBox::Yes|QMessageBox::No,
                                 QMessageBox::No);
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
            if (!(modifiedFilesMap.values().contains(name)
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

        bool yestoall = false;
        Core::VCSManager *vcsManager = Core::ICore::instance()->vcsManager();
        foreach (IFile *file, filesToSave) {
            if (file->isReadOnly()) {
                const QString directory = QFileInfo(file->fileName()).absolutePath();
                if (IVersionControl *versionControl = vcsManager->findVersionControlForDirectory(directory))
                    versionControl->vcsOpen(file->fileName());
            }
            if (!file->isReadOnly() && !file->fileName().isEmpty()) {
                blockFileChange(file);
                const bool ok = file->save();
                unblockFileChange(file);
                if (!ok)
                    notSaved.append(file);
            } else if (QFile::exists(file->fileName()) && !file->isSaveAsAllowed()) {
                if (yestoall)
                    continue;
                const QFileInfo fi(file->fileName());
                switch (skipFailedPrompt(d->m_mainWindow, fi.fileName())) {
                case QMessageBox::YesToAll:
                    yestoall = true;
                    break;
                case QMessageBox::No:
                    if (cancelled)
                        *cancelled = true;
                    return filesToSave;
                default:
                    break;
                }
            } else {
                QString fileName = getSaveAsFileName(file);
                bool ok = false;
                if (!fileName.isEmpty()) {
                    blockFileChange(file);
                    ok = file->save(fileName);
                    file->checkPermissions();
                    unblockFileChange(file);
                }
                if (!ok)
                    notSaved.append(file);
            }
        }
    }
    return notSaved;
}

QString FileManager::getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
    const QString &fileFilter, const QString &extension)
{
    QString fileName;
    bool repeat;
    do {
        repeat = false;
        const QString path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
        fileName = QFileDialog::getSaveFileName(d->m_mainWindow, title, path, fileFilter);
        if (!fileName.isEmpty() && !extension.isEmpty() && !fileName.endsWith(extension)) {
            fileName.append(extension);
            if (QFile::exists(fileName)) {
                if (QMessageBox::warning(d->m_mainWindow, tr("Overwrite?"),
                        tr("An item named '%1' already exists at this location. Do you want to overwrite it?").arg(fileName),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                    repeat = true;
            }
        }
    } while (repeat);
    if (!fileName.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(fileName).absolutePath());
    return fileName;
}

/*!
    \fn QString FileManager::getSaveAsFileName(IFile *file)

    Asks the user for a new file name (Save File As) for /arg file.
*/
QString FileManager::getSaveAsFileName(IFile *file)
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
    QString preferredSuffix;
    if (const MimeType mt = Core::ICore::instance()->mimeDatabase()->findByFile(fi)) {
        filterString = mt.filterString();
        preferredSuffix = mt.preferredSuffix();
    }

    absoluteFilePath = getSaveFileNameWithExtension(tr("Save File As"),
        path + QDir::separator() + fileName,
        filterString,
        preferredSuffix);
    return absoluteFilePath;
}

/*!
    \fn QString FileManager::getOpenFileNames(const QStringList &filters, QString *selectedFilter) const

    Asks the user for a set of file names to be opened.
*/

QStringList FileManager::getOpenFileNames(const QString &filters,
                                          const QString pathIn,
                                          QString *selectedFilter)
{
    const QString path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
    const QStringList files = QFileDialog::getOpenFileNames(d->m_mainWindow,
                                                      tr("Open File"),
                                                      path, filters,
                                                      selectedFilter);
    if (!files.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(files.front()).absolutePath());
    return files;
}


void FileManager::changedFile(const QString &file)
{
    const bool wasempty = d->m_changedFiles.isEmpty();
    foreach (IFile *fileinterface, managedFiles(file))
        d->m_changedFiles << fileinterface;
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
    if (QApplication::activeWindow() == d->m_mainWindow &&
        !d->m_blockActivated && !d->m_changedFiles.isEmpty()) {
        d->m_blockActivated = true;
        const QList<QPointer<IFile> > changed = d->m_changedFiles;
        d->m_changedFiles.clear();
        IFile::ReloadBehavior behavior = EditorManager::instance()->reloadBehavior();
        foreach (IFile *f, changed) {
            if (!f)
                continue;
            QFileInfo fi(f->fileName());
            Internal::FileInfo info = d->m_managedFiles.value(f);
            if (info.modified != fi.lastModified()
                    || info.permissions != fi.permissions()) {
                if (info.modified != fi.lastModified())
                    f->modified(&behavior);
                else {
                    IFile::ReloadBehavior tempBeh =
                        IFile::ReloadPermissions;
                    f->modified(&tempBeh);
                }
                updateFileInfo(f);

                // the file system watchers loses inodes when a file is removed/renamed. Work around it.
                d->m_fileWatcher->removePath(f->fileName());
                d->m_fileWatcher->addPath(f->fileName());
            }
        }
        d->m_blockActivated = false;
        checkForReload();
    }
}

void FileManager::syncWithEditor(Core::IContext *context)
{
    if (!context)
        return;

    Core::IEditor *editor = Core::EditorManager::instance()->currentEditor();
    if (editor && (editor->widget() == context->widget()))
        setCurrentFile(editor->file()->fileName());
}

/*!
    \fn void FileManager::addToRecentFiles(const QString &fileName)

    Adds the \a fileName to the list of recent files.
*/
void FileManager::addToRecentFiles(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    QString prettyFileName(QDir::toNativeSeparators(fileName));
    d->m_recentFiles.removeAll(prettyFileName);
    if (d->m_recentFiles.count() > d->m_maxRecentFiles)
        d->m_recentFiles.removeLast();
    d->m_recentFiles.prepend(prettyFileName);
}

/*!
    \fn QStringList FileManager::recentFiles() const

    Returns the list of recent files.
*/
QStringList FileManager::recentFiles() const
{
    return d->m_recentFiles;
}

void FileManager::saveRecentFiles()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(filesKeyC), d->m_recentFiles);
    s->endGroup();
    s->beginGroup(QLatin1String(directoryGroupC));
    s->setValue(QLatin1String(projectDirectoryKeyC), d->m_projectsDirectory);
    s->setValue(QLatin1String(useProjectDirectoryKeyC), d->m_useProjectsDirectory);
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
    \fn QList<IFile*> FileManager::managedFiles(const QString &fileName) const

    Returns the list one IFile's in the set that point to \a fileName.
*/
QList<IFile *> FileManager::managedFiles(const QString &fileName) const
{
    const QString fixedName = fixFileName(fileName);
    QList<IFile *> result;
    if (!fixedName.isEmpty()) {
        const QMap<IFile*, Internal::FileInfo>::const_iterator cend =  d->m_managedFiles.constEnd();
        for (QMap<IFile*, Internal::FileInfo>::const_iterator i = d->m_managedFiles.constBegin(); i != cend; ++i) {
            if (i.value().fileName == fixedName)
                result << i.key();
        }
    }
    return result;
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

// -------------- FileChangeBlocker

FileChangeBlocker::FileChangeBlocker(const QString &fileName)
    : m_reload(false)
{
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    m_files = fm->managedFiles(fileName);
    foreach (Core::IFile *file, m_files)
        fm->blockFileChange(file);
}

FileChangeBlocker::~FileChangeBlocker()
{
    Core::IFile::ReloadBehavior tempBehavior = Core::IFile::ReloadAll;
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    foreach (Core::IFile *file, m_files) {
        if (m_reload)
            file->modified(&tempBehavior);
        fm->unblockFileChange(file);
    }
}

void FileChangeBlocker::setModifiedReload(bool b)
{
    m_reload = b;
}

bool FileChangeBlocker::modifiedReload() const
{
    return m_reload;
}

} // namespace Core
