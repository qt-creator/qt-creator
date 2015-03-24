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
#include "stashdialog.h"
#include "settingspage.h"
#include "logchangedialog.h"
#include "mergetool.h"
#include "gitutils.h"

#include "gerrit/gerritplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/idocument.h>
#include <coreplugin/infobar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/vcsmanager.h>

#include <coreplugin/messagebox.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/parameteraction.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/cleandialog.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QtPlugin>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QScopedPointer>

#ifdef WITH_TESTS
#include "clonewizardpage.h"
#include <QTest>
#endif

Q_DECLARE_METATYPE(Git::Internal::FileStates)

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Git {
namespace Internal {

const unsigned minimumRequiredVersion = 0x010702;
const char RC_GIT_MIME_XML[] = ":/git/Git.mimetypes.xml";

const VcsBaseEditorParameters editorParameters[] = {
{
    OtherContent,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_ID,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.commandlog"},
{   LogOutput,
    Git::Constants::GIT_LOG_EDITOR_ID,
    Git::Constants::GIT_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.log"},
{   AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_ID,
    Git::Constants::GIT_BLAME_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.annotation"},
{   OtherContent,
    Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID,
    Git::Constants::GIT_COMMIT_TEXT_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.commit"},
{   OtherContent,
    Git::Constants::GIT_REBASE_EDITOR_ID,
    Git::Constants::GIT_REBASE_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.rebase"},
};


// GitPlugin

static GitPlugin *m_instance = 0;

GitPlugin::GitPlugin() :
    m_commandLocator(0),
    m_submitCurrentAction(0),
    m_diffSelectedFilesAction(0),
    m_undoAction(0),
    m_redoAction(0),
    m_menuAction(0),
    m_applyCurrentFilePatchAction(0),
    m_gitClient(0),
    m_submitActionTriggered(false)
{
    m_instance = this;
    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(50);
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

const VcsBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffRows
};

// Create a parameter action
ParameterAction *GitPlugin::createParameterAction(ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Id id, const Context &context,
                                                  bool addToLocator, const QKeySequence &keys)
{
    auto action = new ParameterAction(defaultText, parameterText, ParameterAction::EnabledWithParameter, this);
    Command *command = ActionManager::registerAction(action, id, context);
    if (!keys.isEmpty())
        command->setDefaultKeySequence(keys);
    command->setAttribute(Command::CA_UpdateText);
    ac->addAction(command);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return action;
}

// Create an action to act on a file with a slot.
QAction *GitPlugin::createFileAction(ActionContainer *ac,
                                     const QString &defaultText, const QString &parameterText,
                                     Id id, const Context &context, bool addToLocator,
                                     const char *pluginSlot, const QKeySequence &keys)
{
    ParameterAction *action = createParameterAction(ac, defaultText, parameterText, id, context, addToLocator, keys);
    m_fileActions.push_back(action);
    connect(action, SIGNAL(triggered()), this, pluginSlot);
    return action;
}

// Create an action to act on a project with slot.
QAction *GitPlugin::createProjectAction(ActionContainer *ac,
                                        const QString &defaultText, const QString &parameterText,
                                        Id id, const Context &context, bool addToLocator,
                                        const char *pluginSlot, const QKeySequence &keys)
{
    ParameterAction *action = createParameterAction(ac, defaultText, parameterText, id, context, addToLocator, keys);
    m_projectActions.push_back(action);
    connect(action, SIGNAL(triggered()), this, pluginSlot);
    return action;
}

// Create an action to act on the repository
QAction *GitPlugin::createRepositoryAction(ActionContainer *ac,
                                           const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           const QKeySequence &keys)
{
    auto action = new QAction(text, this);
    Command *command = ActionManager::registerAction(action, id, context);
    if (!keys.isEmpty())
        command->setDefaultKeySequence(keys);
    if (ac)
        ac->addAction(command);
    m_repositoryActions.push_back(action);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return action;
}

// Create an action to act on the repository with slot
QAction *GitPlugin::createRepositoryAction(ActionContainer *ac,
                                           const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           const char *pluginSlot, const QKeySequence &keys)
{
    QAction *action = createRepositoryAction(ac, text, id, context, addToLocator, keys);
    connect(action, SIGNAL(triggered()), this, pluginSlot);
    action->setData(id.uniqueIdentifier());
    return action;
}

// Action to act on the repository forwarded to a git client member function
// taking the directory.
QAction *GitPlugin::createRepositoryAction(ActionContainer *ac,
                                           const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           GitClientMemberFunc func, const QKeySequence &keys)
{
    // Set the member func as data and connect to generic slot
    QAction *action = createRepositoryAction(ac, text, id, context, addToLocator, keys);
    connect(action, &QAction::triggered, [this, func]() -> void {
        QTC_ASSERT(currentState().hasTopLevel(), return);
        (m_gitClient->*func)(currentState().topLevel());
    });
    return action;
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    Context context(Constants::GIT_CONTEXT);

    m_settings.readSettings(ICore::settings());

    m_gitClient = new GitClient(&m_settings);

    initializeVcs(new GitVersionControl(m_gitClient), context);

    // Create the settings Page
    addAutoReleasedObject(new SettingsPage());

    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new GitEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, m_gitClient, describeSlot));

    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new GitSubmitEditor(&submitParameters); }));

    auto cloneWizardFactory = new BaseCheckoutWizardFactory;
    cloneWizardFactory->setId(QLatin1String(VcsBase::Constants::VCS_ID_GIT));
    cloneWizardFactory->setIcon(QIcon(QLatin1String(":/git/images/git.png")));
    cloneWizardFactory->setDescription(tr("Clones a Git repository and tries to load the contained project."));
    cloneWizardFactory->setDisplayName(tr("Git Repository Clone"));
    cloneWizardFactory->setWizardCreator([this] (const FileName &path, QWidget *parent) {
        return new CloneWizard(path, parent);
    });
    addAutoReleasedObject(cloneWizardFactory);

    const QString prefix = QLatin1String("git");
    m_commandLocator = new CommandLocator("Git", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    //register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *gitContainer = ActionManager::createMenu("Git");
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();


    /*  "Current File" menu */
    ActionContainer *currentFileMenu = ActionManager::createMenu("Git.CurrentFileMenu");
    currentFileMenu->menu()->setTitle(tr("Current &File"));
    gitContainer->addMenu(currentFileMenu);

    createFileAction(currentFileMenu, tr("Diff Current File"), tr("Diff of \"%1\""),
                     "Git.Diff", context, true, SLOT(diffCurrentFile()),
                      QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+D") : tr("Alt+G,Alt+D")));

    createFileAction(currentFileMenu, tr("Log Current File"), tr("Log of \"%1\""),
                     "Git.Log", context, true, SLOT(logFile()),
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+L") : tr("Alt+G,Alt+L")));

    createFileAction(currentFileMenu, tr("Blame Current File"), tr("Blame for \"%1\""),
                     "Git.Blame", context, true, SLOT(blameFile()),
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+B") : tr("Alt+G,Alt+B")));

    currentFileMenu->addSeparator(context);

    createFileAction(currentFileMenu, tr("Stage File for Commit"), tr("Stage \"%1\" for Commit"),
                     "Git.Stage", context, true, SLOT(stageFile()),
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+A") : tr("Alt+G,Alt+A")));

    createFileAction(currentFileMenu, tr("Unstage File from Commit"), tr("Unstage \"%1\" from Commit"),
                     "Git.Unstage", context, true, SLOT(unstageFile()));

    createFileAction(currentFileMenu, tr("Undo Unstaged Changes"), tr("Undo Unstaged Changes for \"%1\""),
                     "Git.UndoUnstaged", context,
                     true, SLOT(undoUnstagedFileChanges()));

    createFileAction(currentFileMenu, tr("Undo Uncommitted Changes"), tr("Undo Uncommitted Changes for \"%1\""),
                     "Git.Undo", context,
                     true, SLOT(undoFileChanges()),
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+U") : tr("Alt+G,Alt+U")));


    /*  "Current Project" menu */
    ActionContainer *currentProjectMenu = ActionManager::createMenu("Git.CurrentProjectMenu");
    currentProjectMenu->menu()->setTitle(tr("Current &Project"));
    gitContainer->addMenu(currentProjectMenu);

    createProjectAction(currentProjectMenu, tr("Diff Current Project"), tr("Diff Project \"%1\""),
                        "Git.DiffProject", context, true, SLOT(diffCurrentProject()),
                        QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+Shift+D") : tr("Alt+G,Alt+Shift+D")));

    createProjectAction(currentProjectMenu, tr("Log Project"), tr("Log Project \"%1\""),
                        "Git.LogProject", context, true, SLOT(logProject()),
                        QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+K") : tr("Alt+G,Alt+K")));

    createProjectAction(currentProjectMenu, tr("Clean Project..."), tr("Clean Project \"%1\"..."),
                        "Git.CleanProject", context, true, SLOT(cleanProject()));


    /*  "Local Repository" menu */
    ActionContainer *localRepositoryMenu = ActionManager::createMenu("Git.LocalRepositoryMenu");
    localRepositoryMenu->menu()->setTitle(tr("&Local Repository"));
    gitContainer->addMenu(localRepositoryMenu);

    createRepositoryAction(localRepositoryMenu, tr("Diff"), "Git.DiffRepository",
                           context, true, SLOT(diffRepository()));

    createRepositoryAction(localRepositoryMenu, tr("Log"), "Git.LogRepository",
                           context, true,
                           SLOT(logRepository()));

    createRepositoryAction(localRepositoryMenu, tr("Reflog"), "Git.ReflogRepository",
                           context, true,
                           SLOT(reflogRepository()));

    createRepositoryAction(localRepositoryMenu, tr("Clean..."), "Git.CleanRepository",
                           context, true, SLOT(cleanRepository()));

    createRepositoryAction(localRepositoryMenu, tr("Status"), "Git.StatusRepository",
                           context, true, &GitClient::status);

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, tr("Commit..."), "Git.Commit",
                           context, true, SLOT(startCommit()),
                           QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+C") : tr("Alt+G,Alt+C")));

    createRepositoryAction(localRepositoryMenu,
                           tr("Amend Last Commit..."), "Git.AmendCommit",
                           context, true, SLOT(startAmendCommit()));

    m_fixupCommitAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Fixup Previous Commit..."), "Git.FixupCommit",
                                   context, true, SLOT(startFixupCommit()));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu,
                           tr("Reset..."), "Git.Reset",
                           context, true, SLOT(resetRepository()));

    m_interactiveRebaseAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Interactive Rebase..."), "Git.InteractiveRebase",
                                   context, true, SLOT(startRebase()));

    m_submoduleUpdateAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Update Submodules"), "Git.SubmoduleUpdate",
                                   context, true, SLOT(updateSubmodules()));
    m_abortMergeAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Abort Merge"), "Git.MergeAbort",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_abortRebaseAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Abort Rebase"), "Git.RebaseAbort",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_abortCherryPickAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Abort Cherry Pick"), "Git.CherryPickAbort",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_abortRevertAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Abort Revert"), "Git.RevertAbort",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_continueRebaseAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Continue Rebase"), "Git.RebaseContinue",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_continueCherryPickAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Continue Cherry Pick"), "Git.CherryPickContinue",
                                   context, true, SLOT(continueOrAbortCommand()));

    m_continueRevertAction =
            createRepositoryAction(localRepositoryMenu,
                                   tr("Continue Revert"), "Git.RevertContinue",
                                   context, true, SLOT(continueOrAbortCommand()));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu,
                           tr("Branches..."), "Git.BranchList",
                           context, true, SLOT(branchList()));

    // --------------
    localRepositoryMenu->addSeparator(context);

    // "Patch" menu
    ActionContainer *patchMenu = ActionManager::createMenu("Git.PatchMenu");
    patchMenu->menu()->setTitle(tr("&Patch"));
    localRepositoryMenu->addMenu(patchMenu);

    // Apply current file as patch is handled specially.
    m_applyCurrentFilePatchAction =
            createParameterAction(patchMenu,
                                  tr("Apply from Editor"), tr("Apply \"%1\""),
                                  "Git.ApplyCurrentFilePatch",
                                  context, true);

    connect(m_applyCurrentFilePatchAction, SIGNAL(triggered()), this,
            SLOT(applyCurrentFilePatch()));

    createRepositoryAction(patchMenu,
                           tr("Apply from File..."), "Git.ApplyPatch",
                           context, true, SLOT(promptApplyPatch()));

    // "Stash" menu
    ActionContainer *stashMenu = ActionManager::createMenu("Git.StashMenu");
    stashMenu->menu()->setTitle(tr("&Stash"));
    localRepositoryMenu->addMenu(stashMenu);

    createRepositoryAction(stashMenu,
                           tr("Stashes..."), "Git.StashList",
                           context, false, SLOT(stashList()));

    stashMenu->addSeparator(context);

    QAction *action = createRepositoryAction(stashMenu, tr("Stash"), "Git.Stash",
                                             context, true, SLOT(stash()));
    action->setToolTip(tr("Saves the current state of your work and resets the repository."));

    action = createRepositoryAction(stashMenu, tr("Stash Unstaged Files"), "Git.StashUnstaged",
                                    context, true, SLOT(stashUnstaged()));
    action->setToolTip(tr("Saves the current state of your unstaged files and resets the repository "
                          "to its staged state."));

    action = createRepositoryAction(stashMenu, tr("Take Snapshot..."), "Git.StashSnapshot",
                                    context, true, SLOT(stashSnapshot()));
    action->setToolTip(tr("Saves the current state of your work."));

    stashMenu->addSeparator(context);

    action = createRepositoryAction(stashMenu, tr("Stash Pop"), "Git.StashPop",
                                    context, true, SLOT(stashPop()));
    action->setToolTip(tr("Restores changes saved to the stash list using \"Stash\"."));


    /* \"Local Repository" menu */

    // --------------

    /*  "Remote Repository" menu */
    ActionContainer *remoteRepositoryMenu = ActionManager::createMenu("Git.RemoteRepositoryMenu");
    remoteRepositoryMenu->menu()->setTitle(tr("&Remote Repository"));
    gitContainer->addMenu(remoteRepositoryMenu);

    createRepositoryAction(remoteRepositoryMenu, tr("Fetch"), "Git.Fetch",
                           context, true, SLOT(fetch()));

    createRepositoryAction(remoteRepositoryMenu, tr("Pull"), "Git.Pull",
                           context, true, SLOT(pull()));

    createRepositoryAction(remoteRepositoryMenu, tr("Push"), "Git.Push",
                           context, true, SLOT(push()));

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    // "Subversion" menu
    ActionContainer *subversionMenu = ActionManager::createMenu("Git.Subversion");
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    remoteRepositoryMenu->addMenu(subversionMenu);

    createRepositoryAction(subversionMenu,
                           tr("Log"), "Git.Subversion.Log",
                           context, false, &GitClient::subversionLog);

    createRepositoryAction(subversionMenu,
                           tr("Fetch"), "Git.Subversion.Fetch",
                           context, false, &GitClient::synchronousSubversionFetch);

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    createRepositoryAction(remoteRepositoryMenu,
                           tr("Manage Remotes..."), "Git.RemoteList",
                           context, false, SLOT(remoteList()));

    /* \"Remote Repository" menu */

    // --------------

    /*  Actions only in locator */
    createRepositoryAction(0, tr("Show..."), "Git.Show",
                           context, true, SLOT(startChangeRelatedAction()));

    createRepositoryAction(0, tr("Revert..."), "Git.Revert",
                           context, true, SLOT(startChangeRelatedAction()));

    createRepositoryAction(0, tr("Cherry Pick..."), "Git.CherryPick",
                           context, true, SLOT(startChangeRelatedAction()));

    createRepositoryAction(0, tr("Checkout..."), "Git.Checkout",
                           context, true, SLOT(startChangeRelatedAction()));

    createRepositoryAction(0, tr("Rebase..."), "Git.Rebase",
                           context, true, SLOT(branchList()));

    createRepositoryAction(0, tr("Merge..."), "Git.Merge",
                           context, true, SLOT(branchList()));

    /*  \Actions only in locator */

    // --------------

    /*  "Git Tools" menu */
    ActionContainer *gitToolsMenu = ActionManager::createMenu("Git.GitToolsMenu");
    gitToolsMenu->menu()->setTitle(tr("Git &Tools"));
    gitContainer->addMenu(gitToolsMenu);

    createRepositoryAction(gitToolsMenu,
                           tr("Gitk"), "Git.LaunchGitK",
                           context, true, &GitClient::launchGitK);

    createFileAction(gitToolsMenu, tr("Gitk Current File"), tr("Gitk of \"%1\""),
                     "Git.GitkFile", context, true, SLOT(gitkForCurrentFile()));

    createFileAction(gitToolsMenu, tr("Gitk for folder of Current File"), tr("Gitk for folder of \"%1\""),
                     "Git.GitkFolder", context, true, SLOT(gitkForCurrentFolder()));

    // --------------
    gitToolsMenu->addSeparator(context);

    createRepositoryAction(gitToolsMenu, tr("Git Gui"), "Git.GitGui",
                           context, true, SLOT(gitGui()));

    // --------------
    gitToolsMenu->addSeparator(context);

    m_repositoryBrowserAction =
            createRepositoryAction(gitToolsMenu,
                                   tr("Repository Browser"), "Git.LaunchRepositoryBrowser",
                                   context, true, &GitClient::launchRepositoryBrowser);

    m_mergeToolAction =
            createRepositoryAction(gitToolsMenu,
                                   tr("Merge Tool"), "Git.MergeTool",
                                   context, true, SLOT(startMergeTool()));

    /* \"Git Tools" menu */

    // --------------
    gitContainer->addSeparator(context);

    createRepositoryAction(gitContainer, tr("Actions on Commits..."), "Git.ChangeActions",
                           context, false, SLOT(startChangeRelatedAction()));

    m_createRepositryAction = new QAction(tr("Create Repository..."), this);
    Command *createRepositoryCommand = ActionManager::registerAction(
                m_createRepositryAction, "Git.CreateRepository");
    connect(m_createRepositryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    gitContainer->addAction(createRepositoryCommand);

    // Submit editor
    Context submitContext(Constants::GITSUBMITEDITOR_ID);
    m_submitCurrentAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Command *command = ActionManager::registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFilesAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, submitContext);

    connect(VcsManager::instance(), SIGNAL(repositoryChanged(QString)),
            this, SLOT(updateContinueAndAbortCommands()));
    connect(VcsManager::instance(), SIGNAL(repositoryChanged(QString)),
            this, SLOT(updateBranches(QString)), Qt::QueuedConnection);

    Utils::MimeDatabase::addMimeTypes(QLatin1String(RC_GIT_MIME_XML));

    /* "Gerrit" */
    m_gerritPlugin = new Gerrit::Internal::GerritPlugin(this);
    const bool ok = m_gerritPlugin->initialize(remoteRepositoryMenu);
    m_gerritPlugin->updateActions(currentState().hasTopLevel());
    m_gerritPlugin->addToLocator(m_commandLocator);

    return ok;
}

GitVersionControl *GitPlugin::gitVersionControl() const
{
    return static_cast<GitVersionControl *>(versionControl());
}

void GitPlugin::submitEditorDiff(const QStringList &unstaged, const QStringList &staged)
{
    m_gitClient->diffFiles(m_submitRepository, unstaged, staged);
}

void GitPlugin::submitEditorMerge(const QStringList &unmerged)
{
    m_gitClient->merge(m_submitRepository, unmerged);
}

void GitPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->diffFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::diffCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    if (relativeProject.isEmpty())
        m_gitClient->diffRepository(state.currentProjectTopLevel());
    else
        m_gitClient->diffProject(state.currentProjectTopLevel(), relativeProject);
}

void GitPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->diffRepository(state.topLevel());
}

void GitPlugin::logFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->log(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void GitPlugin::blameFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const int lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    m_gitClient->blame(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile(), QString(), lineNumber);
}

void GitPlugin::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    m_gitClient->log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->log(state.topLevel());
}

void GitPlugin::reflogRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->reflog(state.topLevel());
}

void GitPlugin::undoFileChanges(bool revertStaging)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    FileChangeBlocker fcb(state.currentFile());
    m_gitClient->revert(QStringList(state.currentFile()), revertStaging);
}

void GitPlugin::undoUnstagedFileChanges()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    undoFileChanges(false);
}

class ResetItemDelegate : public LogItemDelegate
{
public:
    ResetItemDelegate(LogChangeWidget *widget) : LogItemDelegate(widget) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
    {
        if (index.row() < currentRow())
            option->font.setStrikeOut(true);
        LogItemDelegate::initStyleOption(option, index);
    }
};

class RebaseItemDelegate : public IconItemDelegate
{
public:
    RebaseItemDelegate(LogChangeWidget *widget)
        : IconItemDelegate(widget, QLatin1String(Core::Constants::ICON_UNDO))
    {
    }

protected:
    bool hasIcon(int row) const
    {
        return row <= currentRow();
    }
};

