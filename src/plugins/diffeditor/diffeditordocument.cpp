/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "diffeditordocument.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"

#include <QCoreApplication>

namespace DiffEditor {

DiffEditorDocument::DiffEditorDocument() :
    Core::IDocument(),
    m_diffEditorController(new DiffEditorController(this))
{
    setId(Constants::DIFF_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("DiffEditor", Constants::DIFF_EDITOR_DISPLAY_NAME));
    setTemporary(true);
}

DiffEditorDocument::~DiffEditorDocument()
{
}

DiffEditorController *DiffEditorDocument::controller() const
{
    return m_diffEditorController;
}

bool DiffEditorDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents);
    return true;
}

bool DiffEditorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
    return false;
}

Core::IDocument::ReloadBehavior DiffEditorDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool DiffEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return false;
}

} // namespace DiffEditor
