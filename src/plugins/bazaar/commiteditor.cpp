/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
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
#include "commiteditor.h"
#include "branchinfo.h"
#include "bazaarcommitwidget.h"
#include "constants.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>

using namespace Bazaar::Internal;

CommitEditor::CommitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters)
    : VcsBase::VcsBaseSubmitEditor(parameters, new BazaarCommitWidget),
      m_fileModel(0)
{
    document()->setPreferredDisplayName(tr("Commit Editor"));
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
    m_fileModel->setRepositoryRoot(repositoryRoot);
    m_fileModel->setFileStatusQualifier([](const QString &status, const QVariant &)
                                           -> VcsBase::SubmitFileModel::FileStatusHint
    {
        if (status == QLatin1String(Constants::FSTATUS_CREATED))
            return VcsBase::SubmitFileModel::FileAdded;
        if (status == QLatin1String(Constants::FSTATUS_MODIFIED))
            return VcsBase::SubmitFileModel::FileModified;
        if (status == QLatin1String(Constants::FSTATUS_DELETED))
            return VcsBase::SubmitFileModel::FileDeleted;
        if (status == QLatin1String(Constants::FSTATUS_RENAMED))
            return VcsBase::SubmitFileModel::FileRenamed;
        return VcsBase::SubmitFileModel::FileStatusUnknown;
    } );

    foreach (const VcsBase::VcsBaseClient::StatusItem &item, repoStatus)
        if (item.flags != QLatin1String("Unknown"))
            m_fileModel->addFile(item.file, item.flags);
    setFileModel(m_fileModel);
}