void GitPlugin::resetRepository()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();

    LogChangeDialog dialog(true, ICore::mainWindow());
    ResetItemDelegate delegate(dialog.widget());
    dialog.setWindowTitle(tr("Undo Changes to %1").arg(QDir::toNativeSeparators(topLevel)));
    if (dialog.runDialog(topLevel, QString(), LogChangeWidget::IncludeRemotes))
        m_gitClient->reset(topLevel, dialog.resetFlag(), dialog.commit());
}

void GitPlugin::startRebase()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString topLevel = state.topLevel();
    if (topLevel.isEmpty() || !m_gitClient->canRebase(topLevel))
        return;
    LogChangeDialog dialog(false, ICore::mainWindow());
    RebaseItemDelegate delegate(dialog.widget());
    dialog.setWindowTitle(tr("Interactive Rebase"));
    if (!dialog.runDialog(topLevel))
        return;
    if (m_gitClient->beginStashScope(topLevel, QLatin1String("Rebase-i")))
        m_gitClient->interactiveRebase(topLevel, dialog.commit(), false);
}

void GitPlugin::startChangeRelatedAction()
{
    const VcsBasePluginState state = currentState();
    if (!state.hasTopLevel())
        return;

    QAction *action = qobject_cast<QAction *>(sender());
    Id id = action ? Id::fromUniqueIdentifier(action->data().toInt()) : Id();
    ChangeSelectionDialog dialog(state.topLevel(), id, ICore::mainWindow());

    int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    const QString workingDirectory = dialog.workingDirectory();
    const QString change = dialog.change();

    if (workingDirectory.isEmpty() || change.isEmpty())
        return;

    if (dialog.command() == Show) {
        m_gitClient->show(workingDirectory, change);
        return;
    }

    if (!DocumentManager::saveAllModifiedDocuments())
        return;

    switch (dialog.command()) {
    case CherryPick:
        m_gitClient->synchronousCherryPick(workingDirectory, change);
        break;
    case Revert:
        m_gitClient->synchronousRevert(workingDirectory, change);
        break;
    case Checkout:
        m_gitClient->stashAndCheckout(workingDirectory, change);
        break;
    default:
        return;
    }
}

