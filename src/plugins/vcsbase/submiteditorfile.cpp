// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "submiteditorfile.h"

#include "vcsbasesubmiteditor.h"

#include <utils/fileutils.h>

#include <QFileInfo>

using namespace VcsBase;
using namespace VcsBase::Internal;
using namespace Utils;

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
            this, &Core::IDocument::contentsChanged);
}

Core::IDocument::OpenResult SubmitEditorFile::open(QString *errorString,
                                                   const FilePath &filePath,
                                                   const FilePath &realFilePath)
{
    if (filePath.isEmpty())
        return OpenResult::ReadError;

    FileReader reader;
    if (!reader.fetch(realFilePath, QIODevice::Text, errorString))
        return OpenResult::ReadError;

    const QString text = QString::fromLocal8Bit(reader.data());
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

bool SubmitEditorFile::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    FileSaver saver(filePath, QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    saver.write(m_editor->fileContents());
    if (!saver.finalize(errorString))
        return false;
    if (autoSave)
        return true;
    setFilePath(filePath.absoluteFilePath());
    setModified(false);
    if (!errorString->isEmpty())
        return false;
    emit changed();
    return true;
}

Core::IDocument::ReloadBehavior SubmitEditorFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}
