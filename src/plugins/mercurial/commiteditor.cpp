// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commiteditor.h"

#include "mercurialcommitwidget.h"
#include "mercurialtr.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

using namespace VcsBase;
using namespace Utils;

namespace Mercurial::Internal {

CommitEditor::CommitEditor() :
    VcsBaseSubmitEditor(new MercurialCommitWidget)
{
    document()->setPreferredDisplayName(Tr::tr("Commit Editor"));
}

MercurialCommitWidget *CommitEditor::commitWidget() const
{
    return static_cast<MercurialCommitWidget *>(widget());
}

void CommitEditor::setFields(const FilePath &repositoryRoot, const QString &branch,
                             const QString &userName, const QString &email,
                             const QList<VcsBaseClient::StatusItem> &repoStatus)
{
    MercurialCommitWidget *mercurialWidget = commitWidget();
    if (!mercurialWidget)
        return;

    mercurialWidget->setFields(repositoryRoot.absoluteFilePath().toString(), branch, userName, email);

    fileModel = new SubmitFileModel(this);
    fileModel->setRepositoryRoot(repositoryRoot.absoluteFilePath());

    QStringList shouldTrack;

    for (const VcsBaseClient::StatusItem &item : repoStatus) {
        if (item.flags == QLatin1String("Untracked"))
            shouldTrack.append(item.file);
        else
            fileModel->addFile(item.file, item.flags, Unchecked);
    }

    VcsBaseSubmitEditor::filterUntrackedFilesOfProject(fileModel->repositoryRoot(), &shouldTrack);

    for (const QString &track : std::as_const(shouldTrack)) {
        for (const VcsBaseClient::StatusItem &item : repoStatus) {
            if (item.file == track)
                fileModel->addFile(item.file, item.flags, Unchecked);
        }
    }

    setFileModel(fileModel);
}

QString CommitEditor::committerInfo() const
{
    return commitWidget()->committer();
}

QString CommitEditor::repoRoot() const
{
    return commitWidget()->repoRoot();
}

} // Mercurial::Internal
