// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "commiteditor.h"
#include "branchinfo.h"
#include "bazaarcommitwidget.h"
#include "constants.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>

using namespace Bazaar::Internal;

CommitEditor::CommitEditor() :
    VcsBase::VcsBaseSubmitEditor(new BazaarCommitWidget)
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
