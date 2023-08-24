// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idocument.h"

#include "coreplugintr.h"

#include <utils/filepath.h>
#include <utils/infobar.h>
#include <utils/minimizableinfobars.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>

#include <memory>
#include <optional>

/*!
    \class Core::IDocument
    \inheaderfile coreplugin/idocument.h
    \inmodule QtCreator

    \brief The IDocument class describes a document that can be saved and
    reloaded.

    The class has two use cases.

    \section1 Handling External Modifications

    You can implement IDocument and register instances in DocumentManager to
    let it handle external modifications of a file. When the file specified with
    filePath() has changed externally, the DocumentManager asks the
    corresponding IDocument instance what to do via reloadBehavior(). If that
    returns \c IDocument::BehaviorAsk, the user is asked if the file should be
    reloaded from disk. If the user requests the reload or reloadBehavior()
    returns \c IDocument::BehaviorSilent, the DocumentManager calls reload()
    to initiate a reload of the file from disk.

    Core functions: setFilePath(), reload(), reloadBehavior().

    If the content of the document can change in \QC, diverging from the
    content on disk: isModified(), save(), isSaveAsAllowed(),
    fallbackSaveAsPath(), fallbackSaveAsFileName().

    \section1 Editor Document

    The most common use case for implementing an IDocument subclass is as a
    document for an IEditor implementation. Multiple editor instances can work
    on the same document instance, for example if the document is visible in
    multiple splits simultaneously. So the IDocument subclass should hold all
    data that is independent from the specific IEditor instance, for example
    the content and highlighting information.

    Each IDocument subclass is only required to work with the corresponding
    IEditor subclasses that it was designed to work with.

    An IDocument can either be backed by a file, or solely represent some data
    in memory. Documents backed by a file are automatically added to the
    DocumentManager by the EditorManager.

    Core functions: setId(), isModified(), contents(), setContents().

    If the content of the document is backed by a file: open(), save(),
    setFilePath(), mimeType(), shouldAutoSave(), setSuspendAllowed(), and
    everything from \l{Handling External Modifications}.

    If the content of the document is not backed by a file:
    setPreferredDisplayName(), setTemporary().

    \ingroup mainclasses
*/

/*!
    \enum IDocument::OpenResult

    The OpenResult enum describes whether a file was successfully opened.

    \value Success
           The file was read successfully and can be handled by this document
           type.
    \value ReadError
           The file could not be opened for reading, either because it does not
           exist or because of missing permissions.
    \value CannotHandle
           This document type could not handle the file content.
*/

/*!
    \enum IDocument::ReloadSetting

    \internal
*/

/*!
    \enum IDocument::ChangeTrigger

    The ChangeTrigger enum describes whether a file was changed from \QC
    internally or from the outside.

    \value TriggerInternal
           The file was changed by \QC.
    \value TriggerExternal
           The file was changed from the outside.

    \sa IDocument::reloadBehavior()
*/

/*!
    \enum IDocument::ChangeType

    The ChangeType enum describes the way in which the file changed.

    \value TypeContents
           The contents of the file changed.
    \value TypeRemoved
           The file was removed.

    \sa IDocument::reloadBehavior()
    \sa IDocument::reload()
*/

/*!
    \enum IDocument::ReloadFlag

    The ReloadFlag enum describes if a file should be reloaded from disk.

    \value FlagReload
           The file should be reloaded.
    \value FlagIgnore
           The file should not be reloaded, but the document state should
           reflect the change.

    \sa IDocument::reload()
*/

/*!
    \fn Core::IDocument::changed()

    This signal is emitted when the document's meta data, like file name or
    modified state, changes.

    \sa isModified()
    \sa filePath()
    \sa displayName()
*/

/*!
    \fn Core::IDocument::contentsChanged()

    This signal is emitted when the document's content changes.

    \sa contents()
*/

/*!
    \fn Core::IDocument::mimeTypeChanged()

    This signal is emitted when the document content's MIME type changes.

    \sa mimeType()
*/

/*!
    \fn Core::IDocument::aboutToReload()

    This signal is emitted before the document is reloaded from the backing
    file.

    \sa reload()
*/

/*!
    \fn Core::IDocument::reloadFinished(bool success)

    This signal is emitted after the document is reloaded from the backing
    file, or if reloading failed.

    The success state is passed in \a success.

    \sa reload()
*/

/*!
    \fn Core::IDocument::aboutToSave(const Utils::FilePath &filePath, bool autoSave)

    This signal is emitted before the document is saved to \a filePath.

    \a autoSave indicates whether this save was triggered by the auto save timer.

    \sa save()
*/

