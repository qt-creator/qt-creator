/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "commiteditor.h"
#include "mercurialcommitwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QtCore/QDebug>

#include <QDir> //TODO REMOVE WHEN BASE FILE CHANGES ARE PULLED

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
                             const QList<QPair<QString, QString> > &repoStatus)
{
    MercurialCommitWidget *mercurialWidget = commitWidget();
    if (!mercurialWidget)
        return;

    mercurialWidget->setFields(repositoryRoot.absoluteFilePath(), branch, userName, email);

    fileModel = new VCSBase::SubmitFileModel(this);

    //TODO Messy tidy this up
    typedef QPair<QString, QString> PAIR;
    QStringList shouldTrack;

    foreach (PAIR status, repoStatus) {
        if (status.first == "Untracked")
            shouldTrack.append(status.second);
        else
            fileModel->addFile(status.second, status.first, false);
    }

    VCSBase::VCSBaseSubmitEditor::filterUntrackedFilesOfProject(repositoryRoot.absoluteFilePath(),
                                                                &shouldTrack);

    foreach (QString track, shouldTrack) {
        foreach (PAIR status, repoStatus) {
            if (status.second == track)
                fileModel->addFile(status.second, status.first, false);
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