void GitPlugin::stageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::unstageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->synchronousReset(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void GitPlugin::gitkForCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient->launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::gitkForCurrentFolder()
{
    const VcsBasePluginState state = currentState();
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
    if (QFileInfo(dir,QLatin1String(".git")).exists() || dir.cd(QLatin1String(".git"))) {
        m_gitClient->launchGitK(state.currentFileDirectory());
    } else {
        QString folderName = dir.absolutePath();
        dir.cdUp();
        folderName = folderName.remove(0, dir.absolutePath().length() + 1);
        m_gitClient->launchGitK(dir.absolutePath(), folderName);
    }
}

void GitPlugin::gitGui()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->launchGitGui(state.topLevel());
}

void GitPlugin::startAmendCommit()
{
    startCommit(AmendCommit);
}

void GitPlugin::startFixupCommit()
{
    startCommit(FixupCommit);
}

void GitPlugin::startCommit()
{
    startCommit(SimpleCommit);
}

void GitPlugin::startCommit(CommitType commitType)
{
    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another submit is currently being executed."));
        return;
    }

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QString errorMessage, commitTemplate;
    CommitData data(commitType);
    if (!m_gitClient->getCommitData(state.topLevel(), &commitTemplate, data, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return;
    }

    // Store repository for diff and the original list of
    // files to be able to unstage files the user unchecks
    m_submitRepository = data.panelInfo.repository;

    // Start new temp file with message template
    TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    saver.write(commitTemplate.toLocal8Bit());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    openSubmitEditor(m_commitMessageFileName, data);
}

