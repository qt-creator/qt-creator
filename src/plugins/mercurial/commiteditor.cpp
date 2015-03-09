/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion
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
#include "mercurialcommitwidget.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

CommitEditor::CommitEditor(const VcsBaseSubmitEditorParameters *parameters)
        : VcsBaseSubmitEditor(parameters, new MercurialCommitWidget),
        fileModel(0)
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
