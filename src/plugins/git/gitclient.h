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

#pragma once

#include "gitsettings.h"
#include "commitdata.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcsbaseclient.h>

#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QProcessEnvironment;
class QMenu;
QT_END_NAMESPACE

namespace Core { class ICore; }

namespace VcsBase {
class VcsCommand;
class SubmitFileModel;
class VcsBaseEditorWidget;
}

namespace DiffEditor {
class ChunkSelection;
class DiffEditorController;
}

namespace Git {
namespace Internal {

class CommitData;
class GitBaseDiffEditorController;
class GitSubmitEditorPanelData;
class Stash;

enum StatusMode
{
    ShowAll = 0,
    NoUntracked = 1,
    NoSubmodules = 2
};

enum StashFlag {
    Default        = 0x00, /* Prompt and do not allow unstashed */
    AllowUnstashed = 0x01,
    NoPrompt       = 0x02
};

enum PushFailure {
    Unknown,
    NonFastForward,
    NoRemoteBranch
};

class SubmoduleData
{
public:
    QString dir;
    QString url;
    QString ignore;
};

using SubmoduleDataMap = QMap<QString, SubmoduleData>;

class UpstreamStatus
{
public:
    UpstreamStatus() = default;
    UpstreamStatus(int ahead, int behind) : ahead(ahead), behind(behind) {}

    int ahead = 0;
    int behind = 0;
};

class GitClient : public VcsBase::VcsBaseClientImpl
{
    Q_OBJECT

public:
    enum CommandInProgress { NoCommand, Revert, CherryPick,
                             Rebase, Merge, RebaseMerge };

    class StashInfo
    {
    public:
        StashInfo() = default;
        enum StashResult { StashUnchanged, StashCanceled, StashFailed,
                           Stashed, NotStashed /* User did not want it */ };

        bool init(const Utils::FilePath &workingDirectory, const QString &command,
                  StashFlag flag = Default, PushAction pushAction = NoPush);
        bool stashingFailed() const;
        void end();
        StashResult result() const { return m_stashResult; }
        QString stashMessage() const { return m_message; }

    private:
        void stashPrompt(const QString &command, const QString &statusOutput, QString *errorMessage);
        void executeStash(const QString &command, QString *errorMessage);

        StashResult m_stashResult = NotStashed;
        QString m_message;
        Utils::FilePath m_workingDir;
        StashFlag m_flags = Default;
        PushAction m_pushAction = NoPush;
    };

    explicit GitClient(GitSettings *settings);
    static GitClient *instance();
    static GitSettings &settings();

    Utils::FilePath vcsBinary() const override;
    unsigned gitVersion(QString *errorMessage = nullptr) const;

    VcsBase::VcsCommand *vcsExecAbortable(const Utils::FilePath &workingDirectory,
                                          const QStringList &arguments,
                                          bool isRebase = false,
                                          QString abortCommand = {});

    Utils::FilePath findRepositoryForDirectory(const Utils::FilePath &directory) const;
    QString findGitDirForRepository(const Utils::FilePath &repositoryDir) const;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const;
    Utils::FilePaths unmanagedFiles(const Utils::FilePaths &filePaths) const;

    void diffFile(const Utils::FilePath &workingDirectory, const QString &fileName) const;
    void diffFiles(const Utils::FilePath &workingDirectory,
                   const QStringList &unstagedFileNames,
                   const QStringList &stagedFileNames) const;
    void diffProject(const Utils::FilePath &workingDirectory,
                     const QString &projectDirectory) const;
    void diffRepository(const Utils::FilePath &workingDirectory) const
    {
        return diffRepository(workingDirectory, {}, {});
    }
    void diffRepository(const Utils::FilePath &workingDirectory,
                        const QString &leftCommit,
                        const QString &rightCommit) const;
    void diffBranch(const Utils::FilePath &workingDirectory,
                    const QString &branchName) const;
    void merge(const Utils::FilePath &workingDirectory, const QStringList &unmergedFileNames = {});

