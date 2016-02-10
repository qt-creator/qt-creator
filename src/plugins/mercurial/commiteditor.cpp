/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "commiteditor.h"
#include "mercurialcommitwidget.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

CommitEditor::CommitEditor(const VcsBaseSubmitEditorParameters *parameters) :
    VcsBaseSubmitEditor(parameters, new MercurialCommitWidget)
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

    foreach (const VcsBaseClient::StatusItem &item, repoStatus) {
        if (item.flags == QLatin1String("Untracked"))
            shouldTrack.append(item.file);
        else
            fileModel->addFile(item.file, item.flags, Unchecked);
    }

    VcsBaseSubmitEditor::filterUntrackedFilesOfProject(fileModel->repositoryRoot(), &shouldTrack);

    foreach (const QString &track, shouldTrack) {
        foreach (const VcsBaseClient::StatusItem &item, repoStatus) {
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
