/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include "branchinfo.h"
#include "commiteditor.h"
#include "constants.h"
#include "fossilcommitwidget.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace Fossil {
namespace Internal {

CommitEditor::CommitEditor() :
    VcsBase::VcsBaseSubmitEditor(new FossilCommitWidget)
{
    document()->setPreferredDisplayName(tr("Commit Editor"));
}

FossilCommitWidget *CommitEditor::commitWidget()
{
    return static_cast<FossilCommitWidget *>(widget());
}

void CommitEditor::setFields(const Utils::FilePath &repositoryRoot, const BranchInfo &branch,
                             const QStringList &tags, const QString &userName,
                             const QList<VcsBase::VcsBaseClient::StatusItem> &repoStatus)
{
    FossilCommitWidget *fossilWidget = commitWidget();
    QTC_ASSERT(fossilWidget, return);

    fossilWidget->setFields(repositoryRoot, branch, tags, userName);

    m_fileModel = new VcsBase::SubmitFileModel(this);
    m_fileModel->setRepositoryRoot(repositoryRoot);
    m_fileModel->setFileStatusQualifier([](const QString &status, const QVariant &) {
        if (status == Constants::FSTATUS_ADDED
            || status == Constants::FSTATUS_ADDED_BY_MERGE
            || status == Constants::FSTATUS_ADDED_BY_INTEGRATE) {
            return VcsBase::SubmitFileModel::FileAdded;
        } else if (status == Constants::FSTATUS_EDITED
            || status == Constants::FSTATUS_UPDATED_BY_MERGE
            || status == Constants::FSTATUS_UPDATED_BY_INTEGRATE) {
            return VcsBase::SubmitFileModel::FileModified;
        } else if (status == Constants::FSTATUS_DELETED) {
            return VcsBase::SubmitFileModel::FileDeleted;
        } else if (status == Constants::FSTATUS_RENAMED) {
            return VcsBase::SubmitFileModel::FileRenamed;
        }
        return VcsBase::SubmitFileModel::FileStatusUnknown;
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
