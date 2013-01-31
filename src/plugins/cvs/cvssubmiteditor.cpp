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


#include "cvssubmiteditor.h"

#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Cvs::Internal;
using namespace VcsBase;

CvsSubmitEditor::CvsSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                 QWidget *parentWidget) :
    VcsBaseSubmitEditor(parameters, new VcsBase::SubmitEditorWidget(parentWidget)),
    m_msgAdded(tr("Added")),
    m_msgRemoved(tr("Removed")),
    m_msgModified(tr("Modified"))
{
}

QString CvsSubmitEditor::stateName(State st) const
{
    switch (st) {
    case LocallyAdded:
        return m_msgAdded;
    case LocallyModified:
        return m_msgModified;
    case LocallyRemoved:
        return m_msgRemoved;
    }
    return QString();
}

void CvsSubmitEditor::setStateList(const StateFilePairs &statusOutput)
{
    typedef StateFilePairs::const_iterator ConstIterator;
    SubmitFileModel *model = new SubmitFileModel(this);

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(it->second, stateName(it->first));
    setFileModel(model);
}
