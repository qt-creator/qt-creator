/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "commiteditor.h"
#include "mercurialcommitwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QDebug>
#include <QDir> //TODO REMOVE WHEN BASE FILE CHANGES ARE PULLED

using namespace Mercurial::Internal;
using namespace VcsBase;

CommitEditor::CommitEditor(const VcsBaseSubmitEditorParameters *parameters, QWidget *parent)
        : VcsBaseSubmitEditor(parameters, new MercurialCommitWidget(parent)),
        fileModel(0)
{
    setDisplayName(tr("Commit Editor"));
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

    //TODO Messy tidy this up
    QStringList shouldTrack;

    foreach (const VcsBaseClient::StatusItem &item, repoStatus) {
        if (item.flags == QLatin1String("Untracked"))
            shouldTrack.append(item.file);
        else
            fileModel->addFile(item.file, item.flags, VcsBase::Unchecked);
    }

    VcsBaseSubmitEditor::filterUntrackedFilesOfProject(repositoryRoot.absoluteFilePath(),
                                                                &shouldTrack);

    foreach (const QString &track, shouldTrack) {
        foreach (const VcsBaseClient::StatusItem &item, repoStatus) {
            if (item.file == track)
                fileModel->addFile(item.file, item.flags, VcsBase::Unchecked);
        }
    }

    setFileModel(fileModel, repositoryRoot.absoluteFilePath());
}

QString CommitEditor::committerInfo()
{
    return commitWidget()->committer();
}

QString CommitEditor::repoRoot()
{
    return commitWidget()->repoRoot();
}
