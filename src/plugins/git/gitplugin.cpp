/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gitplugin.h"

#include "changeselectiondialog.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"
#include "branchdialog.h"
#include "clonewizard.h"
#include "gitoriousclonewizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>
#include <utils/parameteraction.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QtPlugin>

#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_ID,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_COMMAND_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_commandlog",
    "gitlog"},
{   VCSBase::LogOutput,
    Git::Constants::GIT_LOG_EDITOR_ID,
    Git::Constants::GIT_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_filelog",
    "gitfilelog"},
{   VCSBase::AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_ID,
    Git::Constants::GIT_BLAME_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_BLAME_EDITOR,
    "application/vnd.nokia.text.scs_git_annotation",
    "gitsannotate"},
{   VCSBase::DiffOutput,
    Git::Constants::GIT_DIFF_EDITOR_ID,
    Git::Constants::GIT_DIFF_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_DIFF_EDITOR,
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

using namespace Git;
using namespace Git::Internal;

// GitPlugin

GitPlugin *GitPlugin::m_instance = 0;

GitPlugin::GitPlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(Git::Constants::GITSUBMITEDITOR_ID)),
    m_core(0),
    m_diffAction(0),
    m_diffProjectAction(0),
    m_diffRepositoryAction(0),
    m_statusRepositoryAction(0),
    m_logAction(0),
    m_blameAction(0),
    m_logProjectAction(0),
    m_undoFileAction(0),
    m_logRepositoryAction(0),
    m_undoRepositoryAction(0),
    m_createRepositoryAction(0),
    m_showAction(0),
    m_stageAction(0),
    m_unstageAction(0),
    m_commitAction(0),
    m_pullAction(0),
    m_pushAction(0),
    m_submitCurrentAction(0),
    m_diffSelectedFilesAction(0),
    m_undoAction(0),
    m_redoAction(0),
    m_stashAction(0),
    m_stashPopAction(0),
    m_stashListAction(0),
    m_branchListAction(0),
    m_menuAction(0),
    m_gitClient(0),
    m_changeSelectionDialog(0),
    m_submitActionTriggered(false)
{
    m_instance = this;
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

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    Git::Constants::C_GITSUBMITEDITOR
};

