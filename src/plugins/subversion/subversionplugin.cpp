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

#include "subversionplugin.h"

#include "settingspage.h"
#include "subversioneditor.h"

#include "subversionsubmiteditor.h"
#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversioncontrol.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsoutputwindow.h>

#include <texteditor/textdocument.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QtPlugin>
#include <QProcessEnvironment>
#include <QUrl>
#include <QXmlStreamReader>
#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

#include <limits.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

const char CMD_ID_SUBVERSION_MENU[]    = "Subversion.Menu";
const char CMD_ID_ADD[]                = "Subversion.Add";
const char CMD_ID_DELETE_FILE[]        = "Subversion.Delete";
const char CMD_ID_REVERT[]             = "Subversion.Revert";
const char CMD_ID_DIFF_PROJECT[]       = "Subversion.DiffAll";
const char CMD_ID_DIFF_CURRENT[]       = "Subversion.DiffCurrent";
const char CMD_ID_COMMIT_ALL[]         = "Subversion.CommitAll";
const char CMD_ID_REVERT_ALL[]         = "Subversion.RevertAll";
const char CMD_ID_COMMIT_CURRENT[]     = "Subversion.CommitCurrent";
const char CMD_ID_FILELOG_CURRENT[]    = "Subversion.FilelogCurrent";
const char CMD_ID_ANNOTATE_CURRENT[]   = "Subversion.AnnotateCurrent";
const char CMD_ID_STATUS[]             = "Subversion.Status";
const char CMD_ID_PROJECTLOG[]         = "Subversion.ProjectLog";
const char CMD_ID_REPOSITORYLOG[]      = "Subversion.RepositoryLog";
const char CMD_ID_REPOSITORYUPDATE[]   = "Subversion.RepositoryUpdate";
const char CMD_ID_REPOSITORYDIFF[]     = "Subversion.RepositoryDiff";
const char CMD_ID_REPOSITORYSTATUS[]   = "Subversion.RepositoryStatus";
const char CMD_ID_UPDATE[]             = "Subversion.Update";
const char CMD_ID_COMMIT_PROJECT[]     = "Subversion.CommitProject";
const char CMD_ID_DESCRIBE[]           = "Subversion.Describe";

const VcsBaseEditorParameters editorParameters[] = {
{
    LogOutput,
    Constants::SUBVERSION_LOG_EDITOR_ID,
    Constants::SUBVERSION_LOG_EDITOR_DISPLAY_NAME,
    Constants::SUBVERSION_LOG_MIMETYPE},
{    AnnotateOutput,
     Constants::SUBVERSION_BLAME_EDITOR_ID,
     Constants::SUBVERSION_BLAME_EDITOR_DISPLAY_NAME,
     Constants::SUBVERSION_BLAME_MIMETYPE}
};

// Utility to find a parameter set by type
static const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return VcsBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(editorParameters[0]), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

// Parse "svn status" output for added/conflicted/deleted/modified files
// "M<7blanks>file"
typedef QList<SubversionSubmitEditor::StatusFilePair> StatusList;

StatusList parseStatusOutput(const QString &output)
{
    StatusList changeSet;
    const QString newLine = QString(QLatin1Char('\n'));
    const QStringList list = output.split(newLine, QString::SkipEmptyParts);
    foreach (const QString &l, list) {
        const QString line =l.trimmed();
        if (line.size() > 8) {
            const QByteArray state = line.left(1).toLatin1();
            if (state == FileAddedC || state == FileConflictedC
                    || state == FileDeletedC || state == FileModifiedC) {
                const QString fileName = line.mid(7); // Column 8 starting from svn 1.6
                changeSet.push_back(SubversionSubmitEditor::StatusFilePair(QLatin1String(state),
                                                                           fileName.trimmed()));
            }

        }
    }
    return changeSet;
}

// Return a list of names for the internal svn directories
static inline QStringList svnDirectories()
{
    QStringList rc(QLatin1String(".svn"));
    if (HostOsInfo::isWindowsHost())
        // Option on Windows systems to avoid hassle with some IDEs
        rc.push_back(QLatin1String("_svn"));
    return rc;
}

