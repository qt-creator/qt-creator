/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

SubmitEditorFile::SubmitEditorFile(const VcsBaseSubmitEditorParameters *parameters, VcsBaseSubmitEditor *parent) :
    Core::IDocument(parent),
    m_modified(false),
    m_editor(parent)
{
    setId(parameters->id);
    setMimeType(QLatin1String(parameters->mimeType));
    setTemporary(true);
}

Core::IDocument::OpenResult SubmitEditorFile::open(QString *errorString, const QString &fileName,
                                                   const QString &realFileName)
{
    if (fileName.isEmpty())
        return OpenResult::ReadError;

    FileReader reader;
    if (!reader.fetch(realFileName, QIODevice::Text, errorString))
        return OpenResult::ReadError;

    const QString text = QString::fromLocal8Bit(reader.data());
    if (!m_editor->setFileContents(text.toUtf8()))
        return OpenResult::CannotHandle;

    setFilePath(FileName::fromString(fileName));
    setModified(fileName != realFileName);
    return OpenResult::Success;
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

bool SubmitEditorFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    const FileName fName = fileName.isEmpty() ? filePath() : FileName::fromString(fileName);
    FileSaver saver(fName.toString(),
                    QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    saver.write(m_editor->fileContents());
    if (!saver.finalize(errorString))
        return false;
    if (autoSave)
        return true;
    setFilePath(FileName::fromUserInput(fName.toFileInfo().absoluteFilePath()));
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

bool SubmitEditorFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}