static Core::Command *createSeparator(Core::ActionManager *am,
                                       const QList<int> &context,
                                       const QString &id,
                                       QObject *parent)
{
    QAction *a = new QAction(parent);
    a->setSeparator(true);
    return am->registerAction(a, id, context);
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    typedef VCSBase::VCSEditorFactory<GitEditor> GitEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<GitSubmitEditor> GitSubmitEditorFactory;

    VCSBase::VCSBasePlugin::initialize(new GitVersionControl(m_gitClient));

    m_core = Core::ICore::instance();
    m_gitClient = new GitClient(this);
    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << m_core->uniqueIDManager()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the settings Page
    addAutoReleasedObject(new SettingsPage());

    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new GitEditorFactory(editorParameters + i, m_gitClient, describeSlot));

    addAutoReleasedObject(new GitSubmitEditorFactory(&submitParameters));
    addAutoReleasedObject(new CloneWizard);
    addAutoReleasedObject(new Gitorious::Internal::GitoriousCloneWizard);

    //register actions
    Core::ActionManager *actionManager = m_core->actionManager();

    Core::ActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *gitContainer =
        actionManager->createMenu(QLatin1String("Git"));
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();
    Core::Command *command;

    m_diffAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_diffAction, "Git.Diff", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+D")));
    connect(m_diffAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    gitContainer->addAction(command);

    m_logAction = new Utils::ParameterAction(tr("Log File"), tr("Log of \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_logAction, "Git.Log", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+L")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logAction, SIGNAL(triggered()), this, SLOT(logFile()));
    gitContainer->addAction(command);

    m_blameAction = new Utils::ParameterAction(tr("Blame"), tr("Blame for \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_blameAction, "Git.Blame", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+B")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_blameAction, SIGNAL(triggered()), this, SLOT(blameFile()));
    gitContainer->addAction(command);

    m_undoFileAction = new Utils::ParameterAction(tr("Undo Changes"), tr("Undo Changes for \"%1\""),  Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_undoFileAction, "Git.Undo", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+U")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_undoFileAction, SIGNAL(triggered()), this, SLOT(undoFileChanges()));
    gitContainer->addAction(command);

    m_stageAction = new Utils::ParameterAction(tr("Stage File for Commit"), tr("Stage \"%1\" for Commit"), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_stageAction, "Git.Stage", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+A")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_stageAction, SIGNAL(triggered()), this, SLOT(stageFile()));
    gitContainer->addAction(command);

    m_unstageAction = new Utils::ParameterAction(tr("Unstage File from Commit"), tr("Unstage \"%1\" from Commit"), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_unstageAction, "Git.Unstage", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_unstageAction, SIGNAL(triggered()), this, SLOT(unstageFile()));
    gitContainer->addAction(command);

    gitContainer->addAction(createSeparator(actionManager, globalcontext, QLatin1String("Git.Sep.Project"), this));

    m_diffProjectAction = new Utils::ParameterAction(tr("Diff Current Project"), tr("Diff Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_diffProjectAction, "Git.DiffProject", globalcontext);
    command->setDefaultKeySequence(QKeySequence("Alt+G,Alt+Shift+D"));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffCurrentProject()));
    gitContainer->addAction(command);

    m_logProjectAction = new Utils::ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(m_logProjectAction, "Git.LogProject", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+K")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    gitContainer->addAction(command);

    gitContainer->addAction(createSeparator(actionManager, globalcontext, QLatin1String("Git.Sep.Repository"), this));

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = actionManager->registerAction(m_diffRepositoryAction, "Git.DiffRepository", globalcontext);
    connect(m_diffRepositoryAction, SIGNAL(triggered()), this, SLOT(diffRepository()));
    gitContainer->addAction(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = actionManager->registerAction(m_statusRepositoryAction, "Git.StatusRepository", globalcontext);
    connect(m_statusRepositoryAction, SIGNAL(triggered()), this, SLOT(statusRepository()));
    gitContainer->addAction(command);

    m_logRepositoryAction = new QAction(tr("Log Repository"), this);
    command = actionManager->registerAction(m_logRepositoryAction, "Git.LogRepository", globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    gitContainer->addAction(command);

    m_undoRepositoryAction = new QAction(tr("Undo Repository Changes"), this);
    command = actionManager->registerAction(m_undoRepositoryAction, "Git.UndoRepository", globalcontext);
    connect(m_undoRepositoryAction, SIGNAL(triggered()), this, SLOT(undoRepositoryChanges()));
    gitContainer->addAction(command);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = actionManager->registerAction(m_createRepositoryAction, "Git.CreateRepository", globalcontext);
    connect(m_createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    gitContainer->addAction(command);

    gitContainer->addAction(createSeparator(actionManager, globalcontext, QLatin1String("Git.Sep.Global"), this));

    m_stashAction = new QAction(tr("Stash"), this);
    m_stashAction->setToolTip(tr("Saves the current state of your work."));
    command = actionManager->registerAction(m_stashAction, "Git.Stash", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_stashAction, SIGNAL(triggered()), this, SLOT(stash()));
    gitContainer->addAction(command);

    m_pullAction = new QAction(tr("Pull"), this);
    command = actionManager->registerAction(m_pullAction, "Git.Pull", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_pullAction, SIGNAL(triggered()), this, SLOT(pull()));
    gitContainer->addAction(command);

    m_stashPopAction = new QAction(tr("Stash Pop"), this);
    m_stashAction->setToolTip(tr("Restores changes saved to the stash list using \"Stash\"."));
    command = actionManager->registerAction(m_stashPopAction, "Git.StashPop", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_stashPopAction, SIGNAL(triggered()), this, SLOT(stashPop()));
    gitContainer->addAction(command);

    m_commitAction = new QAction(tr("Commit..."), this);
    command = actionManager->registerAction(m_commitAction, "Git.Commit", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+C")));
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_commitAction, SIGNAL(triggered()), this, SLOT(startCommit()));
    gitContainer->addAction(command);

    m_pushAction = new QAction(tr("Push"), this);
    command = actionManager->registerAction(m_pushAction, "Git.Push", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_pushAction, SIGNAL(triggered()), this, SLOT(push()));
    gitContainer->addAction(command);

    gitContainer->addAction(createSeparator(actionManager, globalcontext, QLatin1String("Git.Sep.Branch"), this));

    m_branchListAction = new QAction(tr("Branches..."), this);
    command = actionManager->registerAction(m_branchListAction, "Git.BranchList", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_branchListAction, SIGNAL(triggered()), this, SLOT(branchList()));
    gitContainer->addAction(command);

    m_stashListAction = new QAction(tr("List Stashes"), this);
    command = actionManager->registerAction(m_stashListAction, "Git.StashList", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_stashListAction, SIGNAL(triggered()), this, SLOT(stashList()));
    gitContainer->addAction(command);

    m_showAction = new QAction(tr("Show Commit..."), this);
    command = actionManager->registerAction(m_showAction, "Git.ShowCommit", globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_showAction, SIGNAL(triggered()), this, SLOT(showCommit()));
    gitContainer->addAction(command);

    // Submit editor
    QList<int> submitContext;
    submitContext.push_back(m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String(Constants::C_GITSUBMITEDITOR)));
    m_submitCurrentAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = actionManager->registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    connect(m_submitCurrentAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFilesAction = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(m_redoAction, Core::Constants::REDO, submitContext);

    return true;
}

void GitPlugin::extensionsInitialized()
{
}

GitVersionControl *GitPlugin::gitVersionControl() const
{
    return static_cast<GitVersionControl *>(versionControl());
}

void GitPlugin::submitEditorDiff(const QStringList &unstaged, const QStringList &staged)
{
    m_gitClient->diff(m_submitRepository, QStringList(), unstaged, staged);
}

void GitPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->diff(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile());
}

void GitPlugin::diffCurrentProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    m_gitClient->diff(state.currentProjectTopLevel(), QStringList(), state.relativeCurrentProject());
}

void GitPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->diff(state.topLevel(), QStringList(), QStringList());
}

void GitPlugin::statusRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->status(state.topLevel());
}

void GitPlugin::logFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void GitPlugin::blameFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    const int lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    m_gitClient->blame(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), lineNumber);
}

