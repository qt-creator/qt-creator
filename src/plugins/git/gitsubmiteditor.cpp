/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "gitsubmiteditor.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitsubmiteditorwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDebug>
#include <QStringList>
#include <QTextCodec>
#include <QtConcurrentRun>

static const char TASK_UPDATE_COMMIT[] = "Git.UpdateCommit";

using namespace VcsBase;

namespace Git {
namespace Internal {

class GitSubmitFileModel : public SubmitFileModel
{
public:
    GitSubmitFileModel(QObject *parent = 0) : SubmitFileModel(parent)
    { }

    void updateSelections(SubmitFileModel *source)
    {
        QTC_ASSERT(source, return);
        GitSubmitFileModel *gitSource = static_cast<GitSubmitFileModel *>(source);
        int j = 0;
        for (int i = 0; i < rowCount() && j < source->rowCount(); ++i) {
            CommitData::StateFilePair stateFile = stateFilePair(i);
            for (; j < source->rowCount(); ++j) {
                CommitData::StateFilePair sourceStateFile = gitSource->stateFilePair(j);
                if (stateFile == sourceStateFile) {
                    if (isCheckable(i) && source->isCheckable(j))
                        setChecked(i, source->checked(j));
                    break;
                } else if (((stateFile.first & UntrackedFile)
                            == (sourceStateFile.first & UntrackedFile))
                           && (stateFile < sourceStateFile)) {
                    break;
                }
            }
        }
    }

private:
    CommitData::StateFilePair stateFilePair(int row) const
    {
        return CommitData::StateFilePair(static_cast<FileStates>(extraData(row).toInt()), file(row));
    }
};

class CommitDataFetcher : public QObject
{
    Q_OBJECT

public:
    CommitDataFetcher(CommitType commitType, const QString &workingDirectory) :
        m_commitData(commitType),
        m_workingDirectory(workingDirectory)
    {
    }

    void start()
    {
        GitClient *client = GitPlugin::instance()->gitClient();
        QString commitTemplate;
        bool success = client->getCommitData(m_workingDirectory, &commitTemplate,
                                             m_commitData, &m_errorMessage);
        emit finished(success);
    }

