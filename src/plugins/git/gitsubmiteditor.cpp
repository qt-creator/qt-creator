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
#include <coreplugin/progressmanager/progressmanager.h>

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

Result<CommitData> fetchCommitData(CommitType commitType, const FilePath &workingDirectory)
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
    connect(versionControl(), &Core::IVersionControl::repositoryChanged,
            this, &GitSubmitEditor::forceUpdateFileModel);
    connect(&m_fetchWatcher, &QFutureWatcher<Result<CommitData>>::finished,
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
    using IVCF = Core::IVersionControl::FileState;

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
            return IVCF::Unmerged;
        if (state.testFlag(UntrackedFile))
            return IVCF::Untracked;
        if (state.testFlag(AddedFile))
            return IVCF::Added;
        if (state.testFlag(ModifiedFile) || state.testFlag(TypeChangedFile))
            return IVCF::Modified;
        if (state.testFlag(DeletedFile))
            return IVCF::Deleted;
        if (state.testFlag(RenamedFile))
            return IVCF::Renamed;
        return IVCF::Unknown;
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

void GitSubmitEditor::showLog(const QStringList &range)
{
    if (!m_workingDirectory.isEmpty())
        gitClient().log(m_workingDirectory, {}, false, range);
}

void GitSubmitEditor::addToGitignore(const FilePath &relativePath)
{
    const FilePath fullPath = m_workingDirectory.resolvePath(relativePath);
    const FilePath gitignorePath = gitClient().findGitignoreFor(fullPath);
    const QString pattern = "/" + relativePath.path();

    const TextEncoding fallbackEncoding = VcsBaseEditor::getEncoding(m_workingDirectory);
    TextFileFormat format;
    TextFileFormat::ReadResult readResult = format.readFile(gitignorePath, fallbackEncoding);

    if (readResult.code != TextFileFormat::ReadSuccess) {
        const QString message = Tr::tr("Cannot read \"%1\", reason %2.")
                                    .arg(gitignorePath.toUserOutput(), readResult.error);
        VcsOutputWindow::appendError(m_workingDirectory, message);
        return;
    }

    QStringList lines = readResult.content.split('\n');
    insertSorted(&lines, pattern);

    const Result<> writeResult = format.writeFile(gitignorePath, lines.join('\n'));
    if (!writeResult) {
        const QString message = Tr::tr("Cannot write \"%1\", reason %2.")
                                    .arg(gitignorePath.toUserOutput(), writeResult.error());
        VcsOutputWindow::appendError(m_workingDirectory, message);
    }
}

void GitSubmitEditor::performFileAction(const Utils::FilePath &filePath, FileAction action)
{
    if (m_workingDirectory.isEmpty())
        return;

    bool refresh = false;
    const FilePath fullPath = m_workingDirectory.pathAppended(filePath.toUrlishString());

    auto markAsResolved = [this, &refresh](const Utils::FilePath &filePath) {
        // Check if file still contains conflict markers
        if (!gitClient().isConflictFree(m_workingDirectory, filePath))
            return;

        // Otherwise mark as resolved
        gitClient().addFile(m_workingDirectory, filePath.toUrlishString());
        refresh = true;
    };

    switch (action) {
    case FileRevertAll:
    case FileRevertUnstaged:
    case FileRevertDeletion: {
        const QStringList files = {filePath.toUrlishString()};
        const bool revertStaging = action != FileRevertUnstaged;
        const bool success = gitClient().synchronousCheckoutFiles(m_workingDirectory, files,
                                                                  {}, nullptr, revertStaging);
        if (success) {
            const QString message = (action == FileRevertDeletion)
                ? Tr::tr("File \"%1\" recovered.\n").arg(filePath.toUserOutput())
                : Tr::tr("File \"%1\" reverted.\n").arg(filePath.toUserOutput());
            VcsOutputWindow::appendMessage(m_workingDirectory, message);
            refresh = true;
        }
        break;
    }

    case FileAddGitignore:
        addToGitignore(filePath);
        refresh = true;
        break;

    case FileCopyClipboard:
        setClipboardAndSelection(filePath.toUserOutput());
        break;

    case FileOpenEditor:
        Core::EditorManager::openEditor(fullPath);
        break;

    case FileStage:
        gitClient().addFile(m_workingDirectory, filePath.toUrlishString());
        refresh = true;
        break;

    case FileUnstage:
        gitClient().synchronousReset(m_workingDirectory, {filePath.toUrlishString()});
        refresh = true;
        break;

    case FileMergeTool:
        gitClient().merge(m_workingDirectory, {filePath.toUrlishString()});
        break;

    case FileMergeResolved:
        markAsResolved(filePath);
        break;

    case FileMergeOurs:
        gitClient().synchronousCheckoutFiles(m_workingDirectory, {filePath.toUrlishString()},
                                             "--ours");
        markAsResolved(filePath);
        break;

    case FileMergeTheirs:
        gitClient().synchronousCheckoutFiles(m_workingDirectory, {filePath.toUrlishString()},
                                             "--theirs");
        markAsResolved(filePath);
        break;

    case FileMergeRecover:
        gitClient().addFile(m_workingDirectory, filePath.toUrlishString());
        refresh = true;
        break;

    case FileMergeRemove:
        gitClient().synchronousDelete(m_workingDirectory, false, {filePath.toUrlishString()});
        refresh = true;
        break;
    }

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
    // TODO: Check if fetch works OK from separate thread, refactor otherwise
    m_fetchWatcher.setFuture(Utils::asyncRun(&fetchCommitData,
                                             m_commitType, m_workingDirectory));
    Core::ProgressManager::addTask(m_fetchWatcher.future(), Tr::tr("Refreshing Commit Data"),
                                   TASK_UPDATE_COMMIT);

    Utils::futureSynchronizer()->addFuture(m_fetchWatcher.future());
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
    const Result<CommitData> result = m_fetchWatcher.result();
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
