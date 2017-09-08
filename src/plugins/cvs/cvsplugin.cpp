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

#include "cvsplugin.h"
#include "settingspage.h"
#include "cvseditor.h"
#include "cvssubmiteditor.h"
#include "cvsclient.h"
#include "cvscontrol.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <texteditor/textdocument.h>

#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/vcsmanager.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QtPlugin>
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace VcsBase;
using namespace Utils;

namespace Cvs {
namespace Internal {

static inline QString msgCannotFindTopLevel(const QString &f)
{
    return CvsPlugin::tr("Cannot find repository for \"%1\".").
            arg(QDir::toNativeSeparators(f));
}

static inline QString msgLogParsingFailed()
{
    return CvsPlugin::tr("Parsing of the log output failed.");
}

const char CVS_CONTEXT[]               = "CVS Context";
const char CMD_ID_CVS_MENU[]           = "CVS.Menu";
const char CMD_ID_ADD[]                = "CVS.Add";
const char CMD_ID_DELETE_FILE[]        = "CVS.Delete";
const char CMD_ID_EDIT_FILE[]          = "CVS.EditFile";
const char CMD_ID_UNEDIT_FILE[]        = "CVS.UneditFile";
const char CMD_ID_UNEDIT_REPOSITORY[]  = "CVS.UneditRepository";
const char CMD_ID_REVERT[]             = "CVS.Revert";
const char CMD_ID_DIFF_PROJECT[]       = "CVS.DiffAll";
const char CMD_ID_DIFF_CURRENT[]       = "CVS.DiffCurrent";
const char CMD_ID_COMMIT_ALL[]         = "CVS.CommitAll";
const char CMD_ID_REVERT_ALL[]         = "CVS.RevertAll";
const char CMD_ID_COMMIT_CURRENT[]     = "CVS.CommitCurrent";
const char CMD_ID_FILELOG_CURRENT[]    = "CVS.FilelogCurrent";
const char CMD_ID_ANNOTATE_CURRENT[]   = "CVS.AnnotateCurrent";
const char CMD_ID_STATUS[]             = "CVS.Status";
const char CMD_ID_UPDATE_DIRECTORY[]   = "CVS.UpdateDirectory";
const char CMD_ID_COMMIT_DIRECTORY[]   = "CVS.CommitDirectory";
const char CMD_ID_UPDATE[]             = "CVS.Update";
const char CMD_ID_PROJECTLOG[]         = "CVS.ProjectLog";
const char CMD_ID_PROJECTCOMMIT[]      = "CVS.ProjectCommit";
const char CMD_ID_REPOSITORYLOG[]      = "CVS.RepositoryLog";
const char CMD_ID_REPOSITORYDIFF[]     = "CVS.RepositoryDiff";
const char CMD_ID_REPOSITORYSTATUS[]   = "CVS.RepositoryStatus";
const char CMD_ID_REPOSITORYUPDATE[]   = "CVS.RepositoryUpdate";

const char CVS_SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.cvs.submit";
const char CVSCOMMITEDITOR_ID[]  = "CVS Commit Editor";
const char CVSCOMMITEDITOR_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("VCS", "CVS Commit Editor");
const char SUBMIT_CURRENT[] = "CVS.SubmitCurrentLog";
const char DIFF_SELECTED[] = "CVS.DiffSelectedFilesInLog";

const VcsBaseEditorParameters editorParameters[] = {
{
    OtherContent,
    "CVS Command Log Editor", // id
    QT_TRANSLATE_NOOP("VCS", "CVS Command Log Editor"), // display name
    "text/vnd.qtcreator.cvs.commandlog"},
{   LogOutput,
    "CVS File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "CVS File Log Editor"),   // display name
    "text/vnd.qtcreator.cvs.log"},
{    AnnotateOutput,
    "CVS Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Annotation Editor"),  // display name
    "text/vnd.qtcreator.cvs.annotation"},
{   DiffOutput,
    "CVS Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Diff Editor"),  // display name
    "text/x-patch"}
};

