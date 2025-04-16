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

Result<> SubmitEditorFile::open(const FilePath &filePath, const FilePath &realFilePath)
{
    if (filePath.isEmpty())
        return ResultError("File name is empty");  // FIXME: Use something better

    FileReader reader;
    if (const Result<> res = reader.fetch(realFilePath); !res)
        return res;

    const QString text = QString::fromLocal8Bit(reader.text());
    if (!m_editor->setFileContents(text.toUtf8()))
        return ResultError("Cannot set file contents"); // FIXME: Use something better

    setFilePath(filePath.absoluteFilePath());
    setModified(filePath != realFilePath);
    return ResultOk;
}

QByteArray SubmitEditorFile::contents() const
{
    return m_editor->fileContents();
}

Result<> SubmitEditorFile::setContents(const QByteArray &contents)
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
    if (const Result<> res = saver.finalize(); !res)
        return res;
    if (autoSave)
        return ResultOk;
    setFilePath(filePath.absoluteFilePath());
    setModified(false);
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