void GitPlugin::logProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    m_gitClient->log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPlugin::undoFileChanges()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    Core::FileChangeBlocker fcb(state.currentFile());
    fcb.setModifiedReload(true);
    m_gitClient->revert(QStringList(state.currentFile()));
}

void GitPlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->graphLog(state.topLevel());
}

void GitPlugin::undoRepositoryChanges()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    const QString msg = tr("Would you like to revert all pending changes to the repository\n%1?").arg(state.topLevel());
    const QMessageBox::StandardButton answer
            = QMessageBox::question(m_core->mainWindow(),
                                    tr("Revert"), msg,
                                    QMessageBox::Yes|QMessageBox::No,
                                    QMessageBox::No);
    if (answer == QMessageBox::No)
        return;
    m_gitClient->hardReset(state.topLevel(), QString());
}

void GitPlugin::stageFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::unstageFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->synchronousReset(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void GitPlugin::startCommit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another submit is currently being executed."));
        return;
    }

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    QString errorMessage, commitTemplate;
    CommitData data;
    if (!m_gitClient->getCommitData(state.topLevel(), &commitTemplate, &data, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->append(errorMessage);
        return;
    }

    // Store repository for diff and the original list of
    // files to be able to unstage files the user unchecks
    m_submitRepository = data.panelInfo.repository;
    m_submitOrigCommitFiles = data.stagedFileNames();
    m_submitOrigDeleteFiles = data.stagedFileNames("deleted");

    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << data << commitTemplate;

    // Start new temp file with message template
    QTemporaryFile changeTmpFile;
    changeTmpFile.setAutoRemove(false);
    if (!changeTmpFile.open()) {
        VCSBase::VCSBaseOutputWindow::instance()->append(tr("Cannot create temporary file: %1").arg(changeTmpFile.errorString()));
        return;
    }
    m_commitMessageFileName = changeTmpFile.fileName();
    changeTmpFile.write(commitTemplate.toLocal8Bit());
    changeTmpFile.flush();
    // Keep the file alive, else it removes self and forgets
    // its name
    changeTmpFile.close();
    openSubmitEditor(m_commitMessageFileName, data);
}

Core::IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd)
{
    Core::IEditor *editor = m_core->editorManager()->openEditor(fileName, QLatin1String(Constants::GITSUBMITEDITOR_ID));
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileName << editor;
    m_core->editorManager()->ensureEditorManagerVisible();
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentAction, m_diffSelectedFilesAction);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    connect(submitEditor, SIGNAL(diff(QStringList,QStringList)), this, SLOT(submitEditorDiff(QStringList,QStringList)));
    return editor;
}

void GitPlugin::submitCurrentLog()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QList<Core::IEditor*> editors;
    editors.push_back(m_core->editorManager()->currentEditor());
    m_core->editorManager()->closeEditors(editors);
}