void GitPlugin::updateVersionWarning()
{
    unsigned version = m_gitClient->gitVersion();
    if (!version || version >= minimumRequiredVersion)
        return;
    IDocument *curDocument = EditorManager::currentDocument();
    if (!curDocument)
        return;
    InfoBar *infoBar = curDocument->infoBar();
    Id gitVersionWarning("GitVersionWarning");
    if (!infoBar->canInfoBeAdded(gitVersionWarning))
        return;
    infoBar->addInfo(InfoBarEntry(gitVersionWarning,
                        tr("Unsupported version of Git found. Git %1 or later required.")
                        .arg(versionString(minimumRequiredVersion)),
                        InfoBarEntry::GlobalSuppressionEnabled));
}

IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd)
{
    IEditor *editor = EditorManager::openEditor(fileName, Constants::GITSUBMITEDITOR_ID);
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    setSubmitEditor(submitEditor);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentAction, m_diffSelectedFilesAction);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    QString title;
    switch (cd.commitType) {
    case AmendCommit:
        title = tr("Amend %1").arg(cd.amendSHA1);
        break;
    case FixupCommit:
        title = tr("Git Fixup Commit");
        break;
    default:
        title = tr("Git Commit");
    }
    IDocument *document = submitEditor->document();
    document->setPreferredDisplayName(title);
    VcsBasePlugin::setSource(document, m_submitRepository);
    connect(submitEditor, SIGNAL(diff(QStringList,QStringList)), this, SLOT(submitEditorDiff(QStringList,QStringList)));
    connect(submitEditor, SIGNAL(merge(QStringList)), this, SLOT(submitEditorMerge(QStringList)));
    connect(submitEditor, SIGNAL(show(QString,QString)), m_gitClient, SLOT(show(QString,QString)));
    return editor;
}

