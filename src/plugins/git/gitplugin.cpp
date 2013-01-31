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

#include "gitplugin.h"

#include "changeselectiondialog.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"
#include "branchdialog.h"
#include "remotedialog.h"
#include "clonewizard.h"
#include "gitorious/gitoriousclonewizard.h"
#include "stashdialog.h"
#include "settingspage.h"
#include "resetdialog.h"
#include "mergetool.h"

#include "gerrit/gerritplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/documentmanager.h>

#include <utils/qtcassert.h>
#include <utils/parameteraction.h>
#include <utils/fileutils.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/cleandialog.h>
#include <locator/commandlocator.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QtPlugin>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

static const VcsBase::VcsBaseEditorParameters editorParameters[] = {
{
    VcsBase::RegularCommandOutput,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_ID,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_COMMAND_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_commandlog",
    "gitlog"},
{   VcsBase::LogOutput,
    Git::Constants::GIT_LOG_EDITOR_ID,
    Git::Constants::GIT_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_filelog",
    "gitfilelog"},
{   VcsBase::AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_ID,
    Git::Constants::GIT_BLAME_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_BLAME_EDITOR,
    "application/vnd.nokia.text.scs_git_annotation",
    "gitsannotate"},
{   VcsBase::DiffOutput,
    Git::Constants::GIT_DIFF_EDITOR_ID,
    Git::Constants::GIT_DIFF_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_DIFF_EDITOR,
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VcsBase::VcsBaseEditorParameters *findType(int ie)
{
    const VcsBase::EditorContentType et = static_cast<VcsBase::EditorContentType>(ie);
    return  VcsBase::VcsBaseEditorWidget::findType(editorParameters, sizeof(editorParameters)/sizeof(VcsBase::VcsBaseEditorParameters), et);
}

Q_DECLARE_METATYPE(Git::Internal::GitClientMemberFunc)

using namespace Git;
using namespace Git::Internal;

// GitPlugin

GitPlugin *GitPlugin::m_instance = 0;

GitPlugin::GitPlugin() :
    VcsBase::VcsBasePlugin(QLatin1String(Git::Constants::GITSUBMITEDITOR_ID)),
    m_commandLocator(0),
    m_showAction(0),
    m_submitCurrentAction(0),
    m_diffSelectedFilesAction(0),
    m_undoAction(0),
    m_redoAction(0),
    m_menuAction(0),
    m_applyCurrentFilePatchAction(0),
    m_gitClient(0),
    m_changeSelectionDialog(0),
    m_submitActionTriggered(false)
{
    m_instance = this;
    const int mid = qRegisterMetaType<GitClientMemberFunc>();
    Q_UNUSED(mid)
    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(15);
}

GitPlugin::~GitPlugin()
{
    cleanCommitMessageFile();
    delete m_gitClient;
    m_instance = 0;
}

void GitPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
    }
}

bool GitPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

GitPlugin *GitPlugin::instance()
{
    return m_instance;
}

static const VcsBase::VcsBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    Git::Constants::C_GITSUBMITEDITOR,
    VcsBase::VcsBaseSubmitEditorParameters::DiffRows
};

