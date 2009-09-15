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
