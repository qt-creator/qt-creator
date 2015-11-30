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

#ifndef GITCLIENT_H
#define GITCLIENT_H

#include "gitsettings.h"
#include "commitdata.h"

#include <coreplugin/editormanager/ieditor.h>
#include <vcsbase/vcsbaseclient.h>

#include <utils/fileutils.h>

#include <QFutureSynchronizer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSignalMapper;
class QDebug;
class QProcessEnvironment;
class QMenu;
QT_END_NAMESPACE

namespace Core { class ICore; }

namespace VcsBase {
    class VcsCommand;
    class SubmitFileModel;
    class VcsBaseEditorWidget;
    class VcsBaseEditorParameterWidget;
}

namespace Utils {
class ExitCodeInterpreter;
struct SynchronousProcessResponse;
} // namespace Utils

namespace DiffEditor {
class DiffEditorDocument;
class DiffEditorController;
}

namespace Git {
namespace Internal {

class CommitData;
struct GitSubmitEditorPanelData;
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

typedef QMap<QString, SubmoduleData> SubmoduleDataMap;

class GitClient : public VcsBase::VcsBaseClientImpl
{
    Q_OBJECT

public:
    enum CommandInProgress { NoCommand, Revert, CherryPick,
                             Rebase, Merge, RebaseMerge };

    class StashInfo
    {
    public:
        StashInfo();
        enum StashResult { StashUnchanged, StashCanceled, StashFailed,
                           Stashed, NotStashed /* User did not want it */ };

        bool init(const QString &workingDirectory, const QString &command,
                  StashFlag flag = Default, PushAction pushAction = NoPush);
        bool stashingFailed() const;
        void end();
        StashResult result() const { return m_stashResult; }
        QString stashMessage() const { return m_message; }

    private:
        void stashPrompt(const QString &command, const QString &statusOutput, QString *errorMessage);
        void executeStash(const QString &command, QString *errorMessage);

        StashResult m_stashResult;
        QString m_message;
        QString m_workingDir;
        GitClient *m_client;
        StashFlag m_flags;
        PushAction m_pushAction;
    };

    static const char *stashNamePrefix;

    explicit GitClient();

    Utils::FileName vcsBinary() const override;
    unsigned gitVersion(QString *errorMessage = 0) const;

    VcsBase::VcsCommand *vcsExecAbortable(const QString &workingDirectory, const QStringList &arguments);

    QString findRepositoryForDirectory(const QString &dir) const;
    QString findGitDirForRepository(const QString &repositoryDir) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;

    void diffFile(const QString &workingDirectory, const QString &fileName) const;
    void diffFiles(const QString &workingDirectory,
                   const QStringList &unstagedFileNames,
                   const QStringList &stagedFileNames) const;
    void diffProject(const QString &workingDirectory,
                     const QString &projectDirectory) const;
    void diffRepository(const QString &workingDirectory);
    void diffBranch(const QString &workingDirectory,
                    const QString &branchName) const;
    void merge(const QString &workingDirectory, const QStringList &unmergedFileNames = QStringList());

    void status(const QString &workingDirectory);
    void log(const QString &workingDirectory, const QString &fileName = QString(),
             bool enableAnnotationContextMenu = false, const QStringList &args = QStringList());
    void reflog(const QString &workingDirectory);
    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber = -1,
                  const QStringList &extraOptions = QStringList()) override;
    void reset(const QString &workingDirectory, const QString &argument, const QString &commit = QString());
    void addFile(const QString &workingDirectory, const QString &fileName);
    bool synchronousLog(const QString &workingDirectory,
                        const QStringList &arguments,
                        QString *output,
                        QString *errorMessage = 0,
                        unsigned flags = 0);
    bool synchronousAdd(const QString &workingDirectory, const QStringList &files);
    bool synchronousDelete(const QString &workingDirectory,
                           bool force,
                           const QStringList &files);
    bool synchronousMove(const QString &workingDirectory,
                         const QString &from,
                         const QString &to);
    bool synchronousReset(const QString &workingDirectory,
                          const QStringList &files = QStringList(),
                          QString *errorMessage = 0);
    bool synchronousCleanList(const QString &workingDirectory, QStringList *files, QStringList *ignoredFiles, QString *errorMessage);
    bool synchronousApplyPatch(const QString &workingDirectory, const QString &file, QString *errorMessage, const QStringList &arguments = QStringList());
    bool synchronousInit(const QString &workingDirectory);
    bool synchronousCheckoutFiles(const QString &workingDirectory,
                                  QStringList files = QStringList(),
                                  QString revision = QString(), QString *errorMessage = 0,
                                  bool revertStaging = true);
    // Checkout ref
    bool stashAndCheckout(const QString &workingDirectory, const QString &ref);
    bool synchronousCheckout(const QString &workingDirectory, const QString &ref,
                             QString *errorMessage = 0);

