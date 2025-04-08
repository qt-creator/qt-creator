// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "submiteditorfile.h"

#include "vcsbasesubmiteditor.h"

#include <utils/fileutils.h>

using namespace Core;
using namespace Utils;

namespace VcsBase::Internal {
/*!
    \class VcsBase::Internal::SubmitEditorFile

    \brief The SubmitEditorFile class provides a non-saveable IDocument for
    submit editor files.
*/

SubmitEditorFile::SubmitEditorFile(VcsBaseSubmitEditor *editor) :
    m_modified(false),
    m_editor(editor)
{
    setTemporary(true);
    connect(m_editor, &VcsBaseSubmitEditor::fileContentsChanged,
            this, &IDocument::contentsChanged);
}

IDocument::OpenResult SubmitEditorFile::open(QString *errorString, const FilePath &filePath,
                                             const FilePath &realFilePath)
{
    if (filePath.isEmpty())
        return OpenResult::ReadError;

    FileReader reader;
    if (!reader.fetch(realFilePath, errorString))
        return OpenResult::ReadError;

    const QString text = QString::fromLocal8Bit(reader.text());
    if (!m_editor->setFileContents(text.toUtf8()))
        return OpenResult::CannotHandle;

    setFilePath(filePath.absoluteFilePath());
    setModified(filePath != realFilePath);
    return OpenResult::Success;
}

QByteArray SubmitEditorFile::contents() const
{
    return m_editor->fileContents();
}

bool SubmitEditorFile::setContents(const QByteArray &contents)
{
    return m_editor->setFileContents(contents);
}

void SubmitEditorFile::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

Result<> SubmitEditorFile::saveImpl(const FilePath &filePath, bool autoSave)
{
    FileSaver saver(filePath, QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    saver.write(m_editor->fileContents());
    QString errorString;
    if (!saver.finalize(&errorString))
        return ResultError(errorString);
    if (autoSave)
        return ResultOk;
    setFilePath(filePath.absoluteFilePath());
    setModified(false);
    if (!errorString.isEmpty())
        return ResultError(errorString);
    emit changed();
    return ResultOk;
}

IDocument::ReloadBehavior SubmitEditorFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

} // VcsBase::Internal