// Create a parameter action
ParameterActionCommandPair
        GitPlugin::createParameterAction(Core::ActionContainer *ac,
                                         const QString &defaultText, const QString &parameterText,
                                         const Core::Id &id, const Core::Context &context,
                                         bool addToLocator)
{
    Utils::ParameterAction *action = new Utils::ParameterAction(defaultText, parameterText,
                                                                Utils::ParameterAction::EnabledWithParameter,
                                                                this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    ac->addAction(command);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return ParameterActionCommandPair(action, command);
}

// Create an action to act on a file with a slot.
ParameterActionCommandPair
        GitPlugin::createFileAction(Core::ActionContainer *ac,
                                    const QString &defaultText, const QString &parameterText,
                                    const Core::Id &id, const Core::Context &context, bool addToLocator,
                                    const char *pluginSlot)
{
    const ParameterActionCommandPair rc = createParameterAction(ac, defaultText, parameterText, id, context, addToLocator);
    m_fileActions.push_back(rc.first);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Create an action to act on a project with slot.
ParameterActionCommandPair
        GitPlugin::createProjectAction(Core::ActionContainer *ac,
                                       const QString &defaultText, const QString &parameterText,
                                       const Core::Id &id, const Core::Context &context, bool addToLocator,
                                       const char *pluginSlot)
{
    const ParameterActionCommandPair rc = createParameterAction(ac, defaultText, parameterText, id, context, addToLocator);
    m_projectActions.push_back(rc.first);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Create an action to act on the repository
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionContainer *ac,
                                          const QString &text, const Core::Id &id,
                                          const Core::Context &context, bool addToLocator)
{
    QAction  *action = new QAction(text, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    ac->addAction(command);
    m_repositoryActions.push_back(action);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return ActionCommandPair(action, command);
}

// Create an action to act on the repository with slot
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionContainer *ac,
                                          const QString &text, const Core::Id &id,
                                          const Core::Context &context, bool addToLocator,
                                          const char *pluginSlot)
{
    const ActionCommandPair rc = createRepositoryAction(ac, text, id, context, addToLocator);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Action to act on the repository forwarded to a git client member function
// taking the directory. Store the member function as data on the action.
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionContainer *ac,
                                          const QString &text, const Core::Id &id,
                                          const Core::Context &context, bool addToLocator,
                                          GitClientMemberFunc func)
{
    // Set the member func as data and connect to generic slot
    const ActionCommandPair rc = createRepositoryAction(ac, text, id, context, addToLocator);
    rc.first->setData(qVariantFromValue(func));
    connect(rc.first, SIGNAL(triggered()), this, SLOT(gitClientMemberFuncRepositoryAction()));
    return rc;
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    m_settings.readSettings(Core::ICore::settings());

    m_gitClient = new GitClient(&m_settings);

    typedef VcsBase::VcsEditorFactory<GitEditor> GitEditorFactory;
    typedef VcsBase::VcsSubmitEditorFactory<GitSubmitEditor> GitSubmitEditorFactory;

    initializeVcs(new GitVersionControl(m_gitClient));

    // Create the globalcontext list to register actions accordingly
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    // Create the settings Page
    addAutoReleasedObject(new SettingsPage());

    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VcsBase::VcsBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new GitEditorFactory(editorParameters + i, m_gitClient, describeSlot));

    addAutoReleasedObject(new GitSubmitEditorFactory(&submitParameters));
    addAutoReleasedObject(new CloneWizard);
    addAutoReleasedObject(new Gitorious::Internal::GitoriousCloneWizard);

    const QString prefix = QLatin1String("git");
    m_commandLocator = new Locator::CommandLocator("Git", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    //register actions
    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *gitContainer = Core::ActionManager::createMenu("Git");
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();

    /*  "Current File" menu */
    Core::ActionContainer *currentFileMenu = Core::ActionManager::createMenu(Core::Id("Git.CurrentFileMenu"));
    currentFileMenu->menu()->setTitle(tr("Current &File"));
    gitContainer->addMenu(currentFileMenu);

    ParameterActionCommandPair parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Diff Current File"), tr("Diff of \"%1\""),
                               Core::Id("Git.Diff"), globalcontext, true,
                               SLOT(diffCurrentFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+D") : tr("Alt+G,Alt+D")));

    parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Log Current File"), tr("Log of \"%1\""),
                               Core::Id("Git.Log"), globalcontext, true, SLOT(logFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+L") : tr("Alt+G,Alt+L")));

    parameterActionCommand
                = createFileAction(currentFileMenu,
                                   tr("Blame Current File"), tr("Blame for \"%1\""),
                                   Core::Id("Git.Blame"),
                                   globalcontext, true, SLOT(blameFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+B") : tr("Alt+G,Alt+B")));

    // ------
    currentFileMenu->addSeparator(globalcontext);

    parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Stage File for Commit"), tr("Stage \"%1\" for Commit"),
                               Core::Id("Git.Stage"), globalcontext, true, SLOT(stageFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+A") : tr("Alt+G,Alt+A")));

    parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Unstage File from Commit"), tr("Unstage \"%1\" from Commit"),
                               Core::Id("Git.Unstage"), globalcontext, true, SLOT(unstageFile()));

    parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Undo Unstaged Changes"), tr("Undo Unstaged Changes for \"%1\""),
                               Core::Id("Git.UndoUnstaged"), globalcontext,
                               true, SLOT(undoUnstagedFileChanges()));

    parameterActionCommand
            = createFileAction(currentFileMenu,
                               tr("Undo Uncommitted Changes"), tr("Undo Uncommitted Changes for \"%1\""),
                               Core::Id("Git.Undo"), globalcontext,
                               true, SLOT(undoFileChanges()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+U") : tr("Alt+G,Alt+U")));

    /* \"Current File" menu */

    // ------------

    /*  "Current Project" menu */
    Core::ActionContainer *currentProjectMenu = Core::ActionManager::createMenu(Core::Id("Git.CurrentProjectMenu"));
    currentProjectMenu->menu()->setTitle(tr("Current &Project"));
    gitContainer->addMenu(currentProjectMenu);

    parameterActionCommand
            = createProjectAction(currentProjectMenu,
                                  tr("Diff Current Project"), tr("Diff Project \"%1\""),
                                  Core::Id("Git.DiffProject"),
                                  globalcontext, true,
                                  SLOT(diffCurrentProject()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+Shift+D") : tr("Alt+G,Alt+Shift+D")));

    parameterActionCommand
            = createProjectAction(currentProjectMenu,
                                  tr("Log Project"), tr("Log Project \"%1\""),
                                  Core::Id("Git.LogProject"), globalcontext, true,
                                  SLOT(logProject()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+K") : tr("Alt+G,Alt+K")));

    parameterActionCommand
                = createProjectAction(currentProjectMenu,
                                      tr("Clean Project..."), tr("Clean Project \"%1\"..."),
                                      Core::Id("Git.CleanProject"), globalcontext,
                                      true, SLOT(cleanProject()));

    /* \"Current Project" menu */

    // --------------

    /*  "Local Repository" menu */
    Core::ActionContainer *localRepositoryMenu = Core::ActionManager::createMenu(Core::Id("Git.LocalRepositoryMenu"));
    localRepositoryMenu->menu()->setTitle(tr("&Local Repository"));
    gitContainer->addMenu(localRepositoryMenu);

    createRepositoryAction(localRepositoryMenu,
                           tr("Diff"), Core::Id("Git.DiffRepository"),
                           globalcontext, true, SLOT(diffRepository()));

    createRepositoryAction(localRepositoryMenu,
                           tr("Log"), Core::Id("Git.LogRepository"),
                           globalcontext, true, &GitClient::graphLog);

    createRepositoryAction(localRepositoryMenu,
                           tr("Clean..."), Core::Id("Git.CleanRepository"),
                           globalcontext, true, SLOT(cleanRepository()));

    createRepositoryAction(localRepositoryMenu,
                           tr("Status"), Core::Id("Git.StatusRepository"),
                           globalcontext, true, &GitClient::status);

    // --------------
    localRepositoryMenu->addSeparator(globalcontext);

    ActionCommandPair actionCommand = createRepositoryAction(localRepositoryMenu,
                                                             tr("Commit..."), Core::Id("Git.Commit"),
                                                             globalcontext, true, SLOT(startCommit()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+G,Meta+C") : tr("Alt+G,Alt+C")));

    createRepositoryAction(localRepositoryMenu,
                           tr("Amend Last Commit..."), Core::Id("Git.AmendCommit"),
                           globalcontext, true, SLOT(startAmendCommit()));
    // --------------
    localRepositoryMenu->addSeparator(globalcontext);

    createRepositoryAction(localRepositoryMenu,
                           tr("Reset..."), Core::Id("Git.Reset"),
                           globalcontext, false, SLOT(resetRepository()));

    createRepositoryAction(localRepositoryMenu,
                           tr("Revert Single Commit..."), Core::Id("Git.Revert"),
                           globalcontext, true, SLOT(startRevertCommit()));

    createRepositoryAction(localRepositoryMenu,
                           tr("Cherry-Pick Commit..."), Core::Id("Git.CherryPick"),
                           globalcontext, true, SLOT(startCherryPickCommit()));

    // --------------
    localRepositoryMenu->addSeparator(globalcontext);

    createRepositoryAction(localRepositoryMenu,
                           tr("Branches..."), Core::Id("Git.BranchList"),
                           globalcontext, true, SLOT(branchList()));

    // --------------
    localRepositoryMenu->addSeparator(globalcontext);

    // "Patch" menu
    Core::ActionContainer *patchMenu = Core::ActionManager::createMenu(Core::Id("Git.PatchMenu"));
    patchMenu->menu()->setTitle(tr("&Patch"));
    localRepositoryMenu->addMenu(patchMenu);

    // Apply current file as patch is handled specially.
    parameterActionCommand =
            createParameterAction(patchMenu,
                                  tr("Apply from Editor"), tr("Apply \"%1\""),
                                  Core::Id("Git.ApplyCurrentFilePatch"),
                                  globalcontext, true);
    m_applyCurrentFilePatchAction = parameterActionCommand.first;
    connect(m_applyCurrentFilePatchAction, SIGNAL(triggered()), this,
            SLOT(applyCurrentFilePatch()));

    createRepositoryAction(patchMenu,
                           tr("Apply from File..."), Core::Id("Git.ApplyPatch"),
                           globalcontext, true, SLOT(promptApplyPatch()));

    // "Stash" menu
    Core::ActionContainer *stashMenu = Core::ActionManager::createMenu(Core::Id("Git.StashMenu"));
    stashMenu->menu()->setTitle(tr("&Stash"));
    localRepositoryMenu->addMenu(stashMenu);

    createRepositoryAction(stashMenu,
                           tr("Stashes..."), Core::Id("Git.StashList"),
                           globalcontext, false, SLOT(stashList()));

    stashMenu->addSeparator(globalcontext);

    actionCommand = createRepositoryAction(stashMenu,
                                           tr("Stash"), Core::Id("Git.Stash"),
                                           globalcontext, true, SLOT(stash()));
    actionCommand.first->setToolTip(tr("Saves the current state of your work and resets the repository."));

    actionCommand = createRepositoryAction(stashMenu,
                                           tr("Take Snapshot..."), Core::Id("Git.StashSnapshot"),
                                           globalcontext, true, SLOT(stashSnapshot()));
    actionCommand.first->setToolTip(tr("Saves the current state of your work."));

    stashMenu->addSeparator(globalcontext);

    actionCommand = createRepositoryAction(stashMenu,
                                           tr("Stash Pop"), Core::Id("Git.StashPop"),
                                           globalcontext, true, &GitClient::stashPop);
    actionCommand.first->setToolTip(tr("Restores changes saved to the stash list using \"Stash\"."));


    /* \"Local Repository" menu */

    // --------------

    /*  "Remote Repository" menu */
    Core::ActionContainer *remoteRepositoryMenu = Core::ActionManager::createMenu(Core::Id("Git.RemoteRepositoryMenu"));
    remoteRepositoryMenu->menu()->setTitle(tr("&Remote Repository"));
    gitContainer->addMenu(remoteRepositoryMenu);

    createRepositoryAction(remoteRepositoryMenu,
                           tr("Fetch"), Core::Id("Git.Fetch"),
                           globalcontext, true, SLOT(fetch()));

    createRepositoryAction(remoteRepositoryMenu,
                           tr("Pull"), Core::Id("Git.Pull"),
                           globalcontext, true, SLOT(pull()));

    actionCommand = createRepositoryAction(remoteRepositoryMenu,
                                           tr("Push"), Core::Id("Git.Push"),
                                           globalcontext, true, SLOT(push()));

    // --------------
    remoteRepositoryMenu->addSeparator(globalcontext);

    // "Subversion" menu
    Core::ActionContainer *subversionMenu = Core::ActionManager::createMenu(Core::Id("Git.Subversion"));
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    remoteRepositoryMenu->addMenu(subversionMenu);

    createRepositoryAction(subversionMenu,
                           tr("Log"), Core::Id("Git.Subversion.Log"),
                           globalcontext, false, &GitClient::subversionLog);

    createRepositoryAction(subversionMenu,
                           tr("Fetch"), Core::Id("Git.Subversion.Fetch"),
                           globalcontext, false, &GitClient::synchronousSubversionFetch);

    // --------------
    remoteRepositoryMenu->addSeparator(globalcontext);

    createRepositoryAction(remoteRepositoryMenu,
                           tr("Manage Remotes..."), Core::Id("Git.RemoteList"),
                           globalcontext, false, SLOT(remoteList()));

    /* \"Remote Repository" menu */

    // --------------

    /*  "Git Tools" menu */
    Core::ActionContainer *gitToolsMenu = Core::ActionManager::createMenu(Core::Id("Git.GitToolsMenu"));
    gitToolsMenu->menu()->setTitle(tr("Git &Tools"));
    gitContainer->addMenu(gitToolsMenu);

    createRepositoryAction(gitToolsMenu,
                           tr("Gitk"), Core::Id("Git.LaunchGitK"),
                           globalcontext, true, &GitClient::launchGitK);

    parameterActionCommand
            = createFileAction(gitToolsMenu,
                               tr("Gitk Current File"), tr("Gitk of \"%1\""),
                               Core::Id("Git.GitkFile"), globalcontext, true, SLOT(gitkForCurrentFile()));

    parameterActionCommand
            = createFileAction(gitToolsMenu,
                               tr("Gitk for folder of Current File"), tr("Gitk for folder of \"%1\""),
                               Core::Id("Git.GitkFolder"), globalcontext, true, SLOT(gitkForCurrentFolder()));

    // --------------
    gitToolsMenu->addSeparator(globalcontext);

    m_repositoryBrowserAction
            = createRepositoryAction(gitToolsMenu,
                                     tr("Repository Browser"), Core::Id("Git.LaunchRepositoryBrowser"),
                                     globalcontext, true, &GitClient::launchRepositoryBrowser).first;

    createRepositoryAction(gitToolsMenu,
                           tr("Merge Tool"), Core::Id("Git.MergeTool"),
                           globalcontext, true, SLOT(startMergeTool()));

    /* \"Git Tools" menu */

    // --------------
    gitContainer->addSeparator(globalcontext);

    m_showAction = new QAction(tr("Show..."), this);
    Core::Command *showCommitCommand = Core::ActionManager::registerAction(m_showAction, "Git.ShowCommit", globalcontext);
    connect(m_showAction, SIGNAL(triggered()), this, SLOT(showCommit()));
    gitContainer->addAction(showCommitCommand);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    Core::Command *createRepositoryCommand = Core::ActionManager::registerAction(m_createRepositoryAction, "Git.CreateRepository", globalcontext);
    connect(m_createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    gitContainer->addAction(createRepositoryCommand);

    if (0) {
        const QList<QAction*> snapShotActions = createSnapShotTestActions();
        const int count = snapShotActions.size();
        for (int i = 0; i < count; i++) {
            Core::Command *tCommand
                    = Core::ActionManager::registerAction(snapShotActions.at(i),
                                                    Core::Id(QLatin1String("Git.Snapshot.") + QString::number(i)),
                                                    globalcontext);
            gitContainer->addAction(tCommand);
        }
    }

    // Submit editor
    Core::Context submitContext(Constants::C_GITSUBMITEDITOR);
    m_submitCurrentAction = new QAction(VcsBase::VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Core::Command *command = Core::ActionManager::registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_submitCurrentAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFilesAction = new QAction(VcsBase::VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = Core::ActionManager::registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = Core::ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = Core::ActionManager::registerAction(m_redoAction, Core::Constants::REDO, submitContext);


    /* "Gerrit" */
    Gerrit::Internal::GerritPlugin *gp = new Gerrit::Internal::GerritPlugin(this);
    return gp->initialize(remoteRepositoryMenu);
}

GitVersionControl *GitPlugin::gitVersionControl() const
{
    return static_cast<GitVersionControl *>(versionControl());
}

void GitPlugin::submitEditorDiff(const QStringList &unstaged, const QStringList &staged)
{
    m_gitClient->diff(m_submitRepository, QStringList(), unstaged, staged);
}

void GitPlugin::submitEditorMerge(const QStringList &unmerged)
{
    m_gitClient->merge(m_submitRepository, unmerged);
}

void GitPlugin::diffCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->diff(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile());
}

void GitPlugin::diffCurrentProject()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    m_gitClient->diff(state.currentProjectTopLevel(), QStringList(), state.relativeCurrentProject());
}

void GitPlugin::diffRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->diff(state.topLevel(), QStringList(), QStringList());
}

void GitPlugin::logFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void GitPlugin::blameFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const int lineNumber = VcsBase::VcsBaseEditorWidget::lineNumberOfCurrentEditor(state.currentFile());
    m_gitClient->blame(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile(), QString(), lineNumber);
}

void GitPlugin::logProject()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    m_gitClient->log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPlugin::undoFileChanges(bool revertStaging)
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    Core::FileChangeBlocker fcb(state.currentFile());
    m_gitClient->revert(QStringList(state.currentFile()), revertStaging);
}

void GitPlugin::undoUnstagedFileChanges()
{
    undoFileChanges(false);
}

void GitPlugin::resetRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    ResetDialog dialog;
    if (dialog.runDialog(state.topLevel()))
        switch (dialog.resetType()) {
        case HardReset:
            m_gitClient->hardReset(state.topLevel(), dialog.commit());
            break;
        case SoftReset:
            m_gitClient->softReset(state.topLevel(), dialog.commit());
            break;
        }
}

void GitPlugin::startRevertCommit()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QString workingDirectory = state.currentDirectoryOrTopLevel();
    if (workingDirectory.isEmpty())
        return;
    GitClient::StashGuard stashGuard(workingDirectory, QLatin1String("Revert"));
    if (stashGuard.stashingFailed(true))
        return;
    ChangeSelectionDialog changeSelectionDialog(workingDirectory);

    if (changeSelectionDialog.exec() != QDialog::Accepted)
        return;
    const QString change = changeSelectionDialog.change();
    if (!change.isEmpty() && !m_gitClient->revertCommit(workingDirectory, change))
        stashGuard.preventPop();
}

void GitPlugin::startCherryPickCommit()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QString workingDirectory = state.currentDirectoryOrTopLevel();
    if (workingDirectory.isEmpty())
        return;
    GitClient::StashGuard stashGuard(state.topLevel(), QLatin1String("Cherry-pick"));
    if (stashGuard.stashingFailed(true))
        return;
    ChangeSelectionDialog changeSelectionDialog(workingDirectory);

    if (changeSelectionDialog.exec() != QDialog::Accepted)
        return;
    const QString change = changeSelectionDialog.change();
    if (!change.isEmpty() && !m_gitClient->cherryPickCommit(workingDirectory, change))
        stashGuard.preventPop();
}

void GitPlugin::stageFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::unstageFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->synchronousReset(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void GitPlugin::gitkForCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::gitkForCurrentFolder()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    /*
     *  entire lower part of the code can be easily replaced with one line:
     *
     *  m_gitClient->launchGitK(dir.currentFileDirectory(), QLatin1String("."));
     *
     *  However, there is a bug in gitk in version 1.7.9.5, and if you run above
     *  command, there will be no documents listed in lower right section.
     *
     *  This is why I use lower combination in order to avoid this problems in gitk.
     *
     *  Git version 1.7.10.4 does not have this issue, and it can easily use
     *  one line command mentioned above.
     *
     */
    QDir dir(state.currentFileDirectory());
    if (QFileInfo(dir,QLatin1String(".git")).exists() || dir.cd(QLatin1String(".git")))
        m_gitClient->launchGitK(state.currentFileDirectory());
    else {
        QString folderName = dir.absolutePath();
        dir.cdUp();
        folderName = folderName.remove(0, dir.absolutePath().length() + 1);
        m_gitClient->launchGitK(dir.absolutePath(), folderName);
    }
}

void GitPlugin::startAmendCommit()
{
    startCommit(true);
}

void GitPlugin::startCommit()
{
    startCommit(false);
}

void GitPlugin::startCommit(bool amend)
{
    if (VcsBase::VcsBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsBase::VcsBaseOutputWindow::instance()->appendWarning(tr("Another submit is currently being executed."));
        return;
    }

    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QString errorMessage, commitTemplate;
    CommitData data;
    if (!m_gitClient->getCommitData(state.topLevel(), amend, &commitTemplate, &data, &errorMessage)) {
        VcsBase::VcsBaseOutputWindow::instance()->append(errorMessage);
        return;
    }

    // Store repository for diff and the original list of
    // files to be able to unstage files the user unchecks
    m_submitRepository = data.panelInfo.repository;
    m_commitAmendSHA1 = data.amendSHA1;

    // Start new temp file with message template
    Utils::TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    saver.write(commitTemplate.toLocal8Bit());
    if (!saver.finalize()) {
        VcsBase::VcsBaseOutputWindow::instance()->append(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    openSubmitEditor(m_commitMessageFileName, data, amend);
}

Core::IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd, bool amend)
{
    Core::IEditor *editor = Core::EditorManager::openEditor(fileName, Constants::GITSUBMITEDITOR_ID,
                                                Core::EditorManager::ModeSwitch);
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentAction, m_diffSelectedFilesAction);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    const QString title = amend ? tr("Amend %1").arg(cd.amendSHA1) : tr("Git Commit");
    submitEditor->setDisplayName(title);
    submitEditor->setAmend(amend);
    connect(submitEditor, SIGNAL(diff(QStringList,QStringList)), this, SLOT(submitEditorDiff(QStringList,QStringList)));
    connect(submitEditor, SIGNAL(merge(QStringList)), this, SLOT(submitEditorMerge(QStringList)));
    return editor;
}

void GitPlugin::submitCurrentLog()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    Core::ICore::editorManager()->closeEditor();
}

bool GitPlugin::submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return false;
    Core::IDocument *editorDocument = submitEditor->document();
    const GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(submitEditor);
    if (!editorDocument || !editor)
        return true;
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(editorDocument->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool *promptData = m_settings.boolPointer(GitSettings::promptOnSubmitKey);
    const VcsBase::VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing Git Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("Git will not accept this commit. Do you want to continue to edit it?"),
                                 promptData, !m_submitActionTriggered, false);
    m_submitActionTriggered = false;
    switch (answer) {
    case VcsBase::VcsBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VcsBase::VcsBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }


    // Go ahead!
    VcsBase::SubmitFileModel *model = qobject_cast<VcsBase::SubmitFileModel *>(editor->fileModel());
    bool closeEditor = true;
    if (model->hasCheckedFiles() || !m_commitAmendSHA1.isEmpty()) {
        // get message & commit
        if (!Core::DocumentManager::saveDocument(editorDocument))
            return false;

        closeEditor = m_gitClient->addAndCommit(m_submitRepository, editor->panelData(),
                                                m_commitAmendSHA1, m_commitMessageFileName, model);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void GitPlugin::fetch()
{
    m_gitClient->synchronousFetch(currentState().topLevel(), QString());
}

void GitPlugin::pull()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    bool rebase = m_gitClient->settings()->boolValue(GitSettings::pullRebaseKey);

    if (!rebase) {
        bool isDetached;
        QString branchRebaseConfig = m_gitClient->synchronousRepositoryBranches(state.topLevel(), &isDetached).at(0);
        if (!isDetached) {
            branchRebaseConfig.prepend(QLatin1String("branch."));
            branchRebaseConfig.append(QLatin1String(".rebase"));
            rebase = (m_gitClient->readConfigValue(state.topLevel(), branchRebaseConfig) == QLatin1String("true"));
        }
    }

    GitClient::StashGuard stashGuard(state.topLevel(), QLatin1String("Pull"));
    if (stashGuard.stashingFailed(false) || (rebase && (stashGuard.result() == GitClient::NotStashed)))
        return;
    if (!m_gitClient->synchronousPull(state.topLevel(), false))
        stashGuard.preventPop();
}

void GitPlugin::push()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->synchronousPush(state.topLevel());
}

void GitPlugin::startMergeTool()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->merge(state.topLevel());
}