// Utility to find a parameter set by type
static inline const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return VcsBaseEditor::findType(editorParameters, sizeof(editorParameters) / sizeof(editorParameters[0]), et);
}

static inline bool messageBoxQuestion(const QString &title, const QString &question)
{
    return QMessageBox::question(ICore::dialogParent(), title, question, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes;
}

// ------------- CVSPlugin
CvsPlugin *CvsPlugin::m_cvsPluginInstance = 0;

CvsPlugin::~CvsPlugin()
{
    delete m_client;
    cleanCommitMessageFile();
}

CvsClient *CvsPlugin::client() const
{
    QTC_CHECK(m_client);
    return m_client;
}

void CvsPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}
bool CvsPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

static const VcsBaseSubmitEditorParameters submitParameters = {
    CVS_SUBMIT_MIMETYPE,
    CVSCOMMITEDITOR_ID,
    CVSCOMMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

bool CvsPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    using namespace Core::Constants;

    Context context(CVS_CONTEXT);

    initializeVcs<CvsControl>(context, this);

    m_cvsPluginInstance = this;

    m_client = new CvsClient;

    addAutoReleasedObject(new SettingsPage(versionControl()));

    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new CvsSubmitEditor(&submitParameters); }));

    const auto describeFunc = [this](const QString &source, const QString &changeNr) {
        QString errorMessage;
        if (!describe(source, changeNr, &errorMessage))
            VcsOutputWindow::appendError(errorMessage);
    };
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new CvsEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc));

    const QString prefix = QLatin1String("cvs");
    m_commandLocator = new CommandLocator("CVS", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *cvsMenu = ActionManager::createMenu(Id(CMD_ID_CVS_MENU));
    cvsMenu->menu()->setTitle(tr("&CVS"));
    toolsContainer->addMenu(cvsMenu);
    m_menuAction = cvsMenu->menu()->menuAction();

    Command *command;

    m_diffCurrentAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+D") : tr("Alt+C,Alt+D")));
    connect(m_diffCurrentAction, &QAction::triggered, this, &CvsPlugin::diffCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_filelogCurrentAction, &QAction::triggered, this, &CvsPlugin::filelogCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateCurrentAction, &QAction::triggered, this, &CvsPlugin::annotateCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+A") : tr("Alt+C,Alt+A")));
    connect(m_addAction, &QAction::triggered, this, &CvsPlugin::addCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+C") : tr("Alt+C,Alt+C")));
    connect(m_commitCurrentAction, &QAction::triggered, this, &CvsPlugin::startCommitCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &CvsPlugin::promptToDeleteCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new ParameterAction(tr("Revert..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertAction, &QAction::triggered, this, &CvsPlugin::revertCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_editCurrentAction = new ParameterAction(tr("Edit"), tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editCurrentAction, CMD_ID_EDIT_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_editCurrentAction, &QAction::triggered, this, &CvsPlugin::editCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditCurrentAction = new ParameterAction(tr("Unedit"), tr("Unedit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_uneditCurrentAction, CMD_ID_UNEDIT_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_uneditCurrentAction, &QAction::triggered, this, &CvsPlugin::uneditCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditRepositoryAction = new QAction(tr("Unedit Repository"), this);
    command = ActionManager::registerAction(m_uneditRepositoryAction, CMD_ID_UNEDIT_REPOSITORY, context);
    connect(m_uneditRepositoryAction, &QAction::triggered, this, &CvsPlugin::uneditCurrentRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_diffProjectAction = new ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_diffProjectAction, &QAction::triggered, this, &CvsPlugin::diffProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_statusProjectAction, &QAction::triggered, this, &CvsPlugin::projectStatus);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &CvsPlugin::logProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, &QAction::triggered, this, &CvsPlugin::updateProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new ParameterAction(tr("Commit Project"), tr("Commit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitProjectAction, CMD_ID_PROJECTCOMMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_commitProjectAction, &QAction::triggered, this, &CvsPlugin::commitProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_updateDirectoryAction = new ParameterAction(tr("Update Directory"), tr("Update Directory \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateDirectoryAction, CMD_ID_UPDATE_DIRECTORY, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateDirectoryAction, &QAction::triggered, this, &CvsPlugin::updateDirectory);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitDirectoryAction = new ParameterAction(tr("Commit Directory"), tr("Commit Directory \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitDirectoryAction,
        CMD_ID_COMMIT_DIRECTORY, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_commitDirectoryAction, &QAction::triggered, this, &CvsPlugin::startCommitDirectory);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, context);
    connect(m_diffRepositoryAction, &QAction::triggered, this, &CvsPlugin::diffRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, context);
    connect(m_statusRepositoryAction, &QAction::triggered, this, &CvsPlugin::statusRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &CvsPlugin::logRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(tr("Update Repository"), this);
    command = ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, context);
    connect(m_updateRepositoryAction, &QAction::triggered, this, &CvsPlugin::updateRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        context);
    connect(m_commitAllAction, &QAction::triggered, this, &CvsPlugin::startCommitAll);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertRepositoryAction = new QAction(tr("Revert Repository..."), this);
    command = ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
                             context);
    connect(m_revertRepositoryAction, &QAction::triggered, this, &CvsPlugin::revertAll);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Context cvscommitcontext(CVSCOMMITEDITOR_ID);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ActionManager::registerAction(m_submitCurrentLogAction, SUBMIT_CURRENT, cvscommitcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, &QAction::triggered, this, &CvsPlugin::submitCurrentLog);

    m_submitDiffAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_submitDiffAction , DIFF_SELECTED, cvscommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_submitUndoAction, Core::Constants::UNDO, cvscommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_submitRedoAction, Core::Constants::REDO, cvscommitcontext);
    return true;
}

bool CvsPlugin::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;

    CvsSubmitEditor *editor = qobject_cast<CvsSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing CVS Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 client()->settings().boolPointer(CvsSettings::promptOnSubmitKey),
                                 !m_submitActionTriggered);
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
    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        closeEditor = DocumentManager::saveDocument(editorDocument);
        if (closeEditor)
            closeEditor = commit(m_commitMessageFileName, fileList);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void CvsPlugin::diffCommitFiles(const QStringList &files)
{
    m_client->diff(m_commitRepository, files);
}

static void setDiffBaseDirectory(IEditor *editor, const QString &db)
{
    if (VcsBaseEditorWidget *ve = qobject_cast<VcsBaseEditorWidget*>(editor->widget()))
        ve->setWorkingDirectory(db);
}

CvsSubmitEditor *CvsPlugin::openCVSSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(fileName, CVSCOMMITEDITOR_ID);
    CvsSubmitEditor *submitEditor = qobject_cast<CvsSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &CvsPlugin::diffCommitFiles);

    return submitEditor;
}

void CvsPlugin::updateActions(VcsBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }

    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);

    const QString currentFileName = currentState().currentFileName();
    m_addAction->setParameter(currentFileName);
    m_deleteAction->setParameter(currentFileName);
    m_revertAction->setParameter(currentFileName);
    m_diffCurrentAction->setParameter(currentFileName);
    m_commitCurrentAction->setParameter(currentFileName);
    m_filelogCurrentAction->setParameter(currentFileName);
    m_annotateCurrentAction->setParameter(currentFileName);
    m_editCurrentAction->setParameter(currentFileName);
    m_uneditCurrentAction->setParameter(currentFileName);

    const QString currentProjectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(currentProjectName);
    m_statusProjectAction->setParameter(currentProjectName);
    m_updateProjectAction->setParameter(currentProjectName);
    m_logProjectAction->setParameter(currentProjectName);
    m_commitProjectAction->setParameter(currentProjectName);

    // TODO: Find a more elegant way to shorten the path
    QString currentDirectoryName = QDir::toNativeSeparators(currentState().currentFileDirectory());
    if (currentDirectoryName.size() > 15)
        currentDirectoryName.replace(0, currentDirectoryName.size() - 15, QLatin1String("..."));
    m_updateDirectoryAction->setParameter(currentDirectoryName);
    m_commitDirectoryAction->setParameter(currentDirectoryName);

    m_diffRepositoryAction->setEnabled(hasTopLevel);
    m_statusRepositoryAction->setEnabled(hasTopLevel);
    m_updateRepositoryAction->setEnabled(hasTopLevel);
    m_commitAllAction->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);
    m_uneditRepositoryAction->setEnabled(hasTopLevel);
}

void CvsPlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPlugin::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = tr("Revert Repository");
    if (!messageBoxQuestion(title, tr("Revert all pending changes to the repository?")))
        return;
    QStringList args;
    args << QLatin1String("update") << QLatin1String("-C") << state.topLevel();
    const CvsResponse revertResponse =
            runCvs(state.topLevel(), args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    if (revertResponse.result == CvsResponse::Ok)
        cvsVersionControl()->emitRepositoryChanged(state.topLevel());
    else
        Core::AsynchronousMessageBox::warning(title,
                                              tr("Revert failed: %1").arg(revertResponse.message));
}

void CvsPlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QStringList args;
    args << QLatin1String("diff") << state.relativeCurrentFile();
    const CvsResponse diffResponse =
            runCvs(state.currentFileTopLevel(), args, client()->vcsTimeoutS(), 0);
    switch (diffResponse.result) {
    case CvsResponse::Ok:
        return; // Not modified, diff exit code 0
    case CvsResponse::NonNullExitCode: // Diff exit code != 0
        if (diffResponse.stdOut.isEmpty()) // Paranoia: Something else failed?
            return;
        break;
    case CvsResponse::OtherError:
        return;
    }

    if (!messageBoxQuestion(QLatin1String("CVS Revert"),
                            tr("The file has been changed. Do you want to revert it?")))
        return;

    FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("update") << QLatin1String("-C") << state.relativeCurrentFile();
    const CvsResponse revertResponse =
            runCvs(state.currentFileTopLevel(), args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    if (revertResponse.result == CvsResponse::Ok)
        cvsVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void CvsPlugin::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    m_client->diff(state.currentProjectTopLevel(),
            relativeProject.isEmpty() ? QStringList() : QStringList(relativeProject));
}

void CvsPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    /* The following has the same effect as
       startCommit(state.currentFileTopLevel(), state.relativeCurrentFile()),
       but is faster when the project has multiple directory levels */
    startCommit(state.currentFileDirectory(), state.currentFileName());
}

void CvsPlugin::startCommitDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileDirectory());
}

void CvsPlugin::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void CvsPlugin::startCommit(const QString &workingDir, const QString &file)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    // We need the "Examining <subdir>" stderr output to tell
    // where we are, so, have stdout/stderr channels merged.
    QStringList args = QStringList(QLatin1String("status"));
    const CvsResponse response =
            runCvs(workingDir, args, client()->vcsTimeoutS(), VcsCommand::MergeOutputChannels);
    if (response.result != CvsResponse::Ok)
        return;
    // Get list of added/modified/deleted files and purge out undesired ones
    // (do not run status with relative arguments as it will omit the directories)
    StateList statusOutput = parseStatusOutput(QString(), response.stdOut);
    if (!file.isEmpty()) {
        for (StateList::iterator it = statusOutput.begin(); it != statusOutput.end() ; ) {
            if (file == it->second)
                ++it;
            else
                it = statusOutput.erase(it);
        }
    }
    if (statusOutput.empty()) {
        VcsOutputWindow::appendWarning(tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;

    // Create a new submit change file containing the submit template
    TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    // Create a submit editor and set file list
    CvsSubmitEditor *editor = openCVSSubmitEditor(m_commitMessageFileName);
    setSubmitEditor(editor);
    editor->setCheckScriptWorkingDirectory(m_commitRepository);
    editor->setStateList(statusOutput);
}

bool CvsPlugin::commit(const QString &messageFile,
                              const QStringList &fileList)
{
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String("-F") << messageFile;
    args.append(fileList);
    const CvsResponse response =
            runCvs(m_commitRepository, args, 10 * client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    return response.result == CvsResponse::Ok ;
}

void CvsPlugin::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void CvsPlugin::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void CvsPlugin::filelog(const QString &workingDir,
                        const QString &file,
                        bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(file));
    // no need for temp file
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(file));
    const QString source = VcsBaseEditor::getSource(workingDir, file);
    QStringList args;
    args << QLatin1String("log");
    args.append(file);
    const CvsResponse response =
            runCvs(workingDir, args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt, codec);
    if (response.result != CvsResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditor::editorTag(LogOutput, workingDir, QStringList(file));
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs log %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, LogOutput, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void CvsPlugin::updateDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    update(state.currentFileDirectory(), QString());
}

void CvsPlugin::updateProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    update(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

bool CvsPlugin::update(const QString &topLevel, const QString &file)
{
    QStringList args(QLatin1String("update"));
    args.push_back(QLatin1String("-dR"));
    if (!file.isEmpty())
        args.append(file);
    const CvsResponse response =
            runCvs(topLevel, args, 10 * client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    const bool ok = response.result == CvsResponse::Ok;
    if (ok)
        cvsVersionControl()->emitRepositoryChanged(topLevel);
    return ok;
}

void CvsPlugin::editCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    edit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::uneditCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    unedit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::uneditCurrentRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    unedit(state.topLevel(), QStringList());
}

void CvsPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPlugin::vcsAnnotate(const QString &workingDirectory, const QString &file,
                            const QString &revision, int lineNumber)
{
    annotate(workingDirectory, file, revision, lineNumber);
}

bool CvsPlugin::edit(const QString &topLevel, const QStringList &files)
{
    QStringList args(QLatin1String("edit"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, client()->vcsTimeoutS(),
                   VcsCommand::ShowStdOut | VcsCommand::SshPasswordPrompt);
    return response.result == CvsResponse::Ok;
}

bool CvsPlugin::diffCheckModified(const QString &topLevel, const QStringList &files, bool *modified)
{
    // Quick check for modified files using diff
    *modified = false;
    QStringList args(QLatin1String("-q"));
    args << QLatin1String("diff");
    args.append(files);
    const CvsResponse response = runCvs(topLevel, args, client()->vcsTimeoutS(), 0);
    if (response.result == CvsResponse::OtherError)
        return false;
    *modified = response.result == CvsResponse::NonNullExitCode;
    return true;
}

bool CvsPlugin::unedit(const QString &topLevel, const QStringList &files)
{
    bool modified;
    // Prompt and use force flag if modified
    if (!diffCheckModified(topLevel, files, &modified))
        return false;
    if (modified) {
        const QString question = files.isEmpty() ?
                      tr("Would you like to discard your changes to the repository \"%1\"?").arg(topLevel) :
                      tr("Would you like to discard your changes to the file \"%1\"?").arg(files.front());
        if (!messageBoxQuestion(tr("Unedit"), question))
            return false;
    }

    QStringList args(QLatin1String("unedit"));
    // Note: Option '-y' to force 'yes'-answer to CVS' 'undo change' prompt,
    // exists in CVSNT only as of 6.8.2010. Standard CVS will otherwise prompt
    if (modified)
        args.append(QLatin1String("-y"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, client()->vcsTimeoutS(),
                   VcsCommand::ShowStdOut | VcsCommand::SshPasswordPrompt);
    return response.result == CvsResponse::Ok;
}

void CvsPlugin::annotate(const QString &workingDir, const QString &file,
                         const QString &revision /* = QString() */,
                         int lineNumber /* = -1 */)
{
    const QStringList files(file);
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, revision);
    const QString source = VcsBaseEditor::getSource(workingDir, file);
    QStringList args;
    args << QLatin1String("annotate");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args << file;
    const CvsResponse response =
            runCvs(workingDir, args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt, codec);
    if (response.result != CvsResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber < 1)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(file);

    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, QStringList(file), revision);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, AnnotateOutput, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

bool CvsPlugin::status(const QString &topLevel, const QString &file, const QString &title)
{
    QStringList args(QLatin1String("status"));
    if (!file.isEmpty())
        args.append(file);
    const CvsResponse response =
            runCvs(topLevel, args, client()->vcsTimeoutS(), 0);
    const bool ok = response.result == CvsResponse::Ok;
    if (ok)
        showOutputInEditor(title, response.stdOut, OtherContent, topLevel, 0);
    return ok;
}

void CvsPlugin::projectStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    status(state.currentProjectTopLevel(), state.relativeCurrentProject(), tr("Project status"));
}

void CvsPlugin::commitProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel(), QStringList());
}