void GitPlugin::submitCurrentLog()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

bool GitPlugin::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;
    GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_commitMessageFileName);
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool promptData = false;
    const VcsBaseSubmitEditor::PromptSubmitResult answer
            = editor->promptSubmit(tr("Closing Git Editor"),
                 tr("Do you want to commit the change?"),
                 tr("Git will not accept this commit. Do you want to continue to edit it?"),
                 &promptData, !m_submitActionTriggered, false);
    m_submitActionTriggered = false;
    switch (answer) {
    case VcsBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VcsBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }


    // Go ahead!
    SubmitFileModel *model = qobject_cast<SubmitFileModel *>(editor->fileModel());
    bool closeEditor = true;
    CommitType commitType = editor->commitType();
    QString amendSHA1 = editor->amendSHA1();
    if (model->hasCheckedFiles() || !amendSHA1.isEmpty()) {
        // get message & commit
        if (!DocumentManager::saveDocument(editorDocument))
            return false;

        closeEditor = m_gitClient->addAndCommit(m_submitRepository, editor->panelData(),
                                                commitType, amendSHA1,
                                                m_commitMessageFileName, model);
    }
    if (!closeEditor)
        return false;
    cleanCommitMessageFile();
    if (commitType == FixupCommit) {
        if (!m_gitClient->beginStashScope(m_submitRepository, QLatin1String("Rebase-fixup"),
                                          NoPrompt, editor->panelData().pushAction)) {
            return false;
        }
        m_gitClient->interactiveRebase(m_submitRepository, amendSHA1, true);
    } else {
        m_gitClient->continueCommandIfNeeded(m_submitRepository);
        if (editor->panelData().pushAction == NormalPush)
            m_gitClient->push(m_submitRepository);
        else if (editor->panelData().pushAction == PushToGerrit)
            connect(editor, SIGNAL(destroyed()), this, SLOT(delayedPushToGerrit()));
    }

    return true;
}

void GitPlugin::fetch()
{
    m_gitClient->fetch(currentState().topLevel(), QString());
}

void GitPlugin::pull()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();
    bool rebase = m_settings.boolValue(GitSettings::pullRebaseKey);

    if (!rebase) {
        QString currentBranch = m_gitClient->synchronousCurrentLocalBranch(topLevel);
        if (!currentBranch.isEmpty()) {
            currentBranch.prepend(QLatin1String("branch."));
            currentBranch.append(QLatin1String(".rebase"));
            rebase = (m_gitClient->readConfigValue(topLevel, currentBranch) == QLatin1String("true"));
        }
    }

    if (!m_gitClient->beginStashScope(topLevel, QLatin1String("Pull"), rebase ? Default : AllowUnstashed))
        return;
    m_gitClient->synchronousPull(topLevel, rebase);
}

void GitPlugin::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->push(state.topLevel());
}

void GitPlugin::startMergeTool()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->merge(state.topLevel());
}

