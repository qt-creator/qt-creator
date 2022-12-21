// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearcasesubmiteditor.h"

#include "clearcasesubmiteditorwidget.h"
#include "clearcasetr.h"

#include <coreplugin/idocument.h>

#include <vcsbase/submitfilemodel.h>

namespace ClearCase::Internal {

ClearCaseSubmitEditor::ClearCaseSubmitEditor() :
    VcsBase::VcsBaseSubmitEditor(new ClearCaseSubmitEditorWidget)
{
    document()->setPreferredDisplayName(Tr::tr("ClearCase Check In"));
}

ClearCaseSubmitEditorWidget *ClearCaseSubmitEditor::submitEditorWidget()
{
    return static_cast<ClearCaseSubmitEditorWidget *>(widget());
}

void ClearCaseSubmitEditor::setIsUcm(bool isUcm)
{
    submitEditorWidget()->addActivitySelector(isUcm);
}

void ClearCaseSubmitEditor::setStatusList(const QStringList &statusOutput)
{
    using ConstIterator = QStringList::const_iterator;
    auto model = new VcsBase::SubmitFileModel(this);
    model->setRepositoryRoot(checkScriptWorkingDirectory());

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(*it, QLatin1String("C"));
    setFileModel(model);
    if (statusOutput.count() > 1)
        submitEditorWidget()->addKeep();
}

QByteArray ClearCaseSubmitEditor::fileContents() const
{
    return VcsBase::VcsBaseSubmitEditor::fileContents().trimmed();
}

} // ClearCase::Internal
