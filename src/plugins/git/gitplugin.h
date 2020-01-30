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

#include <coreplugin/id.h>

#include <vcsbase/vcsbaseplugin.h>

#include <QStringList>
#include <QPointer>
#include <QKeySequence>
#include <QVector>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class Command;
class CommandLocator;
class Context;
class ActionContainer;
} // namespace Core
namespace Utils { class ParameterAction; }
namespace Gerrit {
namespace Internal { class GerritPlugin; }
} // namespace Gerrit

namespace Git {
namespace Internal {

class GitClient;
class CommitData;
class StashDialog;
class BranchDialog;
class BranchViewFactory;
class RemoteDialog;

using GitClientMemberFunc = void (GitClient::*)(const QString &);

class GitPluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_OBJECT

public:
    GitPluginPrivate();
    ~GitPluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Core::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;
    QStringList unmanagedFiles(const QString &workingDir, const QStringList &filePaths) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const QString &fileName) final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    bool vcsAnnotate(const QString &file, int line) final;
    QString vcsTopic(const QString &directory) final;

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FilePath &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) final;

    RepoUrl getRepoUrl(const QString &location) const override;

    QStringList additionalToolsPath() const final;

    void emitFilesChanged(const QStringList &);
    void emitRepositoryChanged(const QString &);

    ///
    static GitPluginPrivate *instance();
    static GitClient *client();

    Gerrit::Internal::GerritPlugin *gerritPlugin() const;
    bool isCommitEditorOpen() const;
    static QString msgRepositoryLabel(const QString &repository);
    static QString invalidBranchAndRemoteNamePattern();
    void startCommit(CommitType commitType = SimpleCommit);
    void updateBranches(const QString &repository);
    void updateCurrentBranch();

    void manageRemotes();
    void initRepository();
    void startRebaseFromCommit(const QString &workingDirectory, QString commit);

protected:
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool submitEditorAboutToClose() override;

private:
    void diffCurrentFile();
    void diffCurrentProject();
    void commitFromEditor() override;
    void logFile();
    void blameFile();
    void logProject();
    void logRepository();
    void undoFileChanges(bool revertStaging);
    void resetRepository();
    void recoverDeletedFiles();
    void startRebase();
    void startChangeRelatedAction(const Core::Id &id);
    void stageFile();
    void unstageFile();
    void gitkForCurrentFile();
    void gitkForCurrentFolder();
    void gitGui();
    void cleanProject();
    void cleanRepository();
    void updateSubmodules();
    void applyCurrentFilePatch();
    void promptApplyPatch();

    void stash(bool unstagedOnly = false);
    void stashUnstaged();
    void stashSnapshot();
    void stashPop();
    void branchList();
    void stashList();
    void fetch();
    void pull();
    void push();
    void startMergeTool();
    void continueOrAbortCommand();
    void updateContinueAndAbortCommands();
    void delayedPushToGerrit();

    Core::Command *createCommand(QAction *action, Core::ActionContainer *ac, Core::Id id,
                                 const Core::Context &context, bool addToLocator,
                                 const std::function<void()> &callback, const QKeySequence &keys);

    Utils::ParameterAction *createParameterAction(Core::ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Core::Id id, const Core::Context &context, bool addToLocator,
                                                  const std::function<void()> &callback,
                                                  const QKeySequence &keys = QKeySequence());

    QAction *createFileAction(Core::ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Core::Id id, const Core::Context &context, bool addToLocator,
                              const std::function<void()> &callback,
                              const QKeySequence &keys = QKeySequence());

    QAction *createProjectAction(Core::ActionContainer *ac,
                                 const QString &defaultText, const QString &parameterText,
                                 Core::Id id, const Core::Context &context, bool addToLocator,
                                 void (GitPluginPrivate::*func)(),
                                 const QKeySequence &keys = QKeySequence());

    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Core::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    const std::function<void()> &callback,
                                    const QKeySequence &keys = QKeySequence());
    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Core::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    GitClientMemberFunc, const QKeySequence &keys = QKeySequence());

    QAction *createChangeRelatedRepositoryAction(const QString &text, Core::Id id,
                                                 const Core::Context &context);

    void updateRepositoryBrowserAction();
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);
    void applyPatch(const QString &workingDirectory, QString file = QString());
    void updateVersionWarning();

    Core::CommandLocator *m_commandLocator = nullptr;

    QAction *m_menuAction = nullptr;
    QAction *m_repositoryBrowserAction = nullptr;
    QAction *m_mergeToolAction = nullptr;
    QAction *m_submoduleUpdateAction = nullptr;
    QAction *m_abortMergeAction = nullptr;
    QAction *m_abortRebaseAction = nullptr;
    QAction *m_abortCherryPickAction = nullptr;
    QAction *m_abortRevertAction = nullptr;
    QAction *m_skipRebaseAction = nullptr;
    QAction *m_continueRebaseAction = nullptr;
    QAction *m_continueCherryPickAction = nullptr;
    QAction *m_continueRevertAction = nullptr;
    QAction *m_fixupCommitAction = nullptr;
    QAction *m_interactiveRebaseAction = nullptr;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;
    Utils::ParameterAction *m_applyCurrentFilePatchAction = nullptr;
    Gerrit::Internal::GerritPlugin *m_gerritPlugin = nullptr;

    GitSettings m_settings;
    GitClient *m_gitClient = nullptr;
    QPointer<StashDialog> m_stashDialog;
    QPointer<BranchViewFactory> m_branchViewFactory;
    QPointer<RemoteDialog> m_remoteDialog;
    QString m_submitRepository;
    QString m_commitMessageFileName;
    bool m_submitActionTriggered = false;
};

class GitPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Git.json")

public:
    ~GitPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;

    QObject *remoteCommand(const QStringList &options, const QString &workingDirectory,
                           const QStringList &args) final;

#ifdef WITH_TESTS
private slots:
    void testStatusParsing_data();
    void testStatusParsing();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
    void testGitRemote_data();
    void testGitRemote();
#endif

};

} // namespace Internal
} // namespace Git
