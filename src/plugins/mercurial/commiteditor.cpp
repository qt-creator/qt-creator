/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "commiteditor.h"
#include "mercurialcommitwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QtCore/QDebug>

#include <QtCore/QDir> //TODO REMOVE WHEN BASE FILE CHANGES ARE PULLED

using namespace Mercurial::Internal;

CommitEditor::CommitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent)
        : VCSBase::VCSBaseSubmitEditor(parameters, new MercurialCommitWidget(parent)),
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
                             const QList<VCSBase::VCSBaseClient::StatusItem> &repoStatus)
{
    MercurialCommitWidget *mercurialWidget = commitWidget();
    if (!mercurialWidget)
        return;

    mercurialWidget->setFields(repositoryRoot.absoluteFilePath(), branch, userName, email);

    fileModel = new VCSBase::SubmitFileModel(this);

    //TODO Messy tidy this up
    QStringList shouldTrack;

    foreach (const VCSBase::VCSBaseClient::StatusItem &item, repoStatus) {
        if (item.flags == QLatin1String("Untracked"))
            shouldTrack.append(item.file);
        else
            fileModel->addFile(item.file, item.flags, false);
    }

    VCSBase::VCSBaseSubmitEditor::filterUntrackedFilesOfProject(repositoryRoot.absoluteFilePath(),
                                                                &shouldTrack);

    foreach (const QString &track, shouldTrack) {
        foreach (const VCSBase::VCSBaseClient::StatusItem &item, repoStatus) {
            if (item.file == track)
                fileModel->addFile(item.file, item.flags, false);
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
