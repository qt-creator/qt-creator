// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "commiteditor.h"
#include "mercurialcommitwidget.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

CommitEditor::CommitEditor() :
    VcsBaseSubmitEditor(new MercurialCommitWidget)
{
    document()->setPreferredDisplayName(tr("Commit Editor"));
}

MercurialCommitWidget *CommitEditor::commitWidget()
{
    return static_cast<MercurialCommitWidget *>(widget());
}

void CommitEditor::setFields(const QFileInfo &repositoryRoot, const QString &branch,
                             const QString &userName, const QString &email,
                             const QList<VcsBaseClient::StatusItem> &repoStatus)
{
    MercurialCommitWidget *mercurialWidget = commitWidget();
    if (!mercurialWidget)
        return;

    mercurialWidget->setFields(repositoryRoot.absoluteFilePath(), branch, userName, email);

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

    for (const QString &track : qAsConst(shouldTrack)) {
        for (const VcsBaseClient::StatusItem &item : repoStatus) {
            if (item.file == track)
                fileModel->addFile(item.file, item.flags, Unchecked);
        }
    }

    setFileModel(fileModel);
}

QString CommitEditor::committerInfo()
{
    return commitWidget()->committer();
}

QString CommitEditor::repoRoot()
{
    return commitWidget()->repoRoot();
}

} // namespace Internal
} // namespace Mercurial
