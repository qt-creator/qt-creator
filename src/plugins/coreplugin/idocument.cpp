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

#include "idocument.h"

#include "infobar.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>

/*!
    \class Core::IDocument
    \brief The IDocument class describes a document that can be saved and reloaded.

    The most common use for implementing an IDocument subclass, is as a document for an IEditor
    implementation. Multiple editors can work in the same document instance, so the IDocument
    subclass should hold all data and functions that are independent from the specific
    IEditor instance, for example the content, highlighting information, the name of the
    corresponding file, and functions for saving and reloading the file.

    Each IDocument subclass works only with the corresponding IEditor subclasses that it
    was designed to work with.

    \mainclass
*/

/*!
    \fn QString Core::IDocument::filePath() const
    Returns the absolute path of the file that this document refers to. May be empty for
    non-file documents.
    \sa setFilePath()
*/

namespace Core {

namespace Internal {

class IDocumentPrivate
{
public:
    ~IDocumentPrivate()
    {
        delete infoBar;
    }

    QString mimeType;
    Utils::FileName filePath;
    QString preferredDisplayName;
    QString uniqueDisplayName;
    QString autoSaveName;
    InfoBar *infoBar = nullptr;
    Id id;
    bool temporary = false;
    bool hasWriteWarning = false;
    bool restored = false;
    bool isSuspendAllowed = false;
};

} // namespace Internal

IDocument::IDocument(QObject *parent) : QObject(parent),
    d(new Internal::IDocumentPrivate)
{
}

IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete d;
}

void IDocument::setId(Id id)
{
    d->id = id;
}

Id IDocument::id() const
{
    QTC_CHECK(d->id.isValid());
    return d->id;
}

/*!
    \enum IDocument::OpenResult
    The OpenResult enum describes whether a file was successfully opened.

    \value Success
           The file was read successfully and can be handled by this document type.
    \value ReadError
           The file could not be opened for reading, either because it does not exist or
           because of missing permissions.
    \value CannotHandle
           This document type could not handle the file content.
*/

/*!
 * Used to load a file if this document is part of an IEditor implementation, when the editor
 * is opened.
 * If the editor is opened from an auto save file, \a realFileName is the name of the auto save
 * that should be loaded, and \a fileName is the file name of the resulting file.
 * In that case, the contents of the auto save file should be loaded, the file name of the
 * IDocument should be set to \a fileName, and the document state be set to modified.
 * If the editor is opened from a regular file, \a fileName and \a realFileName are the same.
 * Use \a errorString to return an error message, if this document can not handle the
 * file contents.
 * Returns whether the file was opened and read successfully.
 */
IDocument::OpenResult IDocument::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(realFileName)
    return OpenResult::CannotHandle;
}

bool IDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
    return false;
}

/*!
 * Returns the current contents of the document. The base implementation returns an empty
 * QByteArray.
 */
QByteArray IDocument::contents() const
{
    return QByteArray();
}

/*!
    Used for example by EditorManager::openEditorWithContents() to set the contents
    of this document.
    Returns if setting the contents was successful.
    The base implementation does nothing and returns false.
*/
bool IDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents)
    return false;
}

const Utils::FileName &IDocument::filePath() const
{
    return d->filePath;
}

IDocument::ReloadBehavior IDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool IDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

void IDocument::checkPermissions()
{
}

bool IDocument::shouldAutoSave() const
{
    return false;
}

bool IDocument::isModified() const
{
    return false;
}

bool IDocument::isSaveAsAllowed() const
{
    return false;
}

bool IDocument::isSuspendAllowed() const
{
    return d->isSuspendAllowed;
}

void IDocument::setSuspendAllowed(bool value)
{
    d->isSuspendAllowed = value;
}

bool IDocument::isFileReadOnly() const
{
    if (filePath().isEmpty())
        return false;
    return !filePath().toFileInfo().isWritable();
}