// ------------- SubversionPlugin
SubversionPlugin *SubversionPlugin::m_subversionPluginInstance = 0;

SubversionPlugin::SubversionPlugin() :
    m_svnDirectories(svnDirectories())
{
}

SubversionPlugin::~SubversionPlugin()
{
    delete m_client;
    cleanCommitMessageFile();
}

void SubversionPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}

bool SubversionPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

const VcsBaseSubmitEditorParameters submitParameters = {
    Constants::SUBVERSION_SUBMIT_MIMETYPE,
    Constants::SUBVERSION_COMMIT_EDITOR_ID,
    Constants::SUBVERSION_COMMIT_EDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

bool SubversionPlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    using namespace Constants;
    using namespace Core::Constants;

    Context context(SUBVERSION_CONTEXT);

    initializeVcs(new SubversionControl(this), context);

    m_subversionPluginInstance = this;

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/trolltech.subversion/Subversion.mimetypes.xml"));

    m_client = new SubversionClient;

    addAutoReleasedObject(new SettingsPage(versionControl()));

    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new SubversionSubmitEditor(&submitParameters); }));

    const auto describeFunc = [this](const QString &source, const QString &id) {
        describe(source, id);
    };
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new SubversionEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc));

    const QString prefix = QLatin1String("svn");
    m_commandLocator = new CommandLocator("Subversion", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *subversionMenu = ActionManager::createMenu(Id(CMD_ID_SUBVERSION_MENU));
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    toolsContainer->addMenu(subversionMenu);
    m_menuAction = subversionMenu->menu()->menuAction();
    Command *command;

    m_diffCurrentAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+D") : tr("Alt+S,Alt+D")));
    connect(m_diffCurrentAction, &QAction::triggered, this, &SubversionPlugin::diffCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_filelogCurrentAction, &QAction::triggered, this, &SubversionPlugin::filelogCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateCurrentAction, &QAction::triggered, this, &SubversionPlugin::annotateCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+A") : tr("Alt+S,Alt+A")));
    connect(m_addAction, &QAction::triggered, this, &SubversionPlugin::addCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+C") : tr("Alt+S,Alt+C")));
    connect(m_commitCurrentAction, &QAction::triggered, this, &SubversionPlugin::startCommitCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &SubversionPlugin::promptToDeleteCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new ParameterAction(tr("Revert..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertAction, &QAction::triggered, this, &SubversionPlugin::revertCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_diffProjectAction = new ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_diffProjectAction, &QAction::triggered, this, &SubversionPlugin::diffProject);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_statusProjectAction, &QAction::triggered, this, &SubversionPlugin::projectStatus);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &SubversionPlugin::logProject);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, context);
    connect(m_updateProjectAction, &QAction::triggered, this, &SubversionPlugin::updateProject);
    command->setAttribute(Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new ParameterAction(tr("Commit Project"), tr("Commit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitProjectAction, CMD_ID_COMMIT_PROJECT, context);
    connect(m_commitProjectAction, &QAction::triggered, this, &SubversionPlugin::startCommitProject);
    command->setAttribute(Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, context);
    connect(m_diffRepositoryAction, &QAction::triggered, this, &SubversionPlugin::diffRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, context);
    connect(m_statusRepositoryAction, &QAction::triggered, this, &SubversionPlugin::statusRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Log Repository"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &SubversionPlugin::logRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(tr("Update Repository"), this);
    command = ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, context);
    connect(m_updateRepositoryAction, &QAction::triggered, this, &SubversionPlugin::updateRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        context);
    connect(m_commitAllAction, &QAction::triggered, this, &SubversionPlugin::startCommitAll);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, context);
    connect(m_describeAction, &QAction::triggered, this, &SubversionPlugin::slotDescribe);
    subversionMenu->addAction(command);

    m_revertRepositoryAction = new QAction(tr("Revert Repository..."), this);
    command = ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
        context);
    connect(m_revertRepositoryAction, &QAction::triggered, this, &SubversionPlugin::revertAll);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Context svncommitcontext(Constants::SUBVERSION_COMMIT_EDITOR_ID);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ActionManager::registerAction(m_submitCurrentLogAction, SUBMIT_CURRENT, svncommitcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, &QAction::triggered, this, &SubversionPlugin::submitCurrentLog);

    m_submitDiffAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_submitDiffAction , DIFF_SELECTED, svncommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_submitUndoAction, Core::Constants::UNDO, svncommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_submitRedoAction, Core::Constants::REDO, svncommitcontext);

    return true;
}

bool SubversionPlugin::isVcsDirectory(const FileName &fileName)
{
    const QString baseName = fileName.fileName();
    return fileName.toFileInfo().isDir()
            && contains(m_svnDirectories, [baseName](const QString &s) {
        return !baseName.compare(s, HostOsInfo::fileNameCaseSensitivity());
    });
}

SubversionClient *SubversionPlugin::client() const
{
    QTC_CHECK(m_client);
    return m_client;
}

bool SubversionPlugin::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;

    SubversionSubmitEditor *editor = qobject_cast<SubversionSubmitEditor *>(submitEditor());
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
    VcsBaseClientSettings &newSettings = client()->settings();
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing Subversion Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 newSettings.boolPointer(SubversionSettings::promptOnSubmitKey),
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
        if (closeEditor) {
            VcsCommand *commitCmd = m_client->createCommitCmd(m_commitRepository,
                                                              fileList,
                                                              m_commitMessageFileName);
            QObject::connect(commitCmd, &VcsCommand::finished,
                             this, [this]() { cleanCommitMessageFile(); });
            commitCmd->execute();
        }
    }
    return closeEditor;
}

void SubversionPlugin::diffCommitFiles(const QStringList &files)
{
    m_client->diff(m_commitRepository, files, QStringList());
}

SubversionSubmitEditor *SubversionPlugin::openSubversionSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(fileName, Constants::SUBVERSION_COMMIT_EDITOR_ID);
    SubversionSubmitEditor *submitEditor = qobject_cast<SubversionSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    setSubmitEditor(submitEditor);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &SubversionPlugin::diffCommitFiles);
    submitEditor->setCheckScriptWorkingDirectory(m_commitRepository);
    return submitEditor;
}

void SubversionPlugin::updateActions(VcsBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);

    const QString projectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(projectName);
    m_statusProjectAction->setParameter(projectName);
    m_updateProjectAction->setParameter(projectName);
    m_logProjectAction->setParameter(projectName);
    m_commitProjectAction->setParameter(projectName);

    const bool repoEnabled = currentState().hasTopLevel();
    m_commitAllAction->setEnabled(repoEnabled);
    m_describeAction->setEnabled(repoEnabled);
    m_revertRepositoryAction->setEnabled(repoEnabled);
    m_diffRepositoryAction->setEnabled(repoEnabled);
    m_statusRepositoryAction->setEnabled(repoEnabled);
    m_updateRepositoryAction->setEnabled(repoEnabled);

    const QString fileName = currentState().currentFileName();

    m_addAction->setParameter(fileName);
    m_deleteAction->setParameter(fileName);
    m_revertAction->setParameter(fileName);
    m_diffCurrentAction->setParameter(fileName);
    m_commitCurrentAction->setParameter(fileName);
    m_filelogCurrentAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
}

void SubversionPlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = tr("Revert repository");
    if (QMessageBox::warning(ICore::dialogParent(), title,
                             tr("Revert all pending changes to the repository?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    // NoteL: Svn "revert ." doesn not work.
    QStringList args;
    args << QLatin1String("revert");
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args << QLatin1String("--recursive") << state.topLevel();
    const SubversionResponse revertResponse
            = runSvn(state.topLevel(), args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    if (revertResponse.error)
        QMessageBox::warning(ICore::dialogParent(), title,
                             tr("Revert failed: %1").arg(revertResponse.message), QMessageBox::Ok);
    else
        subVersionControl()->emitRepositoryChanged(state.topLevel());
}

void SubversionPlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QStringList args(QLatin1String("diff"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args.push_back(SubversionClient::escapeFile(state.relativeCurrentFile()));

    const SubversionResponse diffResponse
            = runSvn(state.currentFileTopLevel(), args, m_client->vcsTimeoutS(), 0);
    if (diffResponse.error)
        return;

    if (diffResponse.stdOut.isEmpty())
        return;
    if (QMessageBox::warning(ICore::dialogParent(), QLatin1String("svn revert"),
                             tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;


    FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("revert");
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args << SubversionClient::escapeFile(state.relativeCurrentFile());

    const SubversionResponse revertResponse
            = runSvn(state.currentFileTopLevel(), args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);

    if (!revertResponse.error)
        subVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void SubversionPlugin::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    m_client->diff(state.currentProjectTopLevel(),
                   relativeProject.isEmpty() ? QStringList() : QStringList(relativeProject),
                   QStringList());
}

void SubversionPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                   QStringList());
}

void SubversionPlugin::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void SubversionPlugin::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

void SubversionPlugin::startCommitProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectPath());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void SubversionPlugin::startCommit(const QString &workingDir, const QStringList &files)
{
    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    QStringList args(QLatin1String("status"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args += SubversionClient::escapeFiles(files);

    const SubversionResponse response
            = runSvn(workingDir, args, m_client->vcsTimeoutS(), 0);
    if (response.error)
        return;

    // Get list of added/modified/deleted files
    const StatusList statusOutput = parseStatusOutput(response.stdOut);
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
    SubversionSubmitEditor *editor = openSubversionSubmitEditor(m_commitMessageFileName);
    QTC_ASSERT(editor, return);
    editor->setStatusList(statusOutput);
}

void SubversionPlugin::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void SubversionPlugin::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void SubversionPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel(), QStringList(), QStringList());
}

void SubversionPlugin::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnStatus(state.topLevel());
}

void SubversionPlugin::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnUpdate(state.topLevel());
}

void SubversionPlugin::svnStatus(const QString &workingDir, const QString &relativePath)
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QStringList args(QLatin1String("status"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    if (!relativePath.isEmpty())
        args.append(SubversionClient::escapeFile(relativePath));
    VcsOutputWindow::setRepository(workingDir);
    runSvn(workingDir, args, m_client->vcsTimeoutS(),
           VcsCommand::ShowStdOut | VcsCommand::ShowSuccessMessage);
    VcsOutputWindow::clearRepository();
}

void SubversionPlugin::filelog(const QString &workingDir,
                               const QString &file,
                               bool enableAnnotationContextMenu)
{
    m_client->log(workingDir, QStringList(file), QStringList(), enableAnnotationContextMenu);
}

void SubversionPlugin::updateProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnUpdate(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPlugin::svnUpdate(const QString &workingDir, const QString &relativePath)
{
    QStringList args(QLatin1String("update"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args.push_back(QLatin1String(Constants::NON_INTERACTIVE_OPTION));
    if (!relativePath.isEmpty())
        args.append(relativePath);
    const SubversionResponse response
            = runSvn(workingDir, args, 10 * m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    if (!response.error)
        subVersionControl()->emitRepositoryChanged(workingDir);
}

void SubversionPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::vcsAnnotate(const QString &workingDir, const QString &file,
                                   const QString &revision /* = QString() */,
                                   int lineNumber /* = -1 */)
{
    const QString source = VcsBaseEditor::getSource(workingDir, file);
    QTextCodec *codec = VcsBaseEditor::getCodec(source);

    QStringList args(QLatin1String("annotate"));
    const VcsBaseClientSettings &s = client()->settings();
    args << SubversionClient::addAuthenticationOptions(s);
    if (s.boolValue(SubversionSettings::spaceIgnorantAnnotationKey))
        args << QLatin1String("-x") << QLatin1String("-uw");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args.push_back(QLatin1String("-v"));
    args.append(QDir::toNativeSeparators(SubversionClient::escapeFile(file)));

    const SubversionResponse response
            = runSvn(workingDir, args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ForceCLocale, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber <= 0)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(source);
    // Determine id
    const QStringList files = QStringList(file);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, revision);
    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, AnnotateOutput, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void SubversionPlugin::projectStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnStatus(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPlugin::describe(const QString &source, const QString &changeNr)
{
    // To describe a complete change, find the top level and then do
    //svn diff -r 472958:472959 <top level>
    const QFileInfo fi(source);
    QString topLevel;
    const bool manages = managesDirectory(fi.isDir() ? source : fi.absolutePath(), &topLevel);
    if (!manages || topLevel.isEmpty())
        return;
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << source << topLevel << changeNr;
    // Number must be >= 1
    bool ok;

    const int number = changeNr.toInt(&ok);
    if (!ok || number < 1)
        return;

    const QString title = QString::fromLatin1("svn describe %1#%2").arg(fi.fileName(), changeNr);

    m_client->describe(topLevel, number, title);
}

void SubversionPlugin::slotDescribe()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QInputDialog inputDialog(ICore::dialogParent());
    inputDialog.setWindowFlags(inputDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    inputDialog.setInputMode(QInputDialog::IntInput);
    inputDialog.setIntRange(1, INT_MAX);
    inputDialog.setWindowTitle(tr("Describe"));
    inputDialog.setLabelText(tr("Revision number:"));
    if (inputDialog.exec() != QDialog::Accepted)
        return;

    const int revision = inputDialog.intValue();
    describe(state.topLevel(), QString::number(revision));
}

void SubversionPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

SubversionResponse SubversionPlugin::runSvn(const QString &workingDir,
                                            const QStringList &arguments,
                                            int timeOutS, unsigned flags,
                                            QTextCodec *outputCodec) const
{
    SubversionResponse response;
    if (client()->vcsBinary().isEmpty()) {
        response.error = true;
        response.message =tr("No subversion executable specified.");
        return response;
    }

    const SynchronousProcessResponse sp_resp
            = client()->vcsFullySynchronousExec(workingDir, arguments, flags, timeOutS, outputCodec);

    response.error = sp_resp.result != SynchronousProcessResponse::Finished;
    if (response.error)
        response.message = sp_resp.exitMessage(client()->vcsBinary().toString(), timeOutS);
    response.stdErr = sp_resp.stdErr();
    response.stdOut = sp_resp.stdOut();
    return response;
}

IEditor *SubversionPlugin::showOutputInEditor(const QString &title, const QString &output,
                                                     int editorType, const QString &source,
                                                     QTextCodec *codec)
{
    const VcsBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const Id id = params->id;
    if (Subversion::Constants::debug)
        qDebug() << "SubversionPlugin::showOutputInEditor" << title << id.name()
                 <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    SubversionEditorWidget *e = qobject_cast<SubversionEditorWidget*>(editor->widget());
    if (!e)
        return 0;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested, this, &SubversionPlugin::vcsAnnotate);
    e->setForceReadOnly(true);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

SubversionPlugin *SubversionPlugin::instance()
{
    QTC_ASSERT(m_subversionPluginInstance, return m_subversionPluginInstance);
    return m_subversionPluginInstance;
}

QString SubversionPlugin::monitorFile(const QString &repository) const
{
    QTC_ASSERT(!repository.isEmpty(), return QString());
    QDir repoDir(repository);
    foreach (const QString &svnDir, m_svnDirectories) {
        if (repoDir.exists(svnDir)) {
            QFileInfo fi(repoDir.absoluteFilePath(svnDir + QLatin1String("/wc.db")));
            if (fi.exists() && fi.isFile())
                return fi.absoluteFilePath();
        }
    }
    return QString();
}

QString SubversionPlugin::synchronousTopic(const QString &repository) const
{
    return m_client->synchronousTopic(repository);
}

bool SubversionPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));
    QStringList args;
    args << QLatin1String("add")
         << SubversionClient::addAuthenticationOptions(client()->settings())
         << QLatin1String("--parents") << file;
    const SubversionResponse response
            = runSvn(workingDir, args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    return !response.error;
}

bool SubversionPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));

    QStringList args;
    args << QLatin1String("delete");
    args << SubversionClient::addAuthenticationOptions(client()->settings())
         << QLatin1String("--force") << file;

    const SubversionResponse response
            = runSvn(workingDir, args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut);
    return !response.error;
}

bool SubversionPlugin::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    QStringList args(QLatin1String("move"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args << QDir::toNativeSeparators(SubversionClient::escapeFile(from))
         << QDir::toNativeSeparators(SubversionClient::escapeFile(to));
    const SubversionResponse response
            = runSvn(workingDir, args, m_client->vcsTimeoutS(),
                     VcsCommand::SshPasswordPrompt | VcsCommand::ShowStdOut
                     | VcsCommand::FullySynchronously);
    return !response.error;
}

bool SubversionPlugin::vcsCheckout(const QString &directory, const QByteArray &url)
{
    QUrl tempUrl = QUrl::fromEncoded(url);
    QString username = tempUrl.userName();
    QString password = tempUrl.password();
    QStringList args = QStringList(QLatin1String("checkout"));
    args << QLatin1String(Constants::NON_INTERACTIVE_OPTION) ;

    if (!username.isEmpty()) {
        // If url contains username and password we have to use separate username and password
        // arguments instead of passing those in the url. Otherwise the subversion 'non-interactive'
        // authentication will always fail (if the username and password data are not stored locally),
        // if for example we are logging into a new host for the first time using svn. There seems to
        // be a bug in subversion, so this might get fixed in the future.
        tempUrl.setUserInfo(QString());
        args << QLatin1String("--username") << username;
        if (!password.isEmpty())
            args << QLatin1String("--password") << password;
    }

    args << QLatin1String(tempUrl.toEncoded()) << directory;

    const SubversionResponse response
            = runSvn(directory, args, 10 * m_client->vcsTimeoutS(), VcsCommand::SshPasswordPrompt);
    return !response.error;

}

bool SubversionPlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    const QDir dir(directory);
    if (topLevel)
        topLevel->clear();

    /* Subversion >= 1.7 has ".svn" directory in the root of the working copy. Check for
     * furthest parent containing ".svn/wc.db". Need to check for furthest parent as closer
     * parents may be svn:externals. */
    QDir parentDir = dir;
    while (!parentDir.isRoot()) {
        if (checkSVNSubDir(parentDir)) {
            if (topLevel)
                *topLevel = parentDir.absolutePath();
            return true;
        }
        if (!parentDir.cdUp())
            break;
    }

    return false;
}

bool SubversionPlugin::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("status");
    args << SubversionClient::addAuthenticationOptions(client()->settings())
         << QDir::toNativeSeparators(SubversionClient::escapeFile(fileName));
    SubversionResponse response
            = runSvn(workingDirectory, args, m_client->vcsTimeoutS(), 0);
    return response.stdOut.isEmpty() || response.stdOut.at(0) != QLatin1Char('?');
}

// Check whether SVN management subdirs exist.
bool SubversionPlugin::checkSVNSubDir(const QDir &directory) const
{
    const int dirCount = m_svnDirectories.size();
    for (int i = 0; i < dirCount; i++) {
        const QDir svnDir(directory.absoluteFilePath(m_svnDirectories.at(i)));
        if (!svnDir.exists())
            continue;
        if (!svnDir.exists(QLatin1String("wc.db")))
            continue;
        return true;
    }
    return false;
}

SubversionControl *SubversionPlugin::subVersionControl() const
{
    return static_cast<SubversionControl *>(versionControl());
}

#ifdef WITH_TESTS
void SubversionPlugin::testLogResolving()
{
    QByteArray data(
                "------------------------------------------------------------------------\n"
                "r1439551 | philip | 2013-01-28 20:19:55 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_file_move): Resolve conflict, adjust expectations,\n"
                "   remove XFail.\n"
                "\n"
                "------------------------------------------------------------------------\n"
                "r1439540 | philip | 2013-01-28 20:06:36 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_edited_leaf_del): Do non-recursive resolution, adjust\n"
                "   expectations, remove XFail.\n"
                "\n"
                );
    VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "r1439551", "r1439540");
}
#endif

} // Internal
} // Subversion