    QStringList setupCheckoutArguments(const QString &workingDirectory, const QString &ref);
    void updateSubmodulesIfNeeded(const QString &workingDirectory, bool prompt);

    // Do a stash and return identier.
    enum { StashPromptDescription = 0x1, StashImmediateRestore = 0x2, StashIgnoreUnchanged = 0x4 };
    QString synchronousStash(const QString &workingDirectory,
                             const QString &messageKeyword = QString(),
                             unsigned flags = 0, bool *unchanged = 0) const;

    bool executeSynchronousStash(const QString &workingDirectory,
                                 const QString &message = QString(),
                                 bool unstagedOnly = false,
                                 QString *errorMessage = 0) const;
    bool synchronousStashRestore(const QString &workingDirectory,
                                 const QString &stash,
                                 bool pop = false,
                                 const QString &branch = QString()) const;
    bool synchronousStashRemove(const QString &workingDirectory,
                                const QString &stash = QString(),
                                QString *errorMessage = 0) const;
    bool synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                              QString *output, QString *errorMessage) const;
    bool synchronousTagCmd(const QString &workingDirectory, QStringList tagArgs,
                           QString *output, QString *errorMessage) const;
    bool synchronousForEachRefCmd(const QString &workingDirectory, QStringList args,
                               QString *output, QString *errorMessage = 0) const;
    bool synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                              QString *output = 0, QString *errorMessage = 0, bool silent = false) const;

    QMap<QString,QString> synchronousRemotesList(const QString &workingDirectory,
                                                 QString *errorMessage = 0) const;
    QStringList synchronousSubmoduleStatus(const QString &workingDirectory,
                                           QString *errorMessage = 0) const;
    SubmoduleDataMap submoduleList(const QString &workingDirectory) const;
    bool synchronousShow(const QString &workingDirectory, const QString &id,
                              QString *output, QString *errorMessage) const;

    bool synchronousRevListCmd(const QString &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessage = 0) const;

    bool synchronousParentRevisions(const QString &workingDirectory,
                                    const QString &revision,
                                    QStringList *parents,
                                    QString *errorMessage) const;
    QString synchronousShortDescription(const QString &workingDirectory, const QString &revision) const;
    QString synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     const QString &format) const;

    QString synchronousCurrentLocalBranch(const QString &workingDirectory) const;

    bool synchronousHeadRefs(const QString &workingDirectory, QStringList *output,
                             QString *errorMessage = 0) const;
    QString synchronousTopic(const QString &workingDirectory) const;
    bool synchronousRevParseCmd(const QString &workingDirectory, const QString &ref,
                                QString *output, QString *errorMessage = 0) const;
    QString synchronousTopRevision(const QString &workingDirectory, QString *errorMessage = 0);
    void synchronousTagsForCommit(const QString &workingDirectory, const QString &revision,
                                  QString &precedes, QString &follows) const;
    bool isRemoteCommit(const QString &workingDirectory, const QString &commit);
    bool isFastForwardMerge(const QString &workingDirectory, const QString &branch);

    bool cloneRepository(const QString &directory, const QByteArray &url);
    void fetch(const QString &workingDirectory, const QString &remote);
    bool synchronousPull(const QString &workingDirectory, bool rebase);
    void push(const QString &workingDirectory, const QStringList &pushArgs = QStringList());
    bool synchronousMerge(const QString &workingDirectory, const QString &branch,
                          bool allowFastForward = true);
    bool canRebase(const QString &workingDirectory) const;
    void rebase(const QString &workingDirectory, const QString &argument);
    void cherryPick(const QString &workingDirectory, const QString &argument);
    void revert(const QString &workingDirectory, const QString &argument);

    bool synchronousRevert(const QString &workingDirectory, const QString &commit);
    bool synchronousCherryPick(const QString &workingDirectory, const QString &commit);
    void interactiveRebase(const QString &workingDirectory, const QString &commit, bool fixup);
    void synchronousAbortCommand(const QString &workingDir, const QString &abortCommand);
    QString synchronousTrackingBranch(const QString &workingDirectory,
                                      const QString &branch = QString());
    bool synchronousSetTrackingBranch(const QString &workingDirectory,
                                      const QString &branch,
                                      const QString &tracking);

    // git svn support (asynchronous).
    void synchronousSubversionFetch(const QString &workingDirectory);
    void subversionLog(const QString &workingDirectory);

    void stashPop(const QString &workingDirectory, const QString &stash = QString());
    void revert(const QStringList &files, bool revertStaging);
    bool synchronousStashList(const QString &workingDirectory,
                              QList<Stash> *stashes,
                              QString *errorMessage = 0) const;
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const QString &workingDirectory,
                              const QString &messge, QString *name,
                              QString *errorMessage = 0) const;

    QString readGitVar(const QString &workingDirectory, const QString &configVar) const;
    QString readConfigValue(const QString &workingDirectory, const QString &configVar) const;

    QTextCodec *encoding(const QString &workingDirectory, const QByteArray &configVar) const;
    bool readDataFromCommit(const QString &repoDirectory, const QString &commit,
                            CommitData &commitData, QString *errorMessage = 0,
                            QString *commitTemplate = 0);
    bool getCommitData(const QString &workingDirectory, QString *commitTemplate,
                       CommitData &commitData, QString *errorMessage);

    bool addAndCommit(const QString &workingDirectory,
                      const GitSubmitEditorPanelData &data,
                      CommitType commitType,
                      const QString &amendSHA1,
                      const QString &messageFile,
                      VcsBase::SubmitFileModel *model);

    enum StatusResult { StatusChanged, StatusUnchanged, StatusFailed };
    StatusResult gitStatus(const QString &workingDirectory,
                           StatusMode mode,
                           QString *output = 0,
                           QString *errorMessage = 0) const;

    CommandInProgress checkCommandInProgress(const QString &workingDirectory) const;
    QString commandInProgressDescription(const QString &workingDirectory) const;

    void continueCommandIfNeeded(const QString &workingDirectory, bool allowContinue = true);

    QString extendedShowDescription(const QString &workingDirectory, const QString &text) const;

    void launchGitK(const QString &workingDirectory, const QString &fileName);
    void launchGitK(const QString &workingDirectory) { launchGitK(workingDirectory, QString()); }
    bool launchGitGui(const QString &workingDirectory);
    Utils::FileName gitBinDirectory();

    void launchRepositoryBrowser(const QString &workingDirectory);

    QStringList synchronousRepositoryBranches(const QString &repositoryURL,
                                              const QString &workingDirectory = QString()) const;

    QProcessEnvironment processEnvironment() const override;

    bool beginStashScope(const QString &workingDirectory, const QString &command,
                         StashFlag flag = Default, PushAction pushAction = NoPush);
    StashInfo &stashInfo(const QString &workingDirectory);
    void endStashScope(const QString &workingDirectory);
    bool isValidRevision(const QString &revision) const;
    void handleMergeConflicts(const QString &workingDir, const QString &commit, const QStringList &files, const QString &abortCommand);
    void addFuture(const QFuture<void> &future);

    static QString msgNoChangedFiles();
    static QString msgNoCommits(bool includeRemote);

