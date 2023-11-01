// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsubmiteditor.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "gitsubmiteditorwidget.h"
#include "gittr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/async.h>
#include <utils/qtcassert.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDebug>
#include <QStringList>
#include <QTextCodec>
#include <QTimer>

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

CommitDataFetchResult CommitDataFetchResult::fetch(CommitType commitType, const FilePath &workingDirectory)
{
    CommitDataFetchResult result;
    result.commitData.commitType = commitType;
    QString commitTemplate;
    result.success = gitClient().getCommitData(
                workingDirectory, &commitTemplate, result.commitData, &result.errorMessage);
    return result;
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
    connect(GitPlugin::versionControl(), &Core::IVersionControl::repositoryChanged,
            this, &GitSubmitEditor::forceUpdateFileModel);
    connect(&m_fetchWatcher, &QFutureWatcher<CommitDataFetchResult>::finished,
            this, &GitSubmitEditor::commitDataRetrieved);
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
    m_commitEncoding = d.commitEncoding;
    m_workingDirectory = d.panelInfo.repository;
    m_commitType = d.commitType;
    m_amendSHA1 = d.amendSHA1;

    GitSubmitEditorWidget *w = submitEditorWidget();
    w->initialize(m_workingDirectory, d);
    w->setHasUnmerged(false);

    setEmptyFileListEnabled(m_commitType == AmendCommit); // Allow for just correcting the message

    m_model = new GitSubmitFileModel(this);
    m_model->setRepositoryRoot(d.panelInfo.repository);
    m_model->setFileStatusQualifier([](const QString &, const QVariant &extraData) {
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
            Core::EditorManager::openEditor(m_workingDirectory.pathAppended(fileName));
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
    // TODO: Check if fetch works OK from separate thread, refactor otherwise
    m_fetchWatcher.setFuture(Utils::asyncRun(&CommitDataFetchResult::fetch,
                                             m_commitType, m_workingDirectory));
    Core::ProgressManager::addTask(m_fetchWatcher.future(), Tr::tr("Refreshing Commit Data"),
                                   TASK_UPDATE_COMMIT);

    ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(m_fetchWatcher.future());
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
    const QString commit = submitEditorWidget()->amendSHA1();
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

} // Git::Internal