    void status(const Utils::FilePath &workingDirectory) const;
    void log(const Utils::FilePath &workingDirectory, const QString &fileName = {},
             bool enableAnnotationContextMenu = false, const QStringList &args = {});
    void reflog(const Utils::FilePath &workingDirectory, const QString &branch = {});
    VcsBase::VcsBaseEditorWidget *annotate(const Utils::FilePath &workingDir, const QString &file,
                                           const QString &revision = {}, int lineNumber = -1,
                                           const QStringList &extraOptions = {}) override;
    void reset(const Utils::FilePath &workingDirectory, const QString &argument, const QString &commit = {});
    void removeStaleRemoteBranches(const Utils::FilePath &workingDirectory, const QString &remote);
    void recoverDeletedFiles(const Utils::FilePath &workingDirectory);
    void addFile(const Utils::FilePath &workingDirectory, const QString &fileName);
    bool synchronousLog(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                        QString *output, QString *errorMessage = nullptr,
                        unsigned flags = 0);
    bool synchronousAdd(const Utils::FilePath &workingDirectory, const QStringList &files,
                        const QStringList &extraOptions = {});
    bool synchronousDelete(const Utils::FilePath &workingDirectory,
                           bool force,
                           const QStringList &files);
    bool synchronousMove(const Utils::FilePath &workingDirectory,
                         const QString &from,
                         const QString &to);
    bool synchronousReset(const Utils::FilePath &workingDirectory, const QStringList &files = {},
                          QString *errorMessage = nullptr);
    bool synchronousCleanList(const Utils::FilePath &workingDirectory, const QString &modulePath,
                              QStringList *files, QStringList *ignoredFiles, QString *errorMessage);
    bool synchronousApplyPatch(const Utils::FilePath &workingDirectory, const QString &file,
                               QString *errorMessage, const QStringList &extraArguments = {});
    bool synchronousInit(const Utils::FilePath &workingDirectory);
    bool synchronousCheckoutFiles(const Utils::FilePath &workingDirectory, QStringList files = {},
                                  QString revision = {}, QString *errorMessage = nullptr,
                                  bool revertStaging = true);
    enum class StashMode { NoStash, TryStash };
    VcsBase::VcsCommand *checkout(const Utils::FilePath &workingDirectory, const QString &ref,
                                  StashMode stashMode = StashMode::TryStash);

    QStringList setupCheckoutArguments(const Utils::FilePath &workingDirectory, const QString &ref);
    void updateSubmodulesIfNeeded(const Utils::FilePath &workingDirectory, bool prompt);

    // Do a stash and return identier.
    enum { StashPromptDescription = 0x1, StashImmediateRestore = 0x2, StashIgnoreUnchanged = 0x4 };
    QString synchronousStash(const Utils::FilePath &workingDirectory,
                             const QString &messageKeyword = {},
                             unsigned flags = 0, bool *unchanged = nullptr) const;

    bool executeSynchronousStash(const Utils::FilePath &workingDirectory,
                                 const QString &message = {},
                                 bool unstagedOnly = false,
                                 QString *errorMessage = nullptr) const;
    bool synchronousStashRestore(const Utils::FilePath &workingDirectory,
                                 const QString &stash,
                                 bool pop = false,
                                 const QString &branch = {}) const;
    bool synchronousStashRemove(const Utils::FilePath &workingDirectory,
                                const QString &stash = {},
                                QString *errorMessage = nullptr) const;
    bool synchronousBranchCmd(const Utils::FilePath &workingDirectory, QStringList branchArgs,
                              QString *output, QString *errorMessage) const;
    bool synchronousTagCmd(const Utils::FilePath &workingDirectory, QStringList tagArgs,
                           QString *output, QString *errorMessage) const;
    bool synchronousForEachRefCmd(const Utils::FilePath &workingDirectory, QStringList args,
                               QString *output, QString *errorMessage = nullptr) const;
    VcsBase::VcsCommand *asyncForEachRefCmd(const Utils::FilePath &workingDirectory, QStringList args) const;
    bool synchronousRemoteCmd(const Utils::FilePath &workingDirectory, QStringList remoteArgs,
                              QString *output = nullptr, QString *errorMessage = nullptr,
                              bool silent = false) const;

    QMap<QString,QString> synchronousRemotesList(const Utils::FilePath &workingDirectory,
                                                 QString *errorMessage = nullptr) const;
    QStringList synchronousSubmoduleStatus(const Utils::FilePath &workingDirectory,
                                           QString *errorMessage = nullptr) const;
    SubmoduleDataMap submoduleList(const Utils::FilePath &workingDirectory) const;
    QByteArray synchronousShow(const Utils::FilePath &workingDirectory, const QString &id,
                               unsigned flags = 0) const;

    bool synchronousRevListCmd(const Utils::FilePath &workingDirectory, const QStringList &extraArguments,
                               QString *output, QString *errorMessage = nullptr) const;

    bool synchronousParentRevisions(const Utils::FilePath &workingDirectory,
                                    const QString &revision,
                                    QStringList *parents,
                                    QString *errorMessage) const;
    QString synchronousShortDescription(const Utils::FilePath &workingDirectory, const QString &revision) const;
    QString synchronousShortDescription(const Utils::FilePath &workingDirectory, const QString &revision,
                                     const QString &format) const;

