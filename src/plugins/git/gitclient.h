/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GITCLIENT_H
#define GITCLIENT_H

#include "gitsettings.h"

#include <coreplugin/editormanager/ieditor.h>
#include <vcsbase/command.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSignalMapper;
class QDebug;
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}

namespace VcsBase {
    class VcsBaseEditorWidget;
    class SubmitFileModel;
}

namespace Utils {
    struct SynchronousProcessResponse;
}

namespace DiffEditor {
    class DiffEditor;
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

class GitClient : public QObject
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

        bool init(const QString &workingDirectory, const QString &keyword, StashFlag flag = Default);
        bool stashingFailed() const;
        void end();
        StashResult result() const { return m_stashResult; }
        QString stashMessage() const { return m_message; }

    private:
        void stashPrompt(const QString &keyword, const QString &statusOutput, QString *errorMessage);
        void executeStash(const QString &keyword, QString *errorMessage);

        StashResult m_stashResult;
        QString m_message;
        QString m_workingDir;
        GitClient *m_client;
        StashFlag m_flags;
    };

    static const char *stashNamePrefix;

    explicit GitClient(GitSettings *settings);
    ~GitClient();

    QString gitBinaryPath(bool *ok = 0, QString *errorMessage = 0) const;
    unsigned gitVersion(QString *errorMessage = 0) const;

    QString findRepositoryForDirectory(const QString &dir);
    QString findGitDirForRepository(const QString &repositoryDir) const;

    void diff(const QString &workingDirectory, const QStringList &diffArgs, const QString &fileName);
    void diff(const QString &workingDirectory, const QStringList &diffArgs,
              const QStringList &unstagedFileNames, const QStringList &stagedFileNames= QStringList());
    void diffBranch(const QString &workingDirectory,
                    const QStringList &diffArgs,
                    const QString &branchName);
    void merge(const QString &workingDirectory, const QStringList &unmergedFileNames = QStringList());

    void status(const QString &workingDirectory);
    void log(const QString &workingDirectory, const QStringList &fileNames = QStringList(),
             bool enableAnnotationContextMenu = false, const QStringList &args = QStringList());
    void blame(const QString &workingDirectory, const QStringList &args, const QString &fileName,
               const QString &revision = QString(), int lineNumber = -1);
    void reset(const QString &workingDirectory, const QString &argument, const QString &commit = QString());
    void addFile(const QString &workingDirectory, const QString &fileName);
    bool synchronousLog(const QString &workingDirectory,
                        const QStringList &arguments,
                        QString *output, QString *errorMessage = 0);
    bool synchronousAdd(const QString &workingDirectory,
                        // Warning: Works only from 1.6.1 onwards
                        bool intendToAdd,
                        const QStringList &files);
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
    bool synchronousApplyPatch(const QString &workingDirectory, const QString &file, QString *errorMessage);
    bool synchronousInit(const QString &workingDirectory);
    bool synchronousCheckoutFiles(const QString &workingDirectory,
                                  QStringList files = QStringList(),
                                  QString revision = QString(), QString *errorMessage = 0,
                                  bool revertStaging = true);
    // Checkout branch
    bool synchronousCheckout(const QString &workingDirectory, const QString &ref, QString *errorMessage);
    bool synchronousCheckout(const QString &workingDirectory, const QString &ref)
         { return synchronousCheckout(workingDirectory, ref, 0); }
    void updateSubmodulesIfNeeded(const QString &workingDirectory, bool prompt);

    // Do a stash and return identier.
    enum { StashPromptDescription = 0x1, StashImmediateRestore = 0x2, StashIgnoreUnchanged = 0x4 };
    QString synchronousStash(const QString &workingDirectory,
                             const QString &messageKeyword = QString(),
                             unsigned flags = 0, bool *unchanged = 0);

    bool executeSynchronousStash(const QString &workingDirectory,
                                 const QString &message = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRestore(const QString &workingDirectory,
                                 const QString &stash,
                                 bool pop = false,
                                 const QString &branch = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRemove(const QString &workingDirectory,
                                const QString &stash = QString(),
                                QString *errorMessage = 0);
    bool synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                              QString *output, QString *errorMessage);
    bool synchronousForEachRefCmd(const QString &workingDirectory, QStringList args,
                               QString *output, QString *errorMessage);
    bool synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                              QString *output, QString *errorMessage);

    QMap<QString,QString> synchronousRemotesList(const QString &workingDirectory,
                                                 QString *errorMessage = 0);
    QStringList synchronousSubmoduleStatus(const QString &workingDirectory,
                                           QString *errorMessage = 0);
    SubmoduleDataMap submoduleList(const QString &workingDirectory);
    bool synchronousShow(const QString &workingDirectory, const QString &id,
                              QString *output, QString *errorMessage);

    bool synchronousRevListCmd(const QString &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessage = 0);

    bool synchronousParentRevisions(const QString &workingDirectory,
                                    const QStringList &files /* = QStringList() */,
                                    const QString &revision,
                                    QStringList *parents,
                                    QString *errorMessage);
    QString synchronousShortDescription(const QString &workingDirectory, const QString &revision);
    QString synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     const QString &format);

    QString synchronousCurrentLocalBranch(const QString &workingDirectory);

    bool synchronousHeadRefs(const QString &workingDirectory, QStringList *output,
                             QString *errorMessage = 0);
    QString synchronousTopic(const QString &workingDirectory);
    QString synchronousTopRevision(const QString &workingDirectory, QString *errorMessage = 0);
    void synchronousTagsForCommit(const QString &workingDirectory, const QString &revision,
                                  QByteArray &precedes, QByteArray &follows);

    bool cloneRepository(const QString &directory, const QByteArray &url);
    QString vcsGetRepositoryURL(const QString &directory);
    void fetch(const QString &workingDirectory, const QString &remote);
    bool synchronousPull(const QString &workingDirectory, bool rebase);
    void push(const QString &workingDirectory, const QStringList &pushArgs = QStringList());
    bool synchronousMerge(const QString &workingDirectory, const QString &branch);
    bool canRebase(const QString &workingDirectory) const;
    void rebase(const QString &workingDirectory, const QString &baseBranch);
    bool synchronousRevert(const QString &workingDirectory, const QString &commit);
    bool synchronousCherryPick(const QString &workingDirectory, const QString &commit);
    void interactiveRebase(const QString &workingDirectory, const QString &commit, bool fixup);
    void synchronousAbortCommand(const QString &workingDir, const QString &abortCommand);
    QString synchronousTrackingBranch(const QString &workingDirectory,
                                      const QString &branch = QString());

    // git svn support (asynchronous).
    void synchronousSubversionFetch(const QString &workingDirectory);
    void subversionLog(const QString &workingDirectory);

    void stashPop(const QString &workingDirectory, const QString &stash);
    void stashPop(const QString &workingDirectory);
    void revert(const QStringList &files, bool revertStaging);
    void branchList(const QString &workingDirectory);
    void stashList(const QString &workingDirectory);
    bool synchronousStashList(const QString &workingDirectory,
                              QList<Stash> *stashes,
                              QString *errorMessage = 0);
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const QString &workingDirectory,
                              const QString &messge, QString *name,
                              QString *errorMessage = 0);

    QString readConfig(const QString &workingDirectory, const QStringList &configVar) const;

    QString readConfigValue(const QString &workingDirectory, const QString &configVar) const;

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
                           QString *errorMessage = 0);

    CommandInProgress checkCommandInProgress(const QString &workingDirectory);
    CommandInProgress checkCommandInProgressInGitDir(const QString &gitDir);

    void continueCommandIfNeeded(const QString &workingDirectory);
    void continuePreviousGitCommand(const QString &workingDirectory, const QString &msgBoxTitle, QString msgBoxText,
                                    const QString &buttonName, const QString &gitCommand, bool requireChanges = true);

    void launchGitK(const QString &workingDirectory, const QString &fileName);
    void launchGitK(const QString &workingDirectory) { launchGitK(workingDirectory, QString()); }

    void launchRepositoryBrowser(const QString &workingDirectory);

    QStringList synchronousRepositoryBranches(const QString &repositoryURL);

    GitSettings *settings() const;

    QProcessEnvironment processEnvironment() const;

    bool beginStashScope(const QString &workingDirectory, const QString &keyword, StashFlag flag = Default);
    StashInfo &stashInfo(const QString &workingDirectory);
    void endStashScope(const QString &workingDirectory);
    bool isValidRevision(const QString &revision) const;
    void handleMergeConflicts(const QString &workingDir, const QString &commit, const QStringList &files, const QString &abortCommand);

    static QString msgNoChangedFiles();
    static QString msgNoCommits(bool includeRemote);

