/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gitsubmiteditor.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitsubmiteditorwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDebug>
#include <QStringList>
#include <QTextCodec>
#include <QTimer>

static const char TASK_UPDATE_COMMIT[] = "Git.UpdateCommit";

using namespace VcsBase;

namespace Git {
namespace Internal {

class GitSubmitFileModel : public SubmitFileModel
{
public:
    GitSubmitFileModel(QObject *parent = 0) : SubmitFileModel(parent)
    { }

    void updateSelections(SubmitFileModel *source) override
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

CommitDataFetchResult CommitDataFetchResult::fetch(CommitType commitType, const QString &workingDirectory)
{
    CommitDataFetchResult result;
    result.commitData.commitType = commitType;
    QString commitTemplate;
    result.success = GitPlugin::client()->getCommitData(workingDirectory, &commitTemplate,
                                                        result.commitData, &result.errorMessage);
    return result;
}

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we sort apart the diff file lists
 * according to a type flag we add to the model. */

GitSubmitEditor::GitSubmitEditor(const VcsBaseSubmitEditorParameters *parameters) :
    VcsBaseSubmitEditor(parameters, new GitSubmitEditorWidget)
{
    connect(this, &VcsBaseSubmitEditor::diffSelectedRows, this, &GitSubmitEditor::slotDiffSelected);
    connect(submitEditorWidget(), &GitSubmitEditorWidget::show, this, &GitSubmitEditor::showCommit);
    connect(GitPlugin::instance()->versionControl(), &Core::IVersionControl::repositoryChanged,
            this, &GitSubmitEditor::forceUpdateFileModel);
    connect(&m_fetchWatcher, &QFutureWatcher<CommitDataFetchResult>::finished,
            this, &GitSubmitEditor::commitDataRetrieved);
}

GitSubmitEditor::~GitSubmitEditor()
{
}

GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget()
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

const GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget() const
{
    return static_cast<GitSubmitEditorWidget *>(widget());
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
        if (state & (UnmergedFile | UnmergedThem | UnmergedUs))
            return SubmitFileModel::FileUnmerged;
        if (state.testFlag(AddedFile) || state.testFlag(UntrackedFile))
            return SubmitFileModel::FileAdded;
        if (state.testFlag(ModifiedFile) || state.testFlag(TypeChangedFile))
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
    for (int row : rows) {
        const QString fileName = m_model->file(row);
        const FileStates state = static_cast<FileStates>(m_model->extraData(row).toInt());
        if (state & UnmergedFile) {
            unmergedFiles.push_back(fileName);
        } else if (state & StagedFile) {
            if (state & (RenamedFile | CopiedFile)) {
                const int arrow = fileName.indexOf(" -> ");
                if (arrow != -1) {
                    stagedFiles.push_back(fileName.left(arrow));
                    stagedFiles.push_back(fileName.mid(arrow + 4));
                    continue;
                }
            }
            stagedFiles.push_back(fileName);
        } else if (state == UntrackedFile) {
            Core::EditorManager::openEditor(m_workingDirectory + '/' + fileName);
        } else {
            unstagedFiles.push_back(fileName);
        }
    }
    if (!unstagedFiles.empty() || !stagedFiles.empty())
        GitPlugin::client()->diffFiles(m_workingDirectory, unstagedFiles, stagedFiles);
    if (!unmergedFiles.empty())
        GitPlugin::client()->merge(m_workingDirectory, unmergedFiles);
}

void GitSubmitEditor::showCommit(const QString &commit)
{
    if (!m_workingDirectory.isEmpty())
        GitPlugin::client()->show(m_workingDirectory, commit);
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
    m_fetchWatcher.setFuture(Utils::runAsync(&CommitDataFetchResult::fetch,
                                             m_commitType, m_workingDirectory));
    Core::ProgressManager::addTask(m_fetchWatcher.future(), tr("Refreshing Commit Data"),
                                   TASK_UPDATE_COMMIT);

    GitPlugin::client()->addFuture(m_fetchWatcher.future());
}

void GitSubmitEditor::forceUpdateFileModel()
{
    GitSubmitEditorWidget *w = submitEditorWidget();
    if (w->updateInProgress())
        QTimer::singleShot(10, this, [this] { forceUpdateFileModel(); });
    else
        updateFileModel();
}

void GitSubmitEditor::commitDataRetrieved()
{
    CommitDataFetchResult result = m_fetchWatcher.result();
    GitSubmitEditorWidget *w = submitEditorWidget();
    if (result.success) {
        setCommitData(result.commitData);
        w->refreshLog(m_workingDirectory);
        w->setEnabled(true);
    } else {
        // Nothing to commit left!
        VcsOutputWindow::appendError(result.errorMessage);
        m_model->clear();
        w->setEnabled(false);
    }
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