    QString synchronousCurrentLocalBranch(const Utils::FilePath &workingDirectory) const;

    bool synchronousHeadRefs(const Utils::FilePath &workingDirectory, QStringList *output,
                             QString *errorMessage = nullptr) const;
    QString synchronousTopic(const Utils::FilePath &workingDirectory) const;
    bool synchronousRevParseCmd(const Utils::FilePath &workingDirectory, const QString &ref,
                                QString *output, QString *errorMessage = nullptr) const;
    QString synchronousTopRevision(const Utils::FilePath &workingDirectory, QDateTime *dateTime = nullptr);
    void synchronousTagsForCommit(const Utils::FilePath &workingDirectory, const QString &revision,
                                  QString &precedes, QString &follows) const;
    bool isRemoteCommit(const Utils::FilePath &workingDirectory, const QString &commit);
    bool isFastForwardMerge(const Utils::FilePath &workingDirectory, const QString &branch);

    void fetch(const Utils::FilePath &workingDirectory, const QString &remote);
    void pull(const Utils::FilePath &workingDirectory, bool rebase);
    void push(const Utils::FilePath &workingDirectory, const QStringList &pushArgs = {});
    bool synchronousMerge(const Utils::FilePath &workingDirectory, const QString &branch,
                          bool allowFastForward = true);
    bool canRebase(const Utils::FilePath &workingDirectory) const;
    void rebase(const Utils::FilePath &workingDirectory, const QString &argument);
    void cherryPick(const Utils::FilePath &workingDirectory, const QString &argument);
    void revert(const Utils::FilePath &workingDirectory, const QString &argument);

    bool synchronousRevert(const Utils::FilePath &workingDirectory, const QString &commit);
    bool synchronousCherryPick(const Utils::FilePath &workingDirectory, const QString &commit);
    void interactiveRebase(const Utils::FilePath &workingDirectory, const QString &commit, bool fixup);
    void synchronousAbortCommand(const Utils::FilePath &workingDir, const QString &abortCommand);
    QString synchronousTrackingBranch(const Utils::FilePath &workingDirectory,
                                      const QString &branch = {});
    bool synchronousSetTrackingBranch(const Utils::FilePath &workingDirectory,
                                      const QString &branch,
                                      const QString &tracking);

    // git svn support (asynchronous).
    void synchronousSubversionFetch(const Utils::FilePath &workingDirectory) const;
    void subversionLog(const Utils::FilePath &workingDirectory) const;
    void subversionDeltaCommit(const Utils::FilePath &workingDirectory) const;

    void stashPop(const Utils::FilePath &workingDirectory, const QString &stash = {});
    void revert(const QStringList &files, bool revertStaging);
    bool synchronousStashList(const Utils::FilePath &workingDirectory, QList<Stash> *stashes,
                              QString *errorMessage = nullptr) const;
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const Utils::FilePath &workingDirectory, const QString &messge, QString *name,
                              QString *errorMessage = nullptr) const;

    QString readGitVar(const Utils::FilePath &workingDirectory, const QString &configVar) const;
    QString readConfigValue(const Utils::FilePath &workingDirectory, const QString &configVar) const;
    void setConfigValue(const Utils::FilePath &workingDirectory, const QString &configVar,
                        const QString &value) const;

    QTextCodec *encoding(const Utils::FilePath &workingDirectory, const QString &configVar) const;
    bool readDataFromCommit(const Utils::FilePath &repoDirectory, const QString &commit,
                            CommitData &commitData, QString *errorMessage = nullptr,
                            QString *commitTemplate = nullptr);
    bool getCommitData(const Utils::FilePath &workingDirectory, QString *commitTemplate,
                       CommitData &commitData, QString *errorMessage);

    bool addAndCommit(const Utils::FilePath &workingDirectory,
                      const GitSubmitEditorPanelData &data,
                      CommitType commitType,
                      const QString &amendSHA1,
                      const QString &messageFile,
                      VcsBase::SubmitFileModel *model);

    enum StatusResult { StatusChanged, StatusUnchanged, StatusFailed };
    StatusResult gitStatus(const Utils::FilePath &workingDirectory, StatusMode mode,
                           QString *output = nullptr, QString *errorMessage = nullptr) const;

    CommandInProgress checkCommandInProgress(const Utils::FilePath &workingDirectory) const;
    QString commandInProgressDescription(const Utils::FilePath &workingDirectory) const;

    void continueCommandIfNeeded(const Utils::FilePath &workingDirectory, bool allowContinue = true);

    QString extendedShowDescription(const Utils::FilePath &workingDirectory, const QString &text) const;

