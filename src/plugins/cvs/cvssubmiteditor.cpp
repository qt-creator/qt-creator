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

#include "cvssubmiteditor.h"

#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Cvs::Internal;
using namespace VcsBase;

CvsSubmitEditor::CvsSubmitEditor(const VcsBaseSubmitEditorParameters *parameters) :
    VcsBaseSubmitEditor(parameters, new SubmitEditorWidget),
    m_msgAdded(tr("Added")),
    m_msgRemoved(tr("Removed")),
    m_msgModified(tr("Modified"))
{ }

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
    auto model = new SubmitFileModel(this);

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(it->second, stateName(it->first));
    setFileModel(model);
}