void CvsPlugin::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    status(state.topLevel(), QString(), tr("Repository status"));
}

void CvsPlugin::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    update(state.topLevel(), QString());

}

bool CvsPlugin::describe(const QString &file, const QString &changeNr, QString *errorMessage)
{

    QString toplevel;
    const bool manages = managesDirectory(QFileInfo(file).absolutePath(), &toplevel);
    if (!manages || toplevel.isEmpty()) {
        *errorMessage = msgCannotFindTopLevel(file);
        return false;
    }
    return describe(toplevel, QDir(toplevel).relativeFilePath(file), changeNr, errorMessage);
}

bool CvsPlugin::describe(const QString &toplevel, const QString &file, const
                         QString &changeNr, QString *errorMessage)
{

    // In CVS, revisions of files are normally unrelated, there is
    // no global revision/change number. The only thing that groups
    // a commit is the "commit-id" (as shown in the log).
    // This function makes use of it to find all files related to
    // a commit in order to emulate a "describe global change" functionality
    // if desired.
    // Number must be > 1
    if (isFirstRevision(changeNr)) {
        *errorMessage = tr("The initial revision %1 cannot be described.").arg(changeNr);
        return false;
    }
    // Run log to obtain commit id and details
    QStringList args;
    args << QLatin1String("log") << (QLatin1String("-r") + changeNr) << file;
    const CvsResponse logResponse =
            runCvs(toplevel, args, client()->vcsTimeoutS(), VcsCommand::SshPasswordPrompt);
    if (logResponse.result != CvsResponse::Ok) {
        *errorMessage = logResponse.message;
        return false;
    }
    const QList<CvsLogEntry> fileLog = parseLogEntries(logResponse.stdOut);
    if (fileLog.empty() || fileLog.front().revisions.empty()) {
        *errorMessage = msgLogParsingFailed();
        return false;
    }
    if (client()->settings().boolValue(CvsSettings::describeByCommitIdKey)) {
        // Run a log command over the repo, filtering by the commit date
        // and commit id, collecting all files touched by the commit.
        const QString commitId = fileLog.front().revisions.front().commitId;
        // Date range "D1<D2" in ISO format "YYYY-MM-DD"
        const QString dateS = fileLog.front().revisions.front().date;
        const QDate date = QDate::fromString(dateS, Qt::ISODate);
        const QString nextDayS = date.addDays(1).toString(Qt::ISODate);
        args.clear();
        args << QLatin1String("log") << QLatin1String("-d") << (dateS  + QLatin1Char('<') + nextDayS);

        const CvsResponse repoLogResponse =
                runCvs(toplevel, args, 10 * client()->vcsTimeoutS(), VcsCommand::SshPasswordPrompt);
        if (repoLogResponse.result != CvsResponse::Ok) {
            *errorMessage = repoLogResponse.message;
            return false;
        }
        // Describe all files found, pass on dir to obtain correct absolute paths.
        const QList<CvsLogEntry> repoEntries = parseLogEntries(repoLogResponse.stdOut, QString(), commitId);
        if (repoEntries.empty()) {
            *errorMessage = tr("Could not find commits of id \"%1\" on %2.").arg(commitId, dateS);
            return false;
        }
        return describe(toplevel, repoEntries, errorMessage);
    } else {
        // Just describe that single entry
        return describe(toplevel, fileLog, errorMessage);
    }
    return false;
}

