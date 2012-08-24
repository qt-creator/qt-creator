/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "clearcasesubmiteditor.h"
#include "clearcasesubmiteditorwidget.h"

#include <utils/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace ClearCase::Internal;

ClearCaseSubmitEditor::ClearCaseSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters,
                                             QWidget *parentWidget) :
    VcsBase::VcsBaseSubmitEditor(parameters, new ClearCaseSubmitEditorWidget(parentWidget))
{
    setDisplayName(tr("ClearCase Check In"));
}

ClearCaseSubmitEditorWidget *ClearCaseSubmitEditor::submitEditorWidget()
{
    return static_cast<ClearCaseSubmitEditorWidget *>(widget());
}

void ClearCaseSubmitEditor::setStatusList(const QStringList &statusOutput)
{
    typedef QStringList::const_iterator ConstIterator;
    VcsBase::SubmitFileModel *model = new VcsBase::SubmitFileModel(this);

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(*it, QLatin1String("C"), true);
    setFileModel(model, checkScriptWorkingDirectory());
    if (statusOutput.count() > 1)
        submitEditorWidget()->addKeep();
}

QByteArray ClearCaseSubmitEditor::fileContents() const
{
    return VcsBase::VcsBaseSubmitEditor::fileContents().trimmed();
}