/*!
    \fn Core::IDocument::saved(const Utils::FilePath &filePath, bool autoSave)

    This signal is emitted after the document was saved to \a filePath.

    \a autoSave indicates whether this save was triggered by the auto save timer.

    \sa save()
*/

/*!
    \fn Core::IDocument::filePathChanged(const Utils::FilePath &oldName, const Utils::FilePath &newName)

    This signal is emitted after the file path changed from \a oldName to \a
    newName.

    \sa filePath()
*/

using namespace Utils;

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
    Utils::FilePath filePath;
    QString preferredDisplayName;
    QString uniqueDisplayName;
    Utils::FilePath autoSavePath;
    Utils::InfoBar *infoBar = nullptr;
    std::unique_ptr<MinimizableInfoBars> minimizableInfoBars;
    Id id;
    std::optional<bool> fileIsReadOnly;
    bool temporary = false;
    bool hasWriteWarning = false;
    bool restored = false;
    bool isSuspendAllowed = false;
};

} // namespace Internal

/*!
    Creates an IDocument with \a parent.

    \note Using the \a parent for ownership of the document is generally a
    bad idea if the IDocument is intended for use with IEditor. It is better to
    use shared ownership in that case.
*/
IDocument::IDocument(QObject *parent) : QObject(parent),
    d(new Internal::IDocumentPrivate)
{
}

/*!
    Destroys the IDocument.
    If there was an auto save file for this document, it is removed.

    \sa shouldAutoSave()
*/
IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete d;
}

/*!
    \fn void IDocument::setId(Utils::Id id)

    Sets the ID for this document type to \a id. This is coupled with the
    corresponding IEditor implementation and the \l{IEditorFactory::id()}{id()}
    of the IEditorFactory. If the IDocument implementation only works with a
    single IEditor type, this is preferably set in the IDocuments's
    constructor.

    \sa id()
*/
void IDocument::setId(Id id)
{
    d->id = id;
}

/*!
    Returns the ID for this document type.

    \sa setId()
*/
Id IDocument::id() const
{
    QTC_CHECK(d->id.isValid());
    return d->id;
}

/*!
    The open() method is used to load the contents of a file when a document is
    opened in an editor.

    If the document is opened from an auto save file, \a realFilePath is the
    name of the auto save file that should be loaded, and \a filePath is the
    file name of the resulting file. In that case, the contents of the auto
    save file should be loaded, the file name of the IDocument should be set to
    \a filePath, and the document state be set to modified.

    If the editor is opened from a regular file, \a filePath and \a
    filePath are the same.

    Use \a errorString to return an error message if this document cannot
    handle the file contents.

    Returns whether the file was opened and read successfully.

    The default implementation does nothing and returns
    CannotHandle.

    \sa EditorManager::openEditor()
    \sa shouldAutoSave()
    \sa setFilePath()
*/
IDocument::OpenResult IDocument::open(QString *errorString, const Utils::FilePath &filePath, const Utils::FilePath &realFilePath)
{
    Q_UNUSED(errorString)
    Q_UNUSED(filePath)
    Q_UNUSED(realFilePath)
    return OpenResult::CannotHandle;
}

/*!
    Saves the contents of the document to the \a filePath on disk.
    If \a filePath is empty filePath() is used.

    If \a autoSave is \c true, the saving is done for an auto-save, so the
    document should avoid cleanups or other operations that it does for
    user-requested saves.

    Use \a errorString to return an error message if saving failed.

    Returns whether saving was successful.

    If saving was successful saved is emitted.

    \sa shouldAutoSave()
    \sa aboutToSave()
    \sa saved()
    \sa filePath()
*/
bool IDocument::save(QString *errorString, const Utils::FilePath &filePath, bool autoSave)
{
    const Utils::FilePath savePath = filePath.isEmpty() ? this->filePath() : filePath;
    emit aboutToSave(savePath, autoSave);
    const bool success = saveImpl(errorString, savePath, autoSave);
    if (success)
        emit saved(savePath, autoSave);
    return success;
}

/*!
    Implementation of saving the contents of the document to the \a filePath on disk.

    If \a autoSave is \c true, the saving is done for an auto-save, so the
    document should avoid cleanups or other operations that it does for
    user-requested saves.

    Use \a errorString to return an error message if saving failed.

    Returns whether saving was successful.

    The default implementation does nothing and returns \c false.
*/
bool IDocument::saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(filePath)
    Q_UNUSED(autoSave)
    return false;
}

/*!
    Returns the current contents of the document. The default implementation
    returns an empty QByteArray.

    \sa setContents()
    \sa contentsChanged()
*/
QByteArray IDocument::contents() const
{
    return QByteArray();
}