    const CommitData &commitData() const { return m_commitData; }
    const QString &errorMessage() const { return m_errorMessage; }

signals:
    void finished(bool result);

private:
    CommitData m_commitData;
    QString m_workingDirectory;
    QString m_errorMessage;
};

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we sort apart the diff file lists
 * according to a type flag we add to the model. */

GitSubmitEditor::GitSubmitEditor(const VcsBaseSubmitEditorParameters *parameters) :
    VcsBaseSubmitEditor(parameters, new GitSubmitEditorWidget),
    m_model(0),
    m_commitEncoding(0),
    m_commitType(SimpleCommit),
    m_firstUpdate(true),
    m_commitDataFetcher(0)
{
    connect(this, &VcsBaseSubmitEditor::diffSelectedRows,
            this, &GitSubmitEditor::slotDiffSelected);
    connect(submitEditorWidget(), SIGNAL(show(QString)), this, SLOT(showCommit(QString)));
}

GitSubmitEditor::~GitSubmitEditor()
{
    resetCommitDataFetcher();
}

GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget()
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

const GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget() const
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

void GitSubmitEditor::resetCommitDataFetcher()
{
    if (!m_commitDataFetcher)
        return;
    disconnect(m_commitDataFetcher, SIGNAL(finished(bool)), this, SLOT(commitDataRetrieved(bool)));
    connect(m_commitDataFetcher, SIGNAL(finished(bool)), m_commitDataFetcher, SLOT(deleteLater()));
}

void GitSubmitEditor::setCommitData(const CommitData &d)
{
    m_commitEncoding = d.commitEncoding;
    m_workingDirectory = d.panelInfo.repository;
    m_commitType = d.commitType;
    m_amendSHA1 = d.amendSHA1;

    GitSubmitEditorWidget *w = submitEditorWidget();
    w->initialize(m_commitType, m_workingDirectory, d.panelData, d.panelInfo, d.enablePush);
    w->setHasUnmerged(false);

    setEmptyFileListEnabled(m_commitType == AmendCommit); // Allow for just correcting the message

    m_model = new GitSubmitFileModel(this);
    m_model->setRepositoryRoot(d.panelInfo.repository);
    m_model->setFileStatusQualifier([](const QString &, const QVariant &extraData)
                                    -> SubmitFileModel::FileStatusHint
    {
        const FileStates state = static_cast<FileStates>(extraData.toInt());
        if (state.testFlag(AddedFile) || state.testFlag(UntrackedFile))
            return SubmitFileModel::FileAdded;
        if (state.testFlag(ModifiedFile))
            return SubmitFileModel::FileModified;
        if (state.testFlag(DeletedFile))
            return SubmitFileModel::FileDeleted;
        if (state.testFlag(RenamedFile))
            return SubmitFileModel::FileRenamed;
        return SubmitFileModel::FileStatusUnknown;
    } );

    if (!d.files.isEmpty()) {
        for (QList<CommitData::StateFilePair>::const_iterator it = d.files.constBegin();
             it != d.files.constEnd(); ++it) {
            const FileStates state = it->first;
            const QString file = it->second;
            CheckMode checkMode;
            if (state & UnmergedFile) {
                checkMode = Uncheckable;
                w->setHasUnmerged(true);
            } else if (state & StagedFile) {
                checkMode = Checked;
            } else {
                checkMode = Unchecked;
            }
            m_model->addFile(file, CommitData::stateDisplayName(state), checkMode,
                             QVariant(static_cast<int>(state)));
        }
    }
    setFileModel(m_model);
}

void GitSubmitEditor::slotDiffSelected(const QList<int> &rows)
{
    // Sort it apart into unmerged/staged/unstaged files
    QStringList unmergedFiles;
    QStringList unstagedFiles;
    QStringList stagedFiles;
    foreach (int row, rows) {
        const QString fileName = m_model->file(row);
        const FileStates state = static_cast<FileStates>(m_model->extraData(row).toInt());
        if (state & UnmergedFile)
            unmergedFiles.push_back(fileName);
        else if (state & StagedFile)
            stagedFiles.push_back(fileName);
        else if (state == UntrackedFile)
            Core::EditorManager::openEditor(m_workingDirectory + QLatin1Char('/') + fileName);
        else
            unstagedFiles.push_back(fileName);
    }
    if (!unstagedFiles.empty() || !stagedFiles.empty())
        emit diff(unstagedFiles, stagedFiles);
    if (!unmergedFiles.empty())
        emit merge(unmergedFiles);
}

void GitSubmitEditor::showCommit(const QString &commit)
{
    if (!m_workingDirectory.isEmpty())
        emit show(m_workingDirectory, commit);
}

void GitSubmitEditor::updateFileModel()
{
    // Commit data is set when the editor is initialized, and updateFileModel immediately follows,
    // when the editor is activated. Avoid another call to git status
    if (m_firstUpdate) {
        m_firstUpdate = false;
        return;
    }
    GitSubmitEditorWidget *w = submitEditorWidget();
    if (w->updateInProgress() || m_workingDirectory.isEmpty())
        return;
    w->setUpdateInProgress(true);
    resetCommitDataFetcher();
    m_commitDataFetcher = new CommitDataFetcher(m_commitType, m_workingDirectory);
    connect(m_commitDataFetcher, SIGNAL(finished(bool)), this, SLOT(commitDataRetrieved(bool)));
    QFuture<void> future = QtConcurrent::run(m_commitDataFetcher, &CommitDataFetcher::start);
    Core::ProgressManager::addTask(future, tr("Refreshing Commit Data"), TASK_UPDATE_COMMIT);

    GitPlugin::instance()->gitClient()->addFuture(future);
}

void GitSubmitEditor::commitDataRetrieved(bool success)
{
    GitSubmitEditorWidget *w = submitEditorWidget();
    if (success) {
        setCommitData(m_commitDataFetcher->commitData());
        w->refreshLog(m_workingDirectory);
        w->setEnabled(true);
    } else {
        // Nothing to commit left!
        VcsOutputWindow::appendError(m_commitDataFetcher->errorMessage());
        m_model->clear();
        w->setEnabled(false);
    }
    m_commitDataFetcher->deleteLater();
    m_commitDataFetcher = 0;
    w->setUpdateInProgress(false);
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return submitEditorWidget()->panelData();
}

QString GitSubmitEditor::amendSHA1() const
{
    QString commit = submitEditorWidget()->amendSHA1();
    return commit.isEmpty() ? m_amendSHA1 : commit;
}

QByteArray GitSubmitEditor::fileContents() const
{
    const QString &text = description();

    // Do the encoding convert, When use user-defined encoding
    // e.g. git config --global i18n.commitencoding utf-8
    if (m_commitEncoding)
        return m_commitEncoding->fromUnicode(text);

    // Using utf-8 as the default encoding
    return text.toUtf8();
}

} // namespace Internal
} // namespace Git

#include "gitsubmiteditor.moc"
