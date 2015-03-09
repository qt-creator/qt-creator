/**************************************************************************
**
** Copyright (C) 2015 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcasesubmiteditor.h"
#include "clearcasesubmiteditorwidget.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace ClearCase::Internal;

ClearCaseSubmitEditor::ClearCaseSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters) :
    VcsBase::VcsBaseSubmitEditor(parameters, new ClearCaseSubmitEditorWidget)
{
    document()->setPreferredDisplayName(tr("ClearCase Check In"));
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
    typedef QStringList::const_iterator ConstIterator;
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