    void launchGitK(const Utils::FilePath &workingDirectory, const QString &fileName) const;
    void launchGitK(const Utils::FilePath &workingDirectory) const { launchGitK(workingDirectory, QString()); }
    bool launchGitGui(const Utils::FilePath &workingDirectory);
    Utils::FilePath gitBinDirectory() const;
    bool launchGitBash(const Utils::FilePath &workingDirectory);

    void launchRepositoryBrowser(const Utils::FilePath &workingDirectory) const;

    QStringList synchronousRepositoryBranches(const QString &repositoryURL,
                                              const Utils::FilePath &workingDirectory = {}) const;

    Utils::Environment processEnvironment() const override;

    bool beginStashScope(const Utils::FilePath &workingDirectory, const QString &command,
                         StashFlag flag = Default, PushAction pushAction = NoPush);
    StashInfo &stashInfo(const Utils::FilePath &workingDirectory);
    void endStashScope(const Utils::FilePath &workingDirectory);
    bool isValidRevision(const QString &revision) const;
    void handleMergeConflicts(const Utils::FilePath &workingDir, const QString &commit,
                              const QStringList &files, const QString &abortCommand);
    void addFuture(const QFuture<void> &future);

    static QString msgNoChangedFiles();
    static QString msgNoCommits(bool includeRemote);
    void show(const QString &source, const QString &id, const QString &name = {});
    void archive(const Utils::FilePath &workingDirectory, QString commit);

    VcsBase::VcsCommand *asyncUpstreamStatus(const Utils::FilePath &workingDirectory,
                                             const QString &branch, const QString &upstream);

    enum class BranchTargetType { Remote, Commit };
    static QString suggestedLocalBranchName(
            const Utils::FilePath &workingDirectory, const QStringList &existingLocalNames,
            const QString &target, BranchTargetType targetType);
    static void addChangeActions(QMenu *menu, const QString &source, const QString &change);
    static Utils::FilePath fileWorkingDirectory(const QString &file);
    enum class ShowEditor { OnlyIfDifferent, Always };
    Core::IEditor *openShowEditor(const Utils::FilePath &workingDirectory, const QString &ref,
                                  const QString &path, ShowEditor showSetting = ShowEditor::Always);

private:
    void finishSubmoduleUpdate();
    void chunkActionsRequested(QMenu *menu, int fileIndex, int chunkIndex,
                               const DiffEditor::ChunkSelection &selection);

    void stage(DiffEditor::DiffEditorController *diffController,
               const QString &patch, bool revert);

    enum CodecType { CodecSource, CodecLogOutput, CodecNone };
    QTextCodec *codecFor(CodecType codecType, const Utils::FilePath &source = {}) const;

    void requestReload(const QString &documentId, const QString &source, const QString &title,
                       const Utils::FilePath &workingDirectory,
                       std::function<GitBaseDiffEditorController *(Core::IDocument *)> factory) const;

    // determine version as '(major << 16) + (minor << 8) + patch' or 0.
    unsigned synchronousGitVersion(QString *errorMessage = nullptr) const;

    QString readOneLine(const Utils::FilePath &workingDirectory, const QStringList &arguments) const;

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files,
                         bool *isDirectory,
                         QString *errorMessage,
                         bool revertStaging);
    void connectRepositoryChanged(const QString & repository, VcsBase::VcsCommand *cmd);
    bool executeAndHandleConflicts(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                                   const QString &abortCommand = {}) const;
    bool tryLauchingGitK(const Utils::Environment &env,
                         const Utils::FilePath &workingDirectory,
                         const QString &fileName,
                         const QString &gitBinDirectory) const;
    bool cleanList(const Utils::FilePath &workingDirectory, const QString &modulePath,
                   const QString &flag, QStringList *files, QString *errorMessage);

    enum ContinueCommandMode {
        ContinueOnly,
        SkipOnly,
        SkipIfNoChanges
    };

    void continuePreviousGitCommand(const Utils::FilePath &workingDirectory, const QString &msgBoxTitle,
                                    QString msgBoxText, const QString &buttonName,
                                    const QString &gitCommand, ContinueCommandMode continueMode);

    mutable Utils::FilePath m_gitVersionForBinary;
    mutable unsigned m_cachedGitVersion = 0;

    QString m_gitQtcEditor;
    QMap<Utils::FilePath, StashInfo> m_stashInfo;
    QString m_pushFallbackCommand;
    QString m_diffCommit;
    Utils::FilePaths m_updatedSubmodules;
    bool m_disableEditor = false;
    Utils::FutureSynchronizer m_synchronizer; // for commit updates
};

class GitRemote : public Core::IVersionControl::RepoUrl
{
public:
    GitRemote(const QString &location);
};

} // namespace Internal
} // namespace Git
