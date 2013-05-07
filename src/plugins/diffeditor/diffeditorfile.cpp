/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "diffeditorfile.h"

namespace DiffEditor {
namespace Internal {

///////////////////////////////// DiffFile //////////////////////////////////

DiffEditorFile::DiffEditorFile(const QString &mimeType, QObject *parent) :
    Core::IDocument(parent),
    m_mimeType(mimeType),
    m_modified(false)
{
}

void DiffEditorFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
    return;
}

void DiffEditorFile::setFileName(const QString &name)
{
    if (m_fileName == name)
        return;
    m_fileName = name;
    emit changed();
}

void DiffEditorFile::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

bool DiffEditorFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    emit saveMe(errorString, fileName, autoSave);
    if (!errorString->isEmpty())
        return false;
    emit changed();
    return true;
}

QString DiffEditorFile::mimeType() const
{
    return m_mimeType;
}

Core::IDocument::ReloadBehavior DiffEditorFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool DiffEditorFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

} // namespace Internal
} // namespace DiffEditor
