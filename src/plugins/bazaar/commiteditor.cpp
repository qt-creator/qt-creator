/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#include "branchinfo.h"
#include "commiteditor.h"
#include "bazaarcommitwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QDebug>

#include <QDir> //TODO REMOVE WHEN BASE FILE CHANGES ARE PULLED

using namespace Bazaar::Internal;

CommitEditor::CommitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters, QWidget *parent)
    : VcsBase::VcsBaseSubmitEditor(parameters, new BazaarCommitWidget(parent)),
      m_fileModel(0)
{
    setDisplayName(tr("Commit Editor"));
}

const BazaarCommitWidget *CommitEditor::commitWidget() const
{
    CommitEditor *nonConstThis = const_cast<CommitEditor *>(this);
    return static_cast<const BazaarCommitWidget *>(nonConstThis->widget());
}

BazaarCommitWidget *CommitEditor::commitWidget()
{
    return static_cast<BazaarCommitWidget *>(widget());
}

void CommitEditor::setFields(const QString &repositoryRoot,
                             const BranchInfo &branch, const QString &userName,
                             const QString &email, const QList<VcsBase::VcsBaseClient::StatusItem> &repoStatus)
{
    BazaarCommitWidget *bazaarWidget = commitWidget();
    if (!bazaarWidget)
        return;

    bazaarWidget->setFields(branch, userName, email);

    m_fileModel = new VcsBase::SubmitFileModel(this);
    foreach (const VcsBase::VcsBaseClient::StatusItem &item, repoStatus)
        if (item.flags != QLatin1String("Unknown"))
            m_fileModel->addFile(item.file, item.flags, true);
    setFileModel(m_fileModel, repositoryRoot);
}
