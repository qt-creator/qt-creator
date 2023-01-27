// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbasesubmiteditor.h>

namespace VcsBase { class SubmitFileModel; }

namespace Fossil {
namespace Internal {

class BranchInfo;
class FossilCommitWidget;

class CommitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    CommitEditor();

    void setFields(const Utils::FilePath &repositoryRoot, const BranchInfo &branch,
                   const QStringList &tags, const QString &userName,
                   const QList<VcsBase::VcsBaseClient::StatusItem> &repoStatus);

    FossilCommitWidget *commitWidget();

private:
    VcsBase::SubmitFileModel *m_fileModel = nullptr;
};

} // namespace Internal
} // namespace Fossil