// Describe a set of files and revisions by
// concatenating log and diffs to previous revisions
bool CvsPlugin::describe(const QString &repositoryPath,
                         QList<CvsLogEntry> entries,
                         QString *errorMessage)
{
    // Collect logs
    QString output;
    QTextCodec *codec = 0;
    const QList<CvsLogEntry>::iterator lend = entries.end();
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        // Before fiddling file names, try to find codec
        if (!codec)
            codec = VcsBaseEditor::getCodec(repositoryPath, QStringList(it->file));
        // Run log
        QStringList args(QLatin1String("log"));
        args << (QLatin1String("-r") + it->revisions.front().revision) << it->file;
        const CvsResponse logResponse =
                runCvs(repositoryPath, args, client()->vcsTimeoutS(), VcsCommand::SshPasswordPrompt);
        if (logResponse.result != CvsResponse::Ok) {
            *errorMessage =  logResponse.message;
            return false;
        }
        output += logResponse.stdOut;
    }
    // Collect diffs relative to repository
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        const QString &revision = it->revisions.front().revision;
        if (!isFirstRevision(revision)) {
            const QString previousRev = previousRevision(revision);
            QStringList args(QLatin1String("diff"));
            args << client()->settings().stringValue(CvsSettings::diffOptionsKey)
                 << QLatin1String("-r") << previousRev << QLatin1String("-r")
                 << it->revisions.front().revision << it->file;
            const CvsResponse diffResponse =
                    runCvs(repositoryPath, args, client()->vcsTimeoutS(), 0, codec);
            switch (diffResponse.result) {
            case CvsResponse::Ok:
            case CvsResponse::NonNullExitCode: // Diff exit code != 0
                if (diffResponse.stdOut.isEmpty()) {
                    *errorMessage = diffResponse.message;
                    return false; // Something else failed.
                }
                break;
            case CvsResponse::OtherError:
                *errorMessage = diffResponse.message;
                return false;
            }
            output += fixDiffOutput(diffResponse.stdOut);
        }
    }

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString commitId = entries.front().revisions.front().commitId;
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(commitId)) {
        editor->document()->setContents(output.toUtf8());
        EditorManager::activateEditor(editor);
        setDiffBaseDirectory(editor, repositoryPath);
    } else {
        const QString title = QString::fromLatin1("cvs describe %1").arg(commitId);
        IEditor *newEditor = showOutputInEditor(title, output, DiffOutput, entries.front().file, codec);
        VcsBaseEditor::tagEditor(newEditor, commitId);
        setDiffBaseDirectory(newEditor, repositoryPath);
    }
    return true;
}

void CvsPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

// Run CVS. At this point, file arguments must be relative to
// the working directory (see above).
CvsResponse CvsPlugin::runCvs(const QString &workingDirectory,
                              const QStringList &arguments,
                              int timeOutS,
                              unsigned flags,
                              QTextCodec *outputCodec) const
{
    const FileName executable = client()->vcsBinary();
    CvsResponse response;
    if (executable.isEmpty()) {
        response.result = CvsResponse::OtherError;
        response.message =tr("No CVS executable specified.");
        return response;
    }
    // Run, connect stderr to the output window
    const SynchronousProcessResponse sp_resp =
            runVcs(workingDirectory, executable, client()->settings().addOptions(arguments),
                   timeOutS, flags, outputCodec);

    response.result = CvsResponse::OtherError;
    response.stdErr = sp_resp.stdErr();
    response.stdOut = sp_resp.stdOut();
    switch (sp_resp.result) {
    case SynchronousProcessResponse::Finished:
        response.result = CvsResponse::Ok;
        break;
    case SynchronousProcessResponse::FinishedError:
        response.result = CvsResponse::NonNullExitCode;
        break;
    case SynchronousProcessResponse::TerminatedAbnormally:
    case SynchronousProcessResponse::StartFailed:
    case SynchronousProcessResponse::Hang:
        break;
    }

    if (response.result != CvsResponse::Ok)
        response.message = sp_resp.exitMessage(executable.toString(), timeOutS);

    return response;
}

