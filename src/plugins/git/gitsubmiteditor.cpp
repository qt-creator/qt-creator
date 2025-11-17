// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsubmiteditor.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "gitsubmiteditorwidget.h"
#include "gittr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QStringList>
#include <QTimer>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

const char TASK_UPDATE_COMMIT[] = "Git.UpdateCommit";

class GitSubmitFileModel : public SubmitFileModel
{
public:
    GitSubmitFileModel(QObject *parent = nullptr) : SubmitFileModel(parent)
    { }

    void updateSelections(SubmitFileModel *source) override
    {
        QTC_ASSERT(source, return);
        auto gitSource = static_cast<GitSubmitFileModel *>(source);
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

static Result<CommitData> fetchCommitData(CommitType commitType, const FilePath &workingDirectory)
{
    return gitClient().getCommitData(commitType, workingDirectory);
}

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we sort apart the diff file lists
 * according to a type flag we add to the model. */

GitSubmitEditor::GitSubmitEditor() :
    VcsBaseSubmitEditor(new GitSubmitEditorWidget)
{
    connect(this, &VcsBaseSubmitEditor::diffSelectedRows, this, &GitSubmitEditor::slotDiffSelected);
    connect(submitEditorWidget(), &GitSubmitEditorWidget::showRequested, this, &GitSubmitEditor::showCommit);
    connect(submitEditorWidget(), &GitSubmitEditorWidget::logRequested, this, &GitSubmitEditor::showLog);
    connect(submitEditorWidget(), &GitSubmitEditorWidget::fileActionRequested,
            this, &GitSubmitEditor::performFileAction);
    connect(versionControl(), &IVersionControl::repositoryChanged,
            this, &GitSubmitEditor::forceUpdateFileModel);
}

GitSubmitEditor::~GitSubmitEditor() = default;

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
    using FileState = Core::VcsFileState;

    m_commitEncoding = d.commitEncoding;
    m_workingDirectory = d.panelInfo.repository;
    m_commitType = d.commitType;
    m_amenHash = d.amendHash;

    GitSubmitEditorWidget *w = submitEditorWidget();
    w->initialize(m_workingDirectory, d);
    w->setHasUnmerged(false);

    setEmptyFileListEnabled(m_commitType == AmendCommit); // Allow for just correcting the message

    m_model = new GitSubmitFileModel(this);
    m_model->setRepositoryRoot(d.panelInfo.repository);
    m_model->setFileStatusQualifier([](const QString &, const QVariant &extraData) {
        const FileStates state = static_cast<FileStates>(extraData.toInt());
        if (state & (UnmergedFile | UnmergedThem | UnmergedUs))
            return FileState::Unmerged;
        if (state.testFlag(UntrackedFile))
            return FileState::Untracked;
        if (state.testFlag(AddedFile))
            return FileState::Added;
        if (state.testFlag(ModifiedFile) || state.testFlag(TypeChangedFile))
            return FileState::Modified;
        if (state.testFlag(DeletedFile))
            return FileState::Deleted;
        if (state.testFlag(RenamedFile))
            return FileState::Renamed;
        return FileState::Unknown;
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
                const QStringList files = gitClient().splitRenamedFilePattern(fileName);
                if (files.size() == 2) {
                    stagedFiles.push_back(files.at(0));
                    stagedFiles.push_back(files.at(1));
                    continue;
                }
            }
            stagedFiles.push_back(fileName);
        } else if (state == UntrackedFile) {
            EditorManager::openEditor(m_workingDirectory.pathAppended(fileName));
        } else {
            unstagedFiles.push_back(fileName);
        }
    }
    if (!unstagedFiles.empty() || !stagedFiles.empty())
        gitClient().diffFiles(m_workingDirectory, unstagedFiles, stagedFiles);
    if (!unmergedFiles.empty())
        gitClient().merge(m_workingDirectory, unmergedFiles);
}

void GitSubmitEditor::showCommit(const QString &commit)
{
    if (!m_workingDirectory.isEmpty())
        gitClient().show(m_workingDirectory, commit);
}

void GitSubmitEditor::showLog(const QStringList &range)
{
    if (!m_workingDirectory.isEmpty())
        gitClient().log(m_workingDirectory, {}, false, range);
}

void GitSubmitEditor::performFileAction(const Utils::FilePath &filePath, IVersionControl::FileAction action)
{
    const bool refresh = Git::Internal::performFileAction(m_workingDirectory, filePath, action);
    if (refresh)
        QTimer::singleShot(100, this, &GitSubmitEditor::forceUpdateFileModel);
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

    using ResultType = Result<CommitData>;
    // TODO: Check if fetch works OK from separate thread, refactor otherwise
    const auto onSetup = [this](Async<ResultType> &task) {
        task.setConcurrentCallData(&fetchCommitData,m_commitType,
                                   m_workingDirectory);
    };
    const auto onDone = [this](const Async<ResultType> &task) {
        const ResultType result = task.result();
        GitSubmitEditorWidget *w = submitEditorWidget();
        if (result) {
            setCommitData(result.value());
            w->refreshLog(m_workingDirectory);
            w->setEnabled(true);
        } else {
            // Nothing to commit left!
            VcsOutputWindow::appendError(m_workingDirectory, result.error());
            m_model->clear();
            w->setEnabled(false);
        }
        w->setUpdateInProgress(false);
    };
    const auto onTreeSetup = [](QTaskTree &taskTree) {
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(Tr::tr("Refreshing Commit Data"));
        progress->setId(TASK_UPDATE_COMMIT);
    };
    m_taskTreeRunner.start(
        {AsyncTask<ResultType>(onSetup, onDone, CallDone::OnSuccess)},
        onTreeSetup);
}

void GitSubmitEditor::forceUpdateFileModel()
{
    GitSubmitEditorWidget *w = submitEditorWidget();
    if (w->updateInProgress())
        QTimer::singleShot(10, this, [this] { forceUpdateFileModel(); });
    else
        updateFileModel();
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return submitEditorWidget()->panelData();
}

QString GitSubmitEditor::amendHash() const
{
    const QString commit = submitEditorWidget()->amendHash();
    return commit.isEmpty() ? m_amenHash : commit;
}

QByteArray GitSubmitEditor::fileContents() const
{
    const QString &text = description();

    // Do the encoding convert, When use user-defined encoding
    // e.g. git config --global i18n.commitencoding utf-8
    if (m_commitEncoding.isValid())
        return m_commitEncoding.encode(text);

    // Using utf-8 as the default encoding
    return text.toUtf8();
}

} // Git::Internal
