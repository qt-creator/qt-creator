// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbasesubmiteditor.h>

#include <QFileInfo>

namespace VcsBase { class SubmitFileModel; }

namespace Mercurial::Internal {

class MercurialCommitWidget;

class CommitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    CommitEditor();

    void setFields(const QFileInfo &repositoryRoot, const QString &branch,
                   const QString &userName, const QString &email,
                   const QList<VcsBase::VcsBaseClient::StatusItem> &repoStatus);

    QString committerInfo() const;
    QString repoRoot() const;

private:
    MercurialCommitWidget *commitWidget() const;
    VcsBase::SubmitFileModel *fileModel = nullptr;
};

} // Mercurial::Internal