// Retrieve member function of git client stored as user data of action
static inline GitClientMemberFunc memberFunctionFromAction(const QObject *o)
{
    if (o) {
        if (const QAction *action = qobject_cast<const QAction *>(o)) {
            const QVariant v = action->data();
            if (v.canConvert<GitClientMemberFunc>())
                return qvariant_cast<GitClientMemberFunc>(v);
        }
    }
    return 0;
}

void GitPlugin::gitClientMemberFuncRepositoryAction()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    // Retrieve member function and invoke on repository
    GitClientMemberFunc func = memberFunctionFromAction(sender());
    QTC_ASSERT(func, return);
    (m_gitClient->*func)(state.topLevel());
}

void GitPlugin::cleanProject()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    cleanRepository(state.currentProjectPath());
}

void GitPlugin::cleanRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    cleanRepository(state.topLevel());
}

void GitPlugin::cleanRepository(const QString &directory)
{
    // Find files to be deleted
    QString errorMessage;
    QStringList files;
    QStringList ignoredFiles;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool gotFiles = m_gitClient->synchronousCleanList(directory, &files, &ignoredFiles, &errorMessage);
    QApplication::restoreOverrideCursor();

    QWidget *parent = Core::ICore::mainWindow();
    if (!gotFiles) {
        QMessageBox::warning(parent, tr("Unable to retrieve file list"), errorMessage);
        return;
    }
    if (files.isEmpty() && ignoredFiles.isEmpty()) {
        QMessageBox::information(parent, tr("Repository Clean"),
                                 tr("The repository is clean."));
        return;
    }

    // Show in dialog
    VcsBase::CleanDialog dialog(parent);
    dialog.setFileList(directory, files, ignoredFiles);
    dialog.exec();
}

