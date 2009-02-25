/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "filemanager.h"

#include "editormanager.h"
#include "icore.h"
#include "ifile.h"
#include "iversioncontrol.h"
#include "mainwindow.h"
#include "mimedatabase.h"
#include "saveitemsdialog.h"
#include "vcsmanager.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QFileSystemWatcher>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace Core;
using namespace Core::Internal;

/*!
  \class FileManager
  \mainclass
  \ingroup qwb
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

static const char *settingsGroup = "RecentFiles";
static const char *filesKey = "Files";

FileManager::FileManager(MainWindow *mw)
  : QObject(mw),
    m_mainWindow(mw),
    m_fileWatcher(new QFileSystemWatcher(this)),
    m_blockActivated(false)
{
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
        this, SLOT(changedFile(QString)));
    connect(m_mainWindow, SIGNAL(windowActivated()),
        this, SLOT(mainWindowActivated()));
    connect(Core::ICore::instance(), SIGNAL(contextChanged(Core::IContext*)),
        this, SLOT(syncWithEditor(Core::IContext*)));

    QSettings *s = m_mainWindow->settings();
    s->beginGroup(QLatin1String(settingsGroup));
    m_recentFiles = s->value(QLatin1String(filesKey), QStringList()).toStringList();
    s->endGroup();
    for (QStringList::iterator it = m_recentFiles.begin(); it != m_recentFiles.end(); ) {
        if (QFileInfo(*it).isFile()) {
            ++it;
        } else {
            it = m_recentFiles.erase(it);
        }
    }
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
        if (!file || m_managedFiles.contains(file))
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
    const QString filename = m_managedFiles.value(file).fileName;
    m_managedFiles.remove(file);
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

    if (!m_managedFiles.contains(file))
        return false;
    const FileInfo info = m_managedFiles.take(file);
    const QString filename = info.fileName;
    removeWatch(filename);
    return true;
}

void FileManager::addWatch(const QString &filename)
{
    if (!filename.isEmpty() && managedFiles(filename).isEmpty())
        m_fileWatcher->addPath(filename);
}

void FileManager::removeWatch(const QString &filename)
{
    if (!filename.isEmpty() && managedFiles(filename).isEmpty())
        m_fileWatcher->removePath(filename);
}

void FileManager::checkForNewFileName()
{
    IFile *file = qobject_cast<IFile *>(sender());
    QTC_ASSERT(file, return);
    const QString newfilename = fixFileName(file->fileName());
    const QString oldfilename = m_managedFiles.value(file).fileName;
    if (!newfilename.isEmpty() && newfilename != oldfilename) {
        m_managedFiles[file].fileName = newfilename;
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

    const QMap<IFile*, FileInfo>::const_iterator cend =  m_managedFiles.constEnd();
    for (QMap<IFile*, FileInfo>::const_iterator i = m_managedFiles.constBegin(); i != cend; ++i) {
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
        m_fileWatcher->removePath(file->fileName());
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
        m_fileWatcher->addPath(file->fileName());
}

void FileManager::updateFileInfo(IFile *file)
{
    const QString fixedname = fixFileName(file->fileName());
    const QFileInfo fi(file->fileName());
    FileInfo info;
    info.fileName = fixedname;
    info.modified = fi.lastModified();
    info.permissions = fi.permissions();
    m_managedFiles.insert(file, info);
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
                                                     bool *cancelled, const QString &message)
{
    return saveModifiedFiles(files, cancelled, false, message);
}

static QMessageBox::StandardButton skipFailedPrompt(QWidget *parent, const QString &fileName)
{
    return QMessageBox::question(parent,
                                 QObject::tr("Can't save file"),
                                 QObject::tr("Can't save changes to '%1'. Do you want to continue and loose your changes?").arg(fileName),
                                 QMessageBox::YesToAll| QMessageBox::Yes|QMessageBox::No,
                                 QMessageBox::No);
}

QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                                     bool *cancelled, bool silently, const QString &message)
{
    if (cancelled)
        (*cancelled) = false;

    QList<IFile *> notSaved;
    QMap<IFile*, QString> modifiedFiles;

    foreach (IFile *file, files) {
        if (file->isModified()) {
            QString name = file->fileName();
            if (name.isEmpty())
                name = file->suggestedFileName();

            // There can be several FileInterfaces pointing to the same file
            // Select one that is not readonly.
            if (!(modifiedFiles.values().contains(name)
                && file->isReadOnly()))
                modifiedFiles.insert(file, name);
        }
    }

    if (!modifiedFiles.isEmpty()) {
        QList<IFile *> filesToSave;
        QSet<IFile *> filesToOpen;
        if (silently) {
            filesToSave = modifiedFiles.keys();
        } else {
            SaveItemsDialog dia(m_mainWindow, modifiedFiles);
            if (!message.isEmpty())
                dia.setMessage(message);
            if (dia.exec() != QDialog::Accepted) {
                if (cancelled)
                    (*cancelled) = true;
                notSaved = modifiedFiles.keys();
                return notSaved;
            }
            filesToSave = dia.itemsToSave();
            filesToOpen = dia.itemsToOpen();
        }

        bool yestoall = false;
        foreach (IFile *file, filesToSave) {
            if (file->isReadOnly() && filesToOpen.contains(file)) {
                QString directory = QFileInfo(file->fileName()).absolutePath();
                IVersionControl *versionControl = m_mainWindow->vcsManager()->findVersionControlForDirectory(directory);
                if (versionControl)
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
                switch (skipFailedPrompt(m_mainWindow, fi.fileName())) {
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
                    unblockFileChange(file);
                }
                if (!ok)
                    notSaved.append(file);
            }
        }
    }
    return notSaved;
}

QString FileManager::getSaveFileNameWithExtension(const QString &title, const QString &path,
    const QString &fileFilter, const QString &extension)
{
    QString fileName;
    bool repeat;
    do {
        repeat = false;
        fileName = QFileDialog::getSaveFileName(m_mainWindow, title, path, fileFilter);
        if (!fileName.isEmpty() && !extension.isEmpty() && !fileName.endsWith(extension)) {
            fileName.append(extension);
            if (QFile::exists(fileName)) {
                if (QMessageBox::warning(m_mainWindow, tr("Overwrite?"),
                        tr("An item named '%1' already exists at this location. Do you want to overwrite it?").arg(fileName),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                    repeat = true;
            }
        }
    } while (repeat);
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

void FileManager::changedFile(const QString &file)
{
    const bool wasempty = m_changedFiles.isEmpty();
    foreach (IFile *fileinterface, managedFiles(file))
        m_changedFiles << fileinterface;
    if (wasempty && !m_changedFiles.isEmpty()) {
        QTimer::singleShot (200, this, SLOT(checkForReload()));
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
    if (QApplication::activeWindow() == m_mainWindow &&
        !m_blockActivated && !m_changedFiles.isEmpty()) {
        m_blockActivated = true;
        const QList<QPointer<IFile> > changed = m_changedFiles;
        m_changedFiles.clear();
        IFile::ReloadBehavior behavior =
            IFile::AskForReload;
        foreach (IFile *f, changed) {
            if (!f)
                continue;
            QFileInfo fi(f->fileName());
            FileInfo info = m_managedFiles.value(f);
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
                m_fileWatcher->removePath(f->fileName());
                m_fileWatcher->addPath(f->fileName());
            }
        }
        m_blockActivated = false;
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
    m_recentFiles.removeAll(prettyFileName);
    if (m_recentFiles.count() > m_maxRecentFiles)
        m_recentFiles.removeLast();
    m_recentFiles.prepend(prettyFileName);
}

/*!
    \fn QStringList FileManager::recentFiles() const

    Returns the list of recent files.
*/
QStringList FileManager::recentFiles() const
{
    return m_recentFiles;
}

void FileManager::saveRecentFiles()
{
    QSettings *s = m_mainWindow->settings();
    s->beginGroup(QLatin1String(settingsGroup));
    s->setValue(QLatin1String(filesKey), m_recentFiles);
    s->endGroup();
}

/*!

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \see currentFile
  */
void FileManager::setCurrentFile(const QString &filePath)
{
    if (m_currentFile == filePath)
        return;
    m_currentFile = filePath;
    emit currentFileChanged(m_currentFile);
}

/*!
  Returns the absolute path of the current file

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \see setCurrentFile
  */
QString FileManager::currentFile() const
{
    return m_currentFile;
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
        const QMap<IFile*, FileInfo>::const_iterator cend =  m_managedFiles.constEnd();
        for (QMap<IFile*, FileInfo>::const_iterator i = m_managedFiles.constBegin(); i != cend; ++i) {
            if (i.value().fileName == fixedName)
                result << i.key();
        }
    }
    return result;
}