public slots:
    void show(const QString &source, const QString &id,
              const QStringList &args = QStringList(), const QString &name = QString());
    void saveSettings();

private slots:
    void slotBlameRevisionRequested(const QString &source, QString change, int lineNumber);
    void appendOutputData(const QByteArray &data) const;
    void appendOutputDataSilently(const QByteArray &data) const;
    void finishSubmoduleUpdate();
    void fetchFinished(const QVariant &cookie);

private:
    QTextCodec *getSourceCodec(const QString &file) const;
    VcsBase::VcsBaseEditorWidget *findExistingVCSEditor(const char *registerDynamicProperty,
                                                  const QString &dynamicPropertyValue) const;
    DiffEditor::DiffEditor *findExistingOrOpenNewDiffEditor(const char *registerDynamicProperty,
                                               const QString &dynamicPropertyValue,
                                               const QString &titlePattern,
                                               const Core::Id editorId) const;

    enum CodecType { CodecSource, CodecLogOutput, CodecNone };
    VcsBase::VcsBaseEditorWidget *createVcsEditor(const Core::Id &kind,
                                            QString title,
                                            const QString &source,
                                            CodecType codecType,
                                            const char *registerDynamicProperty,
                                            const QString &dynamicPropertyValue,
                                            QWidget *configWidget) const;

    VcsBase::Command *createCommand(const QString &workingDirectory,
                             VcsBase::VcsBaseEditorWidget* editor = 0,
                             bool useOutputToWindow = false,
                             int editorLineNumber = -1);

    VcsBase::Command *executeGit(const QString &workingDirectory,
                                 const QStringList &arguments,
                                 VcsBase::VcsBaseEditorWidget* editor = 0,
                                 bool useOutputToWindow = false,
                                 bool expectChanges = false,
                                 int editorLineNumber = -1);

    // Fully synchronous git execution (QProcess-based).
    bool fullySynchronousGit(const QString &workingDirectory,
                             const QStringList &arguments,
                             QByteArray* outputText,
                             QByteArray* errorText = 0,
                             unsigned flags = 0) const;

    // Synchronous git execution using Utils::SynchronousProcess, with
    // log windows updating (using VcsBasePlugin::runVcs with flags).
    inline Utils::SynchronousProcessResponse
            synchronousGit(const QString &workingDirectory, const QStringList &arguments,
                           unsigned flags = 0, QTextCodec *outputCodec = 0);

    // determine version as '(major << 16) + (minor << 8) + patch' or 0.
    unsigned synchronousGitVersion(QString *errorMessage = 0) const;

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files,
                         bool *isDirectory,
                         QString *errorMessage,
                         bool revertStaging);
    void connectRepositoryChanged(const QString & repository, VcsBase::Command *cmd);
    bool executeAndHandleConflicts(const QString &workingDirectory, const QStringList &arguments,
                                   const QString &abortCommand = QString());
    bool tryLauchingGitK(const QProcessEnvironment &env,
                         const QString &workingDirectory,
                         const QString &fileName,
                         const QString &gitBinDirectory,
                         bool silent);
    bool cleanList(const QString &workingDirectory, const QString &flag, QStringList *files, QString *errorMessage);

    mutable QString m_gitVersionForBinary;
    mutable unsigned m_cachedGitVersion;

    const QString m_msgWait;
    GitSettings *m_settings;
    QString m_gitQtcEditor;
    QMap<QString, StashInfo> m_stashInfo;
    QStringList m_updatedSubmodules;
    bool m_disableEditor;
};

} // namespace Internal
} // namespace Git

#endif // GITCLIENT_H