/*!
    Returns if the document is a temporary that should for example not be considered
    when saving/restoring the session state, recent files, etc. Defaults to false.
    \sa setTemporary()
*/
bool IDocument::isTemporary() const
{
    return d->temporary;
}

/*!
    Sets if the document is \a temporary.
    \sa isTemporary()
*/
void IDocument::setTemporary(bool temporary)
{
    d->temporary = temporary;
}

QString IDocument::fallbackSaveAsPath() const
{
    return QString();
}

QString IDocument::fallbackSaveAsFileName() const
{
    return QString();
}

QString IDocument::mimeType() const
{
    return d->mimeType;
}

void IDocument::setMimeType(const QString &mimeType)
{
    if (d->mimeType != mimeType) {
        d->mimeType = mimeType;
        emit mimeTypeChanged();
    }
}

bool IDocument::autoSave(QString *errorString, const QString &fileName)
{
    if (!save(errorString, fileName, true))
        return false;
    d->autoSaveName = fileName;
    return true;
}

static const char kRestoredAutoSave[] = "RestoredAutoSave";

void IDocument::setRestoredFrom(const QString &name)
{
    d->autoSaveName = name;
    d->restored = true;
    InfoBarEntry info(Id(kRestoredAutoSave),
          tr("File was restored from auto-saved copy. "
             "Select Save to confirm or Revert to Saved to discard changes."));
    infoBar()->addInfo(info);
}

void IDocument::removeAutoSaveFile()
{
    if (!d->autoSaveName.isEmpty()) {
        QFile::remove(d->autoSaveName);
        d->autoSaveName.clear();
        if (d->restored) {
            d->restored = false;
            infoBar()->removeInfo(Id(kRestoredAutoSave));
        }
    }
}

bool IDocument::hasWriteWarning() const
{
    return d->hasWriteWarning;
}

void IDocument::setWriteWarning(bool has)
{
    d->hasWriteWarning = has;
}

InfoBar *IDocument::infoBar()
{
    if (!d->infoBar)
        d->infoBar = new InfoBar;
    return d->infoBar;
}

/*!
    Set absolute file path for this file to \a filePath. Can be empty.
    The default implementation sets the file name and sends filePathChanged() and changed()
    signals. Can be reimplemented by subclasses to do more.
    \sa filePath()
*/
void IDocument::setFilePath(const Utils::FileName &filePath)
{
    if (d->filePath == filePath)
        return;
    Utils::FileName oldName = d->filePath;
    d->filePath = filePath;
    emit filePathChanged(oldName, d->filePath);
    emit changed();
}

/*!
    Returns the string to display for this document, e.g. in the open document combo box
    and pane.
    The returned string has the following priority:
      * Unique display name set by the document model
      * Preferred display name set by the owner
      * Base name of the document's file name

    \sa setDisplayName()
*/
QString IDocument::displayName() const
{
    return d->uniqueDisplayName.isEmpty() ? plainDisplayName() : d->uniqueDisplayName;
}

/*!
    Sets the string that is displayed for this document, e.g. in the open document combo box
    and pane, to \a name. Defaults to the file name of the file path for this document.
    You can reset the display name to the default by passing an empty string.
    \sa displayName()
    \sa filePath()
 */
void IDocument::setPreferredDisplayName(const QString &name)
{
    if (name == d->preferredDisplayName)
        return;
    d->preferredDisplayName = name;
    emit changed();
}

QString IDocument::preferredDisplayName() const
{
    return d->preferredDisplayName;
}

/*!
    \internal
    Returns displayName without disambiguation.
 */
QString IDocument::plainDisplayName() const
{
    return d->preferredDisplayName.isEmpty() ? d->filePath.fileName() : d->preferredDisplayName;
}

/*!
    \internal
    Sets unique display name for the document. Used by the document model.
 */
void IDocument::setUniqueDisplayName(const QString &name)
{
    d->uniqueDisplayName = name;
}

QString IDocument::uniqueDisplayName() const
{
    return d->uniqueDisplayName;
}

} // namespace Core
