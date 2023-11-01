// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cvssubmiteditor.h"

#include "cvstr.h"

#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace VcsBase;

namespace Cvs::Internal {

CvsSubmitEditor::CvsSubmitEditor() :
    VcsBase::VcsBaseSubmitEditor(new SubmitEditorWidget),
    m_msgAdded(Tr::tr("Added")),
    m_msgRemoved(Tr::tr("Removed")),
    m_msgModified(Tr::tr("Modified"))
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
    return {};
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

} // Cvs::Internal