// If the file is modified in an editor, make sure it is saved.
static bool ensureFileSaved(const QString &fileName)
{
    const QList<Core::IEditor*> editors = Core::EditorManager::instance()->editorsForFileName(fileName);
    if (editors.isEmpty())
        return true;
    Core::IDocument *document = editors.front()->document();
    if (!document || !document->isModified())
        return true;
    bool canceled;
    QList<Core::IDocument *> documents;
    documents << document;
    Core::DocumentManager::saveModifiedDocuments(documents, &canceled);
    return !canceled;
}

void GitPlugin::applyCurrentFilePatch()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasPatchFile() && state.hasTopLevel(), return);
    const QString patchFile = state.currentPatchFile();
    if (!ensureFileSaved(patchFile))
        return;
    applyPatch(state.topLevel(), patchFile);
}

void GitPlugin::promptApplyPatch()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    applyPatch(state.topLevel(), QString());
}

void GitPlugin::applyPatch(const QString &workingDirectory, QString file)
{
    // Ensure user has been notified about pending changes
    GitClient::StashGuard stashGuard(workingDirectory, QLatin1String("Apply-Patch"));
    if (stashGuard.stashingFailed(false))
        return;
    // Prompt for file
    if (file.isEmpty()) {
        const QString filter = tr("Patches (*.patch *.diff)");
        file =  QFileDialog::getOpenFileName(Core::ICore::mainWindow(),
                                             tr("Choose Patch"),
                                             QString(), filter);
        if (file.isEmpty())
            return;
    }
    // Run!
    VcsBase::VcsBaseOutputWindow *outwin = VcsBase::VcsBaseOutputWindow::instance();
    QString errorMessage;
    if (m_gitClient->synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
        if (errorMessage.isEmpty())
            outwin->append(tr("Patch %1 successfully applied to %2").arg(file, workingDirectory));
        else
            outwin->append(errorMessage);
    } else {
        outwin->appendError(errorMessage);
    }
}