void GitPlugin::continueOrAbortCommand()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QObject *action = QObject::sender();

    if (action == m_abortMergeAction)
        m_gitClient->synchronousMerge(state.topLevel(), QLatin1String("--abort"));
    else if (action == m_abortRebaseAction)
        m_gitClient->rebase(state.topLevel(), QLatin1String("--abort"));
    else if (action == m_abortCherryPickAction)
        m_gitClient->synchronousCherryPick(state.topLevel(), QLatin1String("--abort"));
    else if (action == m_abortRevertAction)
        m_gitClient->synchronousRevert(state.topLevel(), QLatin1String("--abort"));
    else if (action == m_continueRebaseAction)
        m_gitClient->rebase(state.topLevel(), QLatin1String("--continue"));
    else if (action == m_continueCherryPickAction)
        m_gitClient->cherryPick(state.topLevel(), QLatin1String("--continue"));
    else if (action == m_continueRevertAction)
        m_gitClient->revert(state.topLevel(), QLatin1String("--continue"));

    updateContinueAndAbortCommands();
}

void GitPlugin::cleanProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    cleanRepository(state.currentProjectPath());
}

void GitPlugin::cleanRepository()
{
    const VcsBasePluginState state = currentState();
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

    if (!gotFiles) {
        Core::AsynchronousMessageBox::warning(tr("Unable to retrieve file list"), errorMessage);
        return;
    }
    if (files.isEmpty() && ignoredFiles.isEmpty()) {
        Core::AsynchronousMessageBox::information(tr("Repository Clean"),
                                                   tr("The repository is clean."));
        return;
    }

    // Show in dialog
    CleanDialog dialog(ICore::dialogParent());
    dialog.setFileList(directory, files, ignoredFiles);
    dialog.exec();
}

void GitPlugin::updateSubmodules()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient->updateSubmodulesIfNeeded(state.topLevel(), false);
}

// If the file is modified in an editor, make sure it is saved.
static bool ensureFileSaved(const QString &fileName)
{
    return DocumentManager::saveModifiedDocument(DocumentModel::documentForFilePath(fileName));
}

void GitPlugin::applyCurrentFilePatch()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasPatchFile() && state.hasTopLevel(), return);
    const QString patchFile = state.currentPatchFile();
    if (!ensureFileSaved(patchFile))
        return;
    applyPatch(state.topLevel(), patchFile);
}

void GitPlugin::promptApplyPatch()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    applyPatch(state.topLevel(), QString());
}

void GitPlugin::applyPatch(const QString &workingDirectory, QString file)
{
    // Ensure user has been notified about pending changes
    if (!m_gitClient->beginStashScope(workingDirectory, QLatin1String("Apply-Patch"), AllowUnstashed))
        return;
    // Prompt for file
    if (file.isEmpty()) {
        const QString filter = tr("Patches (*.patch *.diff)");
        file = QFileDialog::getOpenFileName(ICore::mainWindow(), tr("Choose Patch"), QString(), filter);
        if (file.isEmpty()) {
            m_gitClient->endStashScope(workingDirectory);
            return;
        }
    }
    // Run!
    QString errorMessage;
    if (m_gitClient->synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
        if (errorMessage.isEmpty())
            VcsOutputWindow::appendMessage(tr("Patch %1 successfully applied to %2").arg(file, workingDirectory));
        else
            VcsOutputWindow::appendError(errorMessage);
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
    m_gitClient->endStashScope(workingDirectory);
}

void GitPlugin::stash(bool unstagedOnly)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    // Simple stash without prompt, reset repo.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    const QString topLevel = state.topLevel();
    m_gitClient->executeSynchronousStash(topLevel, QString(), unstagedOnly);
    if (m_stashDialog)
        m_stashDialog->refresh(topLevel, true);
}

void GitPlugin::stashUnstaged()
{
    stash(true);
}

void GitPlugin::stashSnapshot()
{
    // Prompt for description, restore immediately and keep on working.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString id = m_gitClient->synchronousStash(state.topLevel(), QString(),
                GitClient::StashImmediateRestore | GitClient::StashPromptDescription);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

void GitPlugin::stashPop()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const QString repository = currentState().topLevel();
    m_gitClient->stashPop(repository);
    if (m_stashDialog)
        m_stashDialog->refresh(repository, true);
}

