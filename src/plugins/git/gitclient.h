// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitsettings.h"
#include "git_global.h"
#include "commitdata.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcsbaseclient.h>

#include <utils/fileutils.h>
#include <utils/process.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Core { class ICore; }

namespace DiffEditor {
class ChunkSelection;
class DiffEditorController;
}

namespace VcsBase {
class SubmitFileModel;
class VcsBaseEditorWidget;
class VcsCommand;
}

namespace Git::Internal {

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

struct Author {
    bool operator==(const Author &other) const {
        return name == other.name && email == other.email;
    }
    bool operator!=(const Author &other) const {
        return !operator==(other);
    }
    QString name;
    QString email;
};

class GITSHARED_EXPORT GitClient : public VcsBase::VcsBaseClientImpl
{
public:
    enum CommandInProgress { NoCommand, Revert, CherryPick,
                             Rebase, Merge, RebaseMerge };
    enum GitKLaunchTrial { Bin, ParentOfBin, SystemPath, None };

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

    GitClient();
    ~GitClient();

    Utils::FilePath vcsBinary() const override;
    QFuture<unsigned> gitVersion() const;

    void vcsExecAbortable(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                          bool isRebase = false, const QString &abortCommand = {},
                          const QObject *context = nullptr,
                          const VcsBase::CommandHandler &handler = {});

    Utils::FilePath findRepositoryForDirectory(const Utils::FilePath &directory) const;
    Utils::FilePath findGitDirForRepository(const Utils::FilePath &repositoryDir) const;
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
    void annotate(const Utils::FilePath &workingDir, const QString &file,
                  int lineNumber = -1, const QString &revision = {},
                  const QStringList &extraOptions = {}, int firstLine = -1) override;
    void reset(const Utils::FilePath &workingDirectory, const QString &argument, const QString &commit = {});
    void removeStaleRemoteBranches(const Utils::FilePath &workingDirectory, const QString &remote);
    void recoverDeletedFiles(const Utils::FilePath &workingDirectory);
    void addFile(const Utils::FilePath &workingDirectory, const QString &fileName);
    bool synchronousLog(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                        QString *output, QString *errorMessage = nullptr,
                        VcsBase::RunFlags flags = VcsBase::RunFlags::None);
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
                               QString *errorMessage, const QStringList &extraArguments = {}) const;
    bool synchronousInit(const Utils::FilePath &workingDirectory);
    bool synchronousCheckoutFiles(const Utils::FilePath &workingDirectory, QStringList files = {},
                                  QString revision = {}, QString *errorMessage = nullptr,
                                  bool revertStaging = true);
    enum class StashMode { NoStash, TryStash };
    void checkout(const Utils::FilePath &workingDirectory, const QString &ref,
                  StashMode stashMode = StashMode::TryStash, const QObject *context = nullptr,
                  const VcsBase::CommandHandler &handler = {});

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
    bool synchronousRemoteCmd(const Utils::FilePath &workingDirectory, QStringList remoteArgs,
                              QString *output = nullptr, QString *errorMessage = nullptr,
                              bool silent = false) const;

    QMap<QString,QString> synchronousRemotesList(const Utils::FilePath &workingDirectory,
                                                 QString *errorMessage = nullptr) const;
    QStringList synchronousSubmoduleStatus(const Utils::FilePath &workingDirectory,
                                           QString *errorMessage = nullptr) const;
    SubmoduleDataMap submoduleList(const Utils::FilePath &workingDirectory) const;
    QByteArray synchronousShow(const Utils::FilePath &workingDirectory, const QString &id,
                               VcsBase::RunFlags flags = VcsBase::RunFlags::None) const;

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
    Utils::ProcessTask topRevision(const Utils::FilePath &workingDirectory,
        const std::function<void(const QString &, const QDateTime &)> &callback);
    bool isRemoteCommit(const Utils::FilePath &workingDirectory, const QString &commit);

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
    void revertFiles(const QStringList &files, bool revertStaging);
    bool synchronousStashList(const Utils::FilePath &workingDirectory, QList<Stash> *stashes,
                              QString *errorMessage = nullptr) const;
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const Utils::FilePath &workingDirectory, const QString &messge, QString *name,
                              QString *errorMessage = nullptr) const;

    QString readGitVar(const Utils::FilePath &workingDirectory, const QString &configVar) const;
    QString readConfigValue(const Utils::FilePath &workingDirectory, const QString &configVar) const;
    QChar commentChar(const Utils::FilePath &workingDirectory);
    void setConfigValue(const Utils::FilePath &workingDirectory, const QString &configVar,
                        const QString &value) const;

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

    static QString msgNoChangedFiles();
    static QString msgNoCommits(bool includeRemote);
    void show(const Utils::FilePath &source, const QString &id, const QString &name = {});
    void archive(const Utils::FilePath &workingDirectory, QString commit);

    enum class BranchTargetType { Remote, Commit };
    static QString suggestedLocalBranchName(
            const Utils::FilePath &workingDirectory, const QStringList &existingLocalNames,
            const QString &target, BranchTargetType targetType);
    static void addChangeActions(QMenu *menu, const Utils::FilePath &source, const QString &change);
    static Utils::FilePath fileWorkingDirectory(const Utils::FilePath &file);
    enum class ShowEditor { OnlyIfDifferent, Always };
    Core::IEditor *openShowEditor(const Utils::FilePath &workingDirectory, const QString &ref,
                                  const Utils::FilePath &path, ShowEditor showSetting = ShowEditor::Always);

    Author parseAuthor(const QString &authorInfo);
    Author getAuthor(const Utils::FilePath &workingDirectory);

    QTextCodec *defaultCommitEncoding() const;
    enum EncodingType { EncodingSource, EncodingLogOutput, EncodingCommit, EncodingDefault };
    QTextCodec *encoding(EncodingType encodingType, const Utils::FilePath &source = {}) const;

    void readConfigAsync(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                         const VcsBase::CommandHandler &handler) const;

private:
    static GitSettings &settings();

    void finishSubmoduleUpdate();

    void requestReload(const QString &documentId, const Utils::FilePath &source,
                       const QString &title, const Utils::FilePath &workingDirectory,
                       std::function<GitBaseDiffEditorController *(Core::IDocument *)> factory) const;

    QString readOneLine(const Utils::FilePath &workingDirectory, const QStringList &arguments) const;

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files,
                         bool *isDirectory,
                         QString *errorMessage,
                         bool revertStaging);
    bool executeAndHandleConflicts(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                                   const QString &abortCommand = {}) const;
    void tryLaunchingGitK(const Utils::Environment &env,
                          const Utils::FilePath &workingDirectory,
                          const QString &fileName,
                          GitKLaunchTrial trial = GitKLaunchTrial::Bin) const;
    void handleGitKFailedToStart(const Utils::Environment &env,
                                 const Utils::FilePath &workingDirectory,
                                 const QString &fileName,
                                 const GitKLaunchTrial oldTrial,
                                 const Utils::FilePath &oldGitBinDir) const;
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
    QString m_diffCommit;
    Utils::FilePaths m_updatedSubmodules;
    bool m_disableEditor = false;
};

GITSHARED_EXPORT GitClient &gitClient();

class GitRemote : public Core::IVersionControl::RepoUrl
{
public:
    GitRemote(const QString &location);
};

} // Git::Internal