void GitPlugin::stash()
{
    // Simple stash without prompt, reset repo.
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString id;
    gitClient()->ensureStash(state.topLevel(), QString(), false, &id);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

void GitPlugin::stashSnapshot()
{
    // Prompt for description, restore immediately and keep on working.
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString id = m_gitClient->synchronousStash(state.topLevel(), QString(),
                GitClient::StashImmediateRestore|GitClient::StashPromptDescription);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

// Create a non-modal dialog with refresh method or raise if it exists
template <class NonModalDialog>
    inline void showNonModalDialog(const QString &topLevel,
                                   QPointer<NonModalDialog> &dialog)
{
    if (dialog) {
        dialog->show();
        dialog->raise();
    } else {
        dialog = new NonModalDialog(Core::ICore::mainWindow());
        dialog->refresh(topLevel, true);
        dialog->show();
    }
}

void GitPlugin::branchList()
{
   showNonModalDialog(currentState().topLevel(), m_branchDialog);
}

void GitPlugin::remoteList()
{
    showNonModalDialog(currentState().topLevel(), m_remoteDialog);
}

void GitPlugin::stashList()
{
    showNonModalDialog(currentState().topLevel(), m_stashDialog);
}

void GitPlugin::updateActions(VcsBase::VcsBasePlugin::ActionState as)
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    if (m_stashDialog)
        m_stashDialog->refresh(currentState().topLevel(), false);
    if (m_branchDialog)
        m_branchDialog->refresh(currentState().topLevel(), false);
    if (m_remoteDialog)
        m_remoteDialog->refresh(currentState().topLevel(), false);

    m_commandLocator->setEnabled(repositoryEnabled);
    if (!enableMenuAction(as, m_menuAction))
        return;
    // Note: This menu is visible if there is no repository. Only
    // 'Create Repository'/'Show' actions should be available.
    const QString fileName = currentState().currentFileName();
    foreach (Utils::ParameterAction *fileAction, m_fileActions)
        fileAction->setParameter(fileName);
    // If the current file looks like a patch, offer to apply
    m_applyCurrentFilePatchAction->setParameter(currentState().currentPatchFileDisplayName());

    const QString projectName = currentState().currentProjectName();
    foreach (Utils::ParameterAction *projectAction, m_projectActions)
        projectAction->setParameter(projectName);

    foreach (QAction *repositoryAction, m_repositoryActions)
        repositoryAction->setEnabled(repositoryEnabled);
    updateRepositoryBrowserAction();

    // Prompts for repo.
    m_showAction->setEnabled(true);
}

void GitPlugin::updateRepositoryBrowserAction()
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    const bool hasRepositoryBrowserCmd = !settings().stringValue(GitSettings::repositoryBrowserCmd).isEmpty();
    m_repositoryBrowserAction->setEnabled(repositoryEnabled && hasRepositoryBrowserCmd);
}