// Create a non-modal dialog with refresh function or raise if it exists
template <class NonModalDialog>
    inline void showNonModalDialog(const QString &topLevel,
                                   QPointer<NonModalDialog> &dialog)
{
    if (dialog) {
        dialog->show();
        dialog->raise();
    } else {
        dialog = new NonModalDialog(ICore::mainWindow());
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

void GitPlugin::updateActions(VcsBasePlugin::ActionState as)
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    if (m_stashDialog)
        m_stashDialog->refresh(currentState().topLevel(), false);
    if (m_branchDialog)
        m_branchDialog->refresh(currentState().topLevel(), false);
    if (m_remoteDialog)
        m_remoteDialog->refresh(currentState().topLevel(), false);

    m_commandLocator->setEnabled(repositoryEnabled);
    m_createRepositryAction->setEnabled(true);
    if (!enableMenuAction(as, m_menuAction))
        return;
    if (repositoryEnabled)
        updateVersionWarning();
    // Note: This menu is visible if there is no repository. Only
    // 'Create Repository'/'Show' actions should be available.
    const QString fileName = currentState().currentFileName();
    foreach (ParameterAction *fileAction, m_fileActions)
        fileAction->setParameter(fileName);
    // If the current file looks like a patch, offer to apply
    m_applyCurrentFilePatchAction->setParameter(currentState().currentPatchFileDisplayName());
    const QString projectName = currentState().currentProjectName();
    foreach (ParameterAction *projectAction, m_projectActions)
        projectAction->setParameter(projectName);

    foreach (QAction *repositoryAction, m_repositoryActions)
        repositoryAction->setEnabled(repositoryEnabled);

    m_submoduleUpdateAction->setVisible(repositoryEnabled
            && !m_gitClient->submoduleList(currentState().topLevel()).isEmpty());

    updateContinueAndAbortCommands();
    updateRepositoryBrowserAction();

    m_gerritPlugin->updateActions(repositoryEnabled);
}

void GitPlugin::updateContinueAndAbortCommands()
{
    if (currentState().hasTopLevel()) {
        GitClient::CommandInProgress gitCommandInProgress =
                m_gitClient->checkCommandInProgress(currentState().topLevel());

        m_mergeToolAction->setVisible(gitCommandInProgress != GitClient::NoCommand);
        m_abortMergeAction->setVisible(gitCommandInProgress == GitClient::Merge);
        m_abortCherryPickAction->setVisible(gitCommandInProgress == GitClient::CherryPick);
        m_abortRevertAction->setVisible(gitCommandInProgress == GitClient::Revert);
        m_abortRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                        || gitCommandInProgress == GitClient::RebaseMerge);
        m_continueCherryPickAction->setVisible(gitCommandInProgress == GitClient::CherryPick);
        m_continueRevertAction->setVisible(gitCommandInProgress == GitClient::Revert);
        m_continueRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                           || gitCommandInProgress == GitClient::RebaseMerge);
        m_fixupCommitAction->setEnabled(gitCommandInProgress == GitClient::NoCommand);
        m_interactiveRebaseAction->setEnabled(gitCommandInProgress == GitClient::NoCommand);
    } else {
        m_mergeToolAction->setVisible(false);
        m_abortMergeAction->setVisible(false);
        m_abortCherryPickAction->setVisible(false);
        m_abortRevertAction->setVisible(false);
        m_abortRebaseAction->setVisible(false);
        m_continueCherryPickAction->setVisible(false);
        m_continueRevertAction->setVisible(false);
        m_continueRebaseAction->setVisible(false);
    }
}

void GitPlugin::delayedPushToGerrit()
{
    m_gerritPlugin->push(m_submitRepository);
}

void GitPlugin::updateBranches(const QString &repository)
{
    if (m_branchDialog && m_branchDialog->isVisible())
        m_branchDialog->refreshIfSame(repository);
}

void GitPlugin::updateRepositoryBrowserAction()
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    const bool hasRepositoryBrowserCmd = !m_settings.stringValue(GitSettings::repositoryBrowserCmd).isEmpty();
    m_repositoryBrowserAction->setEnabled(repositoryEnabled && hasRepositoryBrowserCmd);
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

Gerrit::Internal::GerritPlugin *GitPlugin::gerritPlugin() const
{
    return m_gerritPlugin;
}

#ifdef WITH_TESTS

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
    QTest::newRow("??") << FileStates(UntrackedFile) << FileStates(UnknownFileState);

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
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[3].id);
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

    VcsBaseEditorWidget::testLogResolving(editorParameters[1].id, data,
                            "50a6b54c - Merge branch 'for-junio' of git://bogomips.org/git-svn",
                            "3587b513 - Update draft release notes to 1.8.2");
}

void GitPlugin::testCloneWizard_directoryFromRepository()
{
    CloneWizardPage page;
    page.testDirectoryFromRepository();
}

void GitPlugin::testCloneWizard_directoryFromRepository_data()
{
    QTest::addColumn<QString>("repository");
    QTest::addColumn<QString>("localDirectory");

    QTest::newRow("http") << "http://host/qt/qt.git" << "qt";
    QTest::newRow("without slash") << "user@host:qt.git" << "qt";
    QTest::newRow("mainline.git") << "git://gitorious.org/gitorious/mainline.git" << "gitorious";
    QTest::newRow("local repo (Unix)") << "/home/user/qt-creator.git" << "qt-creator";
    QTest::newRow("local repo (Windows)") << "c:\\repos\\qt-creator.git" << "qt-creator";
    QTest::newRow("ssh with port") << "ssh://host:29418/qt/qt.git" << "qt";
    QTest::newRow("invalid chars removed") << "ssh://host/in%va$lid.git" << "in-va-lid";
    QTest::newRow("leading dashs removed") << "https://gerrit.local/--leadingDash" << "leadingDash";
}
#endif

} // namespace Internal
} // namespace Git