bool GitPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return false;
    Core::IFile *fileIFace = submitEditor->file();
    const GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(submitEditor);
    if (!fileIFace || !editor)
        return true;
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(fileIFace->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    GitSettings settings = m_gitClient->settings();
    const bool wantedPrompt = settings.promptToSubmit;
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing git editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 &settings.promptToSubmit, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    if (wantedPrompt != settings.promptToSubmit)
        m_gitClient->setSettings(settings);
    // Go ahead!
    const QStringList fileList = editor->checkedFiles();
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileList;
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        m_core->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        m_core->fileManager()->unblockFileChange(fileIFace);

        closeEditor = m_gitClient->addAndCommit(m_submitRepository,
                                                editor->panelData(),
                                                m_commitMessageFileName,
                                                fileList,
                                                m_submitOrigCommitFiles,
                                                m_submitOrigDeleteFiles);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void GitPlugin::pull()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    switch (m_gitClient->ensureStash(state.topLevel())) {
        case GitClient::StashUnchanged:
        case GitClient::Stashed:
        case GitClient::NotStashed:
            m_gitClient->pull(state.topLevel());
        default:
        break;
    }
}

void GitPlugin::push()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->push(state.topLevel());
}

void GitPlugin::stash()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->stash(state.topLevel());
}

void GitPlugin::stashPop()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->stashPop(state.topLevel());
}

void GitPlugin::branchList()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    QString errorMessage;
    BranchDialog dialog(m_core->mainWindow());

    if (!dialog.init(m_gitClient, state.topLevel(), &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return;
    }
    dialog.exec();
}

void GitPlugin::stashList()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->stashList(state.topLevel());
}

void GitPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction))
        return;
    // Note: This menu is visible if there is no repository. Only
    // 'Create Repository'/'Show' actions should be available.
    const QString fileName = currentState().currentFileName();
    m_diffAction->setParameter(fileName);
    m_logAction->setParameter(fileName);
    m_blameAction->setParameter(fileName);
    m_undoFileAction->setParameter(fileName);
    m_stageAction->setParameter(fileName);
    m_unstageAction->setParameter(fileName);

    bool fileEnabled = !fileName.isEmpty();
    m_diffAction->setEnabled(fileEnabled);
    m_logAction->setEnabled(fileEnabled);
    m_blameAction->setEnabled(fileEnabled);
    m_undoFileAction->setEnabled(fileEnabled);
    m_stageAction->setEnabled(fileEnabled);
    m_unstageAction->setEnabled(fileEnabled);

    const QString projectName = currentState().currentProjectName();
    const bool projectEnabled = !projectName.isEmpty();
    m_diffProjectAction->setEnabled(projectEnabled);
    m_diffProjectAction->setParameter(projectName);
    m_logProjectAction->setEnabled(projectEnabled);
    m_logProjectAction->setParameter(projectName);

    const bool repositoryEnabled = currentState().hasTopLevel();
    m_diffRepositoryAction->setEnabled(repositoryEnabled);
    m_statusRepositoryAction->setEnabled(repositoryEnabled);
    m_branchListAction->setEnabled(repositoryEnabled);
    m_stashListAction->setEnabled(repositoryEnabled);
    m_stashAction->setEnabled(repositoryEnabled);
    m_pullAction->setEnabled(repositoryEnabled);
    m_commitAction->setEnabled(repositoryEnabled);
    m_stashPopAction->setEnabled(repositoryEnabled);
    m_logRepositoryAction->setEnabled(repositoryEnabled);
    m_undoRepositoryAction->setEnabled(repositoryEnabled);
    m_pushAction->setEnabled(repositoryEnabled);

    // Prompts for repo.
    m_showAction->setEnabled(true);
}

void GitPlugin::showCommit()
{
    const VCSBase::VCSBasePluginState state = currentState();

    if (!m_changeSelectionDialog)
        m_changeSelectionDialog = new ChangeSelectionDialog();

    if (state.hasTopLevel())
        m_changeSelectionDialog->setRepository(state.topLevel());

    if (m_changeSelectionDialog->exec() != QDialog::Accepted)
        return;
    const QString change = m_changeSelectionDialog->change();
    if (change.isEmpty())
        return;

    m_gitClient->show(m_changeSelectionDialog->repository(), change);
}

GitSettings GitPlugin::settings() const
{
    return m_gitClient->settings();
}

void GitPlugin::setSettings(const GitSettings &s)
{
    m_gitClient->setSettings(s);
}

GitClient *GitPlugin::gitClient() const
{
    return m_gitClient;
}

Q_EXPORT_PLUGIN(GitPlugin)