/*!
    The setContents() method is for example used by
    EditorManager::openEditorWithContents() to set the \a contents of this
    document.

    Returns whether setting the contents was successful.

    The default implementation does nothing and returns false.

    \sa contents()
    \sa EditorManager::openEditorWithContents()
*/
bool IDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents)
    return false;
}

/*!
    Formats the contents of the document, if the implementation supports such functionality.
*/
void IDocument::formatContents()
{
}

/*!
    Returns the absolute path of the file that this document refers to. May be
    empty for documents that are not backed by a file.

    \sa setFilePath()
*/
const Utils::FilePath &IDocument::filePath() const
{
    return d->filePath;
}

/*!
    The reloadBehavior() method is used by the DocumentManager to ask what to
    do if the file backing this document has changed on disk.

    The \a trigger specifies if the change was triggered by some operation in
    \QC. The \a type specifies if the file changed permissions or contents, or
    was removed completely.

    Returns whether the user should be asked or the document should be
    reloaded silently.

    The default implementation requests a silent reload if only the permissions
    changed or if the contents have changed but the \a trigger is internal and
    the document is not modified.

    \sa isModified()
*/
IDocument::ReloadBehavior IDocument::reloadBehavior(ChangeTrigger trigger, ChangeType type) const
{
    if (type == TypeContents && trigger == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

/*!
    Reloads the document from the backing file when that changed on disk.

    If \a flag is FlagIgnore the file should not actually be loaded, but the
    document should reflect the change in its \l{isModified()}{modified state}.

    The \a type specifies whether only the file permissions changed or if the
    contents of the file changed.

    Use \a errorString to return an error message, if this document cannot
    handle the file contents.

    Returns if the file was reloaded successfully.

    The default implementation does nothing and returns \c true.

    Subclasses should emit aboutToReload() before, and reloadFinished() after
    reloading the file.

    \sa isModified()
    \sa aboutToReload()
    \sa reloadFinished()
    \sa changed()
*/
bool IDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

/*!
    Updates the cached information about the read-only status of the backing file.
*/
void IDocument::checkPermissions()
{
    bool previousReadOnly = d->fileIsReadOnly.value_or(false);
    if (!filePath().isEmpty()) {
        d->fileIsReadOnly = !filePath().isWritableFile();
    } else {
        d->fileIsReadOnly = false;
    }
    if (previousReadOnly != *(d->fileIsReadOnly))
        emit changed();
}

/*!
    Returns whether the document should automatically be saved at a user-defined
    interval.

    The default implementation returns \c false.
*/
bool IDocument::shouldAutoSave() const
{
    return false;
}

/*!
    Returns whether the document has been modified after it was loaded from a
    file.

    The default implementation returns \c false. Re-implementations should emit
    changed() when this property changes.

    \sa changed()
*/
bool IDocument::isModified() const
{
    return false;
}

/*!
    Returns whether the document may be saved under a different file name.

    The default implementation returns \c false.

    \sa save()
*/
bool IDocument::isSaveAsAllowed() const
{
    return false;
}

/*!
    Returns whether the document may be suspended.

    The EditorManager can automatically suspend editors and its corresponding
    documents if the document is backed by a file, is not modified, and is not
    temporary. Suspended IEditor and IDocument instances are deleted and
    removed from memory, but are still visually accessible as if the document
    was still opened in \QC.

    The default is \c false.

    \sa setSuspendAllowed()
    \sa isModified()
    \sa isTemporary()
*/
bool IDocument::isSuspendAllowed() const
{
    return d->isSuspendAllowed;
}

/*!
    Sets whether the document may be suspended to \a value.

    \sa isSuspendAllowed()
*/
void IDocument::setSuspendAllowed(bool value)
{
    d->isSuspendAllowed = value;
}

/*!
    Returns whether the file backing this document is read-only, or \c false if
    the document is not backed by a file.
*/
bool IDocument::isFileReadOnly() const
{
    if (filePath().isEmpty())
        return false;
    if (!d->fileIsReadOnly)
        const_cast<IDocument *>(this)->checkPermissions();
    return d->fileIsReadOnly.value_or(false);
}

/*!
    Returns if the document is temporary, and should for example not be
    considered when saving or restoring the session state, or added to the recent
    files list.

    The default is \c false.

    \sa setTemporary()
*/
bool IDocument::isTemporary() const
{
    return d->temporary;
}

/*!
    Sets whether the document is \a temporary.

    \sa isTemporary()
*/
void IDocument::setTemporary(bool temporary)
{
    d->temporary = temporary;
}

/*!
    Returns a path to use for the \uicontrol{Save As} file dialog in case the
    document is not backed by a file.

    \sa fallbackSaveAsFileName()
*/
FilePath IDocument::fallbackSaveAsPath() const
{
    return {};
}

/*!
    Returns a file name to use for the \uicontrol{Save As} file dialog in case
    the document is not backed by a file.

    \sa fallbackSaveAsPath()
*/
QString IDocument::fallbackSaveAsFileName() const
{
    return QString();
}

/*!
    Returns the MIME type of the document content, if applicable.

    Subclasses should set this with setMimeType() after setting or loading
    content.

    The default MIME type is empty.

    \sa setMimeType()
    \sa mimeTypeChanged()
*/
QString IDocument::mimeType() const
{
    return d->mimeType;
}

/*!
    Sets the MIME type of the document content to \a mimeType.

    \sa mimeType()
*/
void IDocument::setMimeType(const QString &mimeType)
{
    if (d->mimeType != mimeType) {
        d->mimeType = mimeType;
        emit mimeTypeChanged();
    }
}

/*!
    \internal
*/
bool IDocument::autoSave(QString *errorString, const FilePath &filePath)
{
    if (!save(errorString, filePath, true))
        return false;
    d->autoSavePath = filePath;
    return true;
}

static const char kRestoredAutoSave[] = "RestoredAutoSave";

/*!
    \internal
*/
void IDocument::setRestoredFrom(const Utils::FilePath &path)
{
    d->autoSavePath = path;
    d->restored = true;
    Utils::InfoBarEntry info(Id(kRestoredAutoSave),
                             Tr::tr("File was restored from auto-saved copy. "
                                "Select Save to confirm or Revert to Saved to discard changes."));
    infoBar()->addInfo(info);
}

/*!
    \internal
*/
void IDocument::removeAutoSaveFile()
{
    if (!d->autoSavePath.isEmpty()) {
        QFile::remove(d->autoSavePath.toString());
        d->autoSavePath.clear();
        if (d->restored) {
            d->restored = false;
            infoBar()->removeInfo(Id(kRestoredAutoSave));
        }
    }
}

/*!
    \internal
*/
bool IDocument::hasWriteWarning() const
{
    return d->hasWriteWarning;
}

/*!
    \internal
*/
void IDocument::setWriteWarning(bool has)
{
    d->hasWriteWarning = has;
}

/*!
    Returns the document's Utils::InfoBar, which is shown at the top of an
    editor.
*/
Utils::InfoBar *IDocument::infoBar()
{
    if (!d->infoBar)
        d->infoBar = new Utils::InfoBar;
    return d->infoBar;
}

MinimizableInfoBars *IDocument::minimizableInfoBars()
{
    if (!d->minimizableInfoBars)
        d->minimizableInfoBars.reset(new Utils::MinimizableInfoBars(*infoBar()));
    return d->minimizableInfoBars.get();
}

/*!
    Sets the absolute \a filePath of the file that backs this document. The
    default implementation sets the file name and sends the filePathChanged() and
    changed() signals.

    \sa filePath()
    \sa filePathChanged()
    \sa changed()
*/
void IDocument::setFilePath(const Utils::FilePath &filePath)
{
    if (d->filePath == filePath)
        return;
    Utils::FilePath oldName = d->filePath;
    d->filePath = filePath;
    emit filePathChanged(oldName, d->filePath);
    emit changed();
}

/*!
    Returns the string to display for this document, for example in the
    \uicontrol{Open Documents} view and the documents drop down.

    The display name is one of the following, in order:

    \list 1
        \li Unique display name set by the document model
        \li Preferred display name set by the owner
        \li Base name of the document's file name
    \endlist

    \sa setPreferredDisplayName()
    \sa filePath()
    \sa changed()
*/
QString IDocument::displayName() const
{
    return d->uniqueDisplayName.isEmpty() ? plainDisplayName() : d->uniqueDisplayName;
}

/*!
    Sets the preferred display \a name for this document.

    \sa preferredDisplayName()
    \sa displayName()
 */
void IDocument::setPreferredDisplayName(const QString &name)
{
    if (name == d->preferredDisplayName)
        return;
    d->preferredDisplayName = name;
    emit changed();
}

/*!
    Returns the preferred display name for this document.

    The default preferred display name is empty, which means that the display
    name is preferably the file name of the file backing this document.

    \sa setPreferredDisplayName()
    \sa displayName()
*/
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

/*!
    \internal
*/
QString IDocument::uniqueDisplayName() const
{
    return d->uniqueDisplayName;
}

} // namespace Core