void GitPlugin::showCommit()
{
    const VcsBase::VcsBasePluginState state = currentState();

    if (!m_changeSelectionDialog)
        m_changeSelectionDialog = new ChangeSelectionDialog();

    m_changeSelectionDialog->setWorkingDirectory(state.currentDirectoryOrTopLevel());

    if (m_changeSelectionDialog->exec() != QDialog::Accepted)
        return;
    const QString change = m_changeSelectionDialog->change();
    if (change.isEmpty())
        return;

    m_gitClient->show(m_changeSelectionDialog->workingDirectory(), change);
}

const GitSettings &GitPlugin::settings() const
{
    return m_settings;
}

void GitPlugin::setSettings(const GitSettings &s)
{
    if (s == m_settings)
        return;

    m_settings = s;
    m_gitClient->saveSettings();
    static_cast<GitVersionControl *>(versionControl())->emitConfigurationChanged();
    updateRepositoryBrowserAction();
}

GitClient *GitPlugin::gitClient() const
{
    return m_gitClient;
}

#ifdef WITH_TESTS
#include "giteditor.h"

#include <QTest>
#include <QTextBlock>
#include <QTextDocument>

Q_DECLARE_METATYPE(FileStates)

void GitPlugin::testStatusParsing_data()
{
    QTest::addColumn<FileStates>("first");
    QTest::addColumn<FileStates>("second");

    QTest::newRow(" M") << FileStates(ModifiedFile) << FileStates(UnknownFileState);
    QTest::newRow(" D") << FileStates(DeletedFile) << FileStates(UnknownFileState);
    QTest::newRow("M ") << (ModifiedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("MM") << (ModifiedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("MD") << (ModifiedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("A ") << (AddedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("AM") << (AddedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("AD") << (AddedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("D ") << (DeletedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("DM") << (DeletedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("R ") << (RenamedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("RM") << (RenamedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("RD") << (RenamedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("C ") << (CopiedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("CM") << (CopiedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("CD") << (CopiedFile | StagedFile) << FileStates(DeletedFile);

    // Merges
    QTest::newRow("DD") << (DeletedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("AA") << (AddedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("UU") << (ModifiedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("AU") << (AddedFile | UnmergedFile | UnmergedUs) << FileStates(UnknownFileState);
    QTest::newRow("UD") << (DeletedFile | UnmergedFile | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("UA") << (AddedFile | UnmergedFile | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("DU") << (DeletedFile | UnmergedFile | UnmergedUs) << FileStates(UnknownFileState);
}

void GitPlugin::testStatusParsing()
{
    CommitData data;
    QFETCH(FileStates, first);
    QFETCH(FileStates, second);
    QString output = QLatin1String("## master...origin/master [ahead 1]\n");
    output += QString::fromLatin1(QTest::currentDataTag()) + QLatin1String(" main.cpp\n");
    data.parseFilesFromStatus(output);
    QCOMPARE(data.files.at(0).first, first);
    if (second == UnknownFileState)
        QCOMPARE(data.files.size(), 1);
    else
        QCOMPARE(data.files.at(1).first, second);
}

void GitPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "new file mode 100644\n"
            "index 0000000..40997ff\n"
            "--- /dev/null\n"
            "+++ b/src/plugins/git/giteditor.cpp\n"
            "@@ -0,0 +1,281 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "deleted file mode 100644\n"
            "index 40997ff..0000000\n"
            "--- a/src/plugins/git/giteditor.cpp\n"
            "+++ /dev/null\n"
            "@@ -1,281 +0,0 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
    QTest::newRow("Normal") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "index 69e0b52..8fc974d 100644\n"
            "--- a/src/plugins/git/giteditor.cpp\n"
            "+++ b/src/plugins/git/giteditor.cpp\n"
            "@@ -49,6 +49,8 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
}

void GitPlugin::testDiffFileResolving()
{
    GitEditor editor(editorParameters + 3, 0);
    editor.testDiffFileResolving();
}

void GitPlugin::testLogResolving()
{
    QByteArray data(
                "commit 50a6b54c03219ad74b9f3f839e0321be18daeaf6 (HEAD, origin/master)\n"
                "Merge: 3587b51 bc93ceb\n"
                "Author: Junio C Hamano <gitster@pobox.com>\n"
                "Date:   Fri Jan 25 12:53:31 2013 -0800\n"
                "\n"
                "   Merge branch 'for-junio' of git://bogomips.org/git-svn\n"
                "    \n"
                "    * 'for-junio' of git://bogomips.org/git-svn:\n"
                "      git-svn: Simplify calculation of GIT_DIR\n"
                "      git-svn: cleanup sprintf usage for uppercasing hex\n"
                "\n"
                "commit 3587b513bafd7a83d8c816ac1deed72b5e3a27e9\n"
                "Author: Junio C Hamano <gitster@pobox.com>\n"
                "Date:   Fri Jan 25 12:52:55 2013 -0800\n"
                "\n"
                "    Update draft release notes to 1.8.2\n"
                "    \n"
                "    Signed-off-by: Junio C Hamano <gitster@pobox.com>\n"
                );
    GitEditor editor(editorParameters + 1, 0);
    editor.testLogResolving(data,
                            "50a6b54c - Merge branch 'for-junio' of git://bogomips.org/git-svn",
                            "3587b513 - Update draft release notes to 1.8.2");
}
#endif

Q_EXPORT_PLUGIN(GitPlugin)
