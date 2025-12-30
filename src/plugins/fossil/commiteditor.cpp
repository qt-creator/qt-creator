// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commiteditor.h"

#include "branchinfo.h"
#include "constants.h"
#include "fossilcommitwidget.h"
#include "fossiltr.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace Fossil {
namespace Internal {

CommitEditor::CommitEditor() :
    VcsBase::VcsBaseSubmitEditor(new FossilCommitWidget)
{
    document()->setPreferredDisplayName(Tr::tr("Commit Editor"));
}

FossilCommitWidget *CommitEditor::commitWidget()
{
    return static_cast<FossilCommitWidget *>(widget());
}

void CommitEditor::setFields(const Utils::FilePath &repositoryRoot, const BranchInfo &branch,
                             const QStringList &tags, const QString &userName,
                             const QList<VcsBase::VcsBaseClient::StatusItem> &repoStatus)
{
    using FileState = Core::VcsFileState;
    FossilCommitWidget *fossilWidget = commitWidget();
    QTC_ASSERT(fossilWidget, return);

    fossilWidget->setFields(repositoryRoot, branch, tags, userName);

    m_fileModel = new VcsBase::SubmitFileModel(this);
    m_fileModel->setRepositoryRoot(repositoryRoot);
    m_fileModel->setFileStatusQualifier([](const QString &status, const QVariant &) {
        if (status == Constants::FSTATUS_NEW) {
            return FileState::Untracked;
        } else if (status == Constants::FSTATUS_ADDED
            || status == Constants::FSTATUS_ADDED_BY_MERGE
            || status == Constants::FSTATUS_ADDED_BY_INTEGRATE) {
            return FileState::Added;
        } else if (status == Constants::FSTATUS_EDITED
            || status == Constants::FSTATUS_UPDATED_BY_MERGE
            || status == Constants::FSTATUS_UPDATED_BY_INTEGRATE) {
            return FileState::Modified;
        } else if (status == Constants::FSTATUS_DELETED) {
            return FileState::Deleted;
        } else if (status == Constants::FSTATUS_RENAMED) {
            return FileState::Renamed;
        }
        return FileState::Unknown;
    });

    const QList<VcsBase::VcsBaseClient::StatusItem> toAdd = Utils::filtered(repoStatus,
                [](const VcsBase::VcsBaseClient::StatusItem &item)
    { return item.flags != Constants::FSTATUS_UNKNOWN; });
    for (const VcsBase::VcsBaseClient::StatusItem &item : toAdd)
        m_fileModel->addFile(item.file, item.flags);

    setFileModel(m_fileModel);
}

} // namespace Internal
} // namespace Fossil