public slots:
    void show(const QString &source, const QString &id, const QString &name = QString());

private slots:
    void finishSubmoduleUpdate();
    void fetchFinished(const QVariant &cookie);
    void slotChunkActionsRequested(QMenu *menu, bool isValid);
    void slotStageChunk();
    void slotUnstageChunk();
    void branchesForCommit(const QString &revision);

private:
    void stage(const QString &patch, bool revert);

    enum CodecType { CodecSource, CodecLogOutput, CodecNone };
    QTextCodec *codecFor(CodecType codecType, const QString &source = QString()) const;

    void requestReload(const QString &documentId, const QString &source, const QString &title,
                       std::function<DiffEditor::DiffEditorController *(Core::IDocument *)> factory) const;

    // determine version as '(major << 16) + (minor << 8) + patch' or 0.
    unsigned synchronousGitVersion(QString *errorMessage = 0) const;

    QString readOneLine(const QString &workingDirectory, const QStringList &arguments) const;

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files,
                         bool *isDirectory,
                         QString *errorMessage,
                         bool revertStaging);
    void connectRepositoryChanged(const QString & repository, VcsBase::VcsCommand *cmd);
    bool executeAndHandleConflicts(const QString &workingDirectory, const QStringList &arguments,
                                   const QString &abortCommand = QString()) const;
    bool tryLauchingGitK(const QProcessEnvironment &env,
                         const QString &workingDirectory,
                         const QString &fileName,
                         const QString &gitBinDirectory);
    bool cleanList(const QString &workingDirectory, const QString &flag, QStringList *files, QString *errorMessage);

    enum ContinueCommandMode {
        ContinueOnly,
        SkipOnly,
        SkipIfNoChanges
    };

    void continuePreviousGitCommand(const QString &workingDirectory, const QString &msgBoxTitle,
                                    QString msgBoxText, const QString &buttonName,
                                    const QString &gitCommand, ContinueCommandMode continueMode);

    mutable Utils::FileName m_gitVersionForBinary;
    mutable unsigned m_cachedGitVersion;

    QString m_gitQtcEditor;
    QMap<QString, StashInfo> m_stashInfo;
    QStringList m_updatedSubmodules;
    bool m_disableEditor;
    QPointer<DiffEditor::DiffEditorController> m_contextController;
    QFutureSynchronizer<void> m_synchronizer; // for commit updates
};

} // namespace Internal
} // namespace Git

#endif // GITCLIENT_H