IEditor *CvsPlugin::showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec)
{
    const VcsBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const Id id = params->id;
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    CvsEditorWidget *e = qobject_cast<CvsEditorWidget*>(editor->widget());
    if (!e)
        return 0;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested, this, &CvsPlugin::annotate);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    e->setForceReadOnly(true);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

CvsPlugin *CvsPlugin::instance()
{
    QTC_ASSERT(m_cvsPluginInstance, return m_cvsPluginInstance);
    return m_cvsPluginInstance;
}

bool CvsPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("add") << rawFileName;
    const CvsResponse response =
            runCvs(workingDir, args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    return response.result == CvsResponse::Ok;
}

bool CvsPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("remove") << QLatin1String("-f") << rawFileName;
    const CvsResponse response =
            runCvs(workingDir, args, client()->vcsTimeoutS(),
                   VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    return response.result == CvsResponse::Ok;
}

/* CVS has a "CVS" directory in each directory it manages. The top level
 * is the first directory under the directory that does not have it. */
bool CvsPlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    if (topLevel)
        topLevel->clear();
    bool manages = false;
    const QDir dir(directory);
    do {
        if (!dir.exists() || !checkCVSDirectory(dir))
            break;
        manages = true;
        if (!topLevel)
            break;
        /* Recursing up, the top level is a child of the first directory that does
         * not have a  "CVS" directory. The starting directory must be a managed
         * one. Go up and try to find the first unmanaged parent dir. */
        QDir lastDirectory = dir;
        for (QDir parentDir = lastDirectory;
             !parentDir.isRoot() && parentDir.cdUp();
             lastDirectory = parentDir) {
            if (!checkCVSDirectory(parentDir)) {
                *topLevel = lastDirectory.absolutePath();
                break;
            }
        }
    } while (false);

    return manages;
}

bool CvsPlugin::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("status") << fileName;
    const CvsResponse response =
            runCvs(workingDirectory, args, client()->vcsTimeoutS(), VcsCommand::SshPasswordPrompt);
    if (response.result != CvsResponse::Ok)
        return false;
    return !response.stdOut.contains(QLatin1String("Status: Unknown"));
}

bool CvsPlugin::checkCVSDirectory(const QDir &directory) const
{
    const QString cvsDir = directory.absoluteFilePath(QLatin1String("CVS"));
    return QFileInfo(cvsDir).isDir();
}

CvsControl *CvsPlugin::cvsVersionControl() const
{
    return static_cast<CvsControl *>(versionControl());
}

#ifdef WITH_TESTS
void CvsPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("Modified") << QByteArray(
            "Index: src/plugins/cvs/cvseditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:20 -0000\t1.1\n"
            "+++ src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:28 -0000\n"
            "@@ -120,7 +120,7 @@\n\n")
        << QByteArray("src/plugins/cvs/cvseditor.cpp");
}

void CvsPlugin::testDiffFileResolving()
{
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[3].id);
}

void CvsPlugin::testLogResolving()
{
    QByteArray data(
                "RCS file: /sources/cvs/ccvs/Attic/FIXED-BUGS,v\n"
                "Working file: FIXED-BUGS\n"
                "head: 1.3\n"
                "branch:\n"
                "locks: strict\n"
                "access list:\n"
                "symbolic names:\n"
                "keyword substitution: kv\n"
                "total revisions: 3;     selected revisions: 3\n"
                "description:\n"
                "----------------------------\n"
                "revision 1.3\n"
                "date: 1995-04-29 06:22:41 +0300;  author: jimb;  state: dead;  lines: +0 -0;\n"
                "*** empty log message ***\n"
                "----------------------------\n"
                "revision 1.2\n"
                "date: 1995-04-28 18:52:24 +0300;  author: noel;  state: Exp;  lines: +6 -0;\n"
                "added latest commentary\n"
                "----------------------------\n"
                );
    VcsBaseEditorWidget::testLogResolving(editorParameters[1].id, data, "1.3", "1.2");
}
#endif

} // namespace Internal
} // namespace Cvs
