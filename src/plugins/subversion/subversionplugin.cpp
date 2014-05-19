/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "subversionplugin.h"

#include "settingspage.h"
#include "subversioneditor.h"

#include "subversionsubmiteditor.h"
#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversioncontrol.h"
#include "checkoutwizard.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>

#include <coreplugin/locator/commandlocator.h>

#include <utils/qtcassert.h>

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

static const char CMD_ID_SUBVERSION_MENU[]    = "Subversion.Menu";
static const char CMD_ID_ADD[]                = "Subversion.Add";
static const char CMD_ID_DELETE_FILE[]        = "Subversion.Delete";
static const char CMD_ID_REVERT[]             = "Subversion.Revert";
static const char CMD_ID_DIFF_PROJECT[]       = "Subversion.DiffAll";
static const char CMD_ID_DIFF_CURRENT[]       = "Subversion.DiffCurrent";
static const char CMD_ID_COMMIT_ALL[]         = "Subversion.CommitAll";
static const char CMD_ID_REVERT_ALL[]         = "Subversion.RevertAll";
static const char CMD_ID_COMMIT_CURRENT[]     = "Subversion.CommitCurrent";
static const char CMD_ID_FILELOG_CURRENT[]    = "Subversion.FilelogCurrent";
static const char CMD_ID_ANNOTATE_CURRENT[]   = "Subversion.AnnotateCurrent";
static const char CMD_ID_STATUS[]             = "Subversion.Status";
static const char CMD_ID_PROJECTLOG[]         = "Subversion.ProjectLog";
static const char CMD_ID_REPOSITORYLOG[]      = "Subversion.RepositoryLog";
static const char CMD_ID_REPOSITORYUPDATE[]   = "Subversion.RepositoryUpdate";
static const char CMD_ID_REPOSITORYDIFF[]     = "Subversion.RepositoryDiff";
static const char CMD_ID_REPOSITORYSTATUS[]   = "Subversion.RepositoryStatus";
static const char CMD_ID_UPDATE[]             = "Subversion.Update";
static const char CMD_ID_COMMIT_PROJECT[]     = "Subversion.CommitProject";
static const char CMD_ID_DESCRIBE[]           = "Subversion.Describe";

static const char nonInteractiveOptionC[] = "--non-interactive";



static const VcsBaseEditorParameters editorParameters[] = {
{
    LogOutput,
    "Subversion File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "Subversion File Log Editor"),   // display_name
    "Subversion File Log Editor",   // context
    "text/vnd.qtcreator.svn.log"},
{    AnnotateOutput,
    "Subversion Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "Subversion Annotation Editor"),   // display_name
    "Subversion Annotation Editor",  // context
    "text/vnd.qtcreator.svn.annotation"},
{   DiffOutput,
    "Subversion Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "Subversion Diff Editor"),   // display_name
    "Subversion Diff Editor",  // context
    "text/x-patch"}
};

// Utility to find a parameter set by type
static const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return VcsBaseEditorWidget::findType(editorParameters, sizeof(editorParameters)/sizeof(editorParameters[0]), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

// Parse "svn status" output for added/modified/deleted files
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
            const QChar state = line.at(0);
            if (state == QLatin1Char('A') || state == QLatin1Char('D') || state == QLatin1Char('M')) {
                const QString fileName = line.mid(7); // Column 8 starting from svn 1.6
                changeSet.push_back(SubversionSubmitEditor::StatusFilePair(QString(state), fileName.trimmed()));
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
    m_svnDirectories(svnDirectories()),
    m_commandLocator(0),
    m_addAction(0),
    m_deleteAction(0),
    m_revertAction(0),
    m_diffProjectAction(0),
    m_diffCurrentAction(0),
    m_logProjectAction(0),
    m_logRepositoryAction(0),
    m_commitAllAction(0),
    m_revertRepositoryAction(0),
    m_diffRepositoryAction(0),
    m_statusRepositoryAction(0),
    m_updateRepositoryAction(0),
    m_commitCurrentAction(0),
    m_filelogCurrentAction(0),
    m_annotateCurrentAction(0),
    m_statusProjectAction(0),
    m_updateProjectAction(0),
    m_commitProjectAction(0),
    m_describeAction(0),
    m_submitCurrentLogAction(0),
    m_submitDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false)
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
    Subversion::Constants::SUBVERSION_SUBMIT_MIMETYPE,
    Subversion::Constants::SUBVERSIONCOMMITEDITOR_ID,
    Subversion::Constants::SUBVERSIONCOMMITEDITOR_DISPLAY_NAME,
    Subversion::Constants::SUBVERSIONCOMMITEDITOR,
    VcsBaseSubmitEditorParameters::DiffFiles
};

bool SubversionPlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    typedef VcsSubmitEditorFactory<SubversionSubmitEditor> SubversionSubmitEditorFactory;
    typedef VcsEditorFactory<SubversionEditor> SubversionEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    initializeVcs(new SubversionControl(this));

    m_subversionPluginInstance = this;

    if (!MimeDatabase::addMimeTypes(QLatin1String(":/trolltech.subversion/Subversion.mimetypes.xml"), errorMessage))
        return false;

    m_settings.readSettings(ICore::settings());
    m_client = new SubversionClient(&m_settings);

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new SubversionSubmitEditorFactory(&submitParameters));

    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new SubversionEditorFactory(editorParameters + i, this, describeSlot));

    addAutoReleasedObject(new CheckoutWizardFactory);

    const QString prefix = QLatin1String("svn");
    m_commandLocator = new CommandLocator("Subversion", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *subversionMenu = ActionManager::createMenu(Id(CMD_ID_SUBVERSION_MENU));
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    toolsContainer->addMenu(subversionMenu);
    m_menuAction = subversionMenu->menu()->menuAction();
    Context globalcontext(C_GLOBAL);
    Core::Command *command;

    m_diffCurrentAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+D") : tr("Alt+S,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this,
        SLOT(filelogCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(globalcontext);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+A") : tr("Alt+S,Alt+A")));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+S,Meta+C") : tr("Alt+S,Alt+C")));
    connect(m_commitCurrentAction, SIGNAL(triggered()), this, SLOT(startCommitCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new ParameterAction(tr("Revert..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(globalcontext);

    m_diffProjectAction = new ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffProject()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(projectStatus()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, globalcontext);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateProject()));
    command->setAttribute(Core::Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new ParameterAction(tr("Commit Project"), tr("Commit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitProjectAction, CMD_ID_COMMIT_PROJECT, globalcontext);
    connect(m_commitProjectAction, SIGNAL(triggered()), this, SLOT(startCommitProject()));
    command->setAttribute(Core::Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(globalcontext);

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, globalcontext);
    connect(m_diffRepositoryAction, SIGNAL(triggered()), this, SLOT(diffRepository()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, globalcontext);
    connect(m_statusRepositoryAction, SIGNAL(triggered()), this, SLOT(statusRepository()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Log Repository"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(tr("Update Repository"), this);
    command = ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, globalcontext);
    connect(m_updateRepositoryAction, SIGNAL(triggered()), this, SLOT(updateRepository()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        globalcontext);
    connect(m_commitAllAction, SIGNAL(triggered()), this, SLOT(startCommitAll()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, globalcontext);
    connect(m_describeAction, SIGNAL(triggered()), this, SLOT(slotDescribe()));
    subversionMenu->addAction(command);

    m_revertRepositoryAction = new QAction(tr("Revert Repository..."), this);
    command = ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
        globalcontext);
    connect(m_revertRepositoryAction, SIGNAL(triggered()), this, SLOT(revertAll()));
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Context svncommitcontext(Constants::SUBVERSIONCOMMITEDITOR);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ActionManager::registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, svncommitcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_submitDiffAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = ActionManager::registerAction(m_submitDiffAction , Constants::DIFF_SELECTED, svncommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = ActionManager::registerAction(m_submitUndoAction, Core::Constants::UNDO, svncommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = ActionManager::registerAction(m_submitRedoAction, Core::Constants::REDO, svncommitcontext);

    return true;
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
    const QFileInfo editorFile(editorDocument->filePath());
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    SubversionSettings newSettings = m_settings;
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
    setSettings(newSettings); // in case someone turned prompting off
    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        closeEditor = DocumentManager::saveDocument(editorDocument);
        if (closeEditor) {
            VcsBase::Command *commitCmd = m_client->createCommitCmd(m_commitRepository,
                                                                    fileList,
                                                                    m_commitMessageFileName);
            QObject::connect(commitCmd, SIGNAL(success(QVariant)),
                             this, SLOT(cleanCommitMessageFile()));
            commitCmd->execute();
        }
    }
    return closeEditor;
}

void SubversionPlugin::diffCommitFiles(const QStringList &files)
{
    m_client->diff(m_commitRepository, files);
}

SubversionSubmitEditor *SubversionPlugin::openSubversionSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(fileName, Constants::SUBVERSIONCOMMITEDITOR_ID);
    SubversionSubmitEditor *submitEditor = qobject_cast<SubversionSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    setSubmitEditor(submitEditor);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCommitFiles(QStringList)));
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
    args << QLatin1String("revert") << QLatin1String("--recursive") << state.topLevel();
    const SubversionResponse revertResponse =
            runSvn(state.topLevel(), args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
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
    args.push_back(state.relativeCurrentFile());

    const SubversionResponse diffResponse =
            runSvn(state.currentFileTopLevel(), args, m_settings.timeOutMs(), 0);
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
    args << QLatin1String("revert") << state.relativeCurrentFile();

    const SubversionResponse revertResponse =
            runSvn(state.currentFileTopLevel(), args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);

    if (!revertResponse.error)
        subVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void SubversionPlugin::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    m_client->diff(state.currentProjectTopLevel(),
                      relativeProject.isEmpty() ? QStringList() : QStringList(relativeProject));
}

void SubversionPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
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
        VcsBaseOutputWindow::instance()->appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    QStringList args(QLatin1String("status"));
    args += files;

    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(), 0);
    if (response.error)
        return;

    // Get list of added/modified/deleted files
    const StatusList statusOutput = parseStatusOutput(response.stdOut);
    if (statusOutput.empty()) {
        VcsBaseOutputWindow::instance()->appendWarning(tr("There are no modified files."));
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
        VcsBaseOutputWindow::instance()->appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    // Create a submit editor and set file list
    SubversionSubmitEditor *editor = openSubversionSubmitEditor(m_commitMessageFileName);
    QTC_ASSERT(editor, return);
    editor->setStatusList(statusOutput);
}

bool SubversionPlugin::commit(const QString &messageFile,
                              const QStringList &subVersionFileList)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << subVersionFileList;
    // Transform the status list which is sth
    // "[ADM]<blanks>file" into an args list. The files of the status log
    // can be relative or absolute depending on where the command was run.
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String(nonInteractiveOptionC) << QLatin1String("--file") << messageFile;
    args.append(subVersionFileList);
    const SubversionResponse response =
            runSvn(m_commitRepository, args, 10 * m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return !response.error ;
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
    m_client->diff(state.topLevel(), QStringList());
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
    if (!relativePath.isEmpty())
        args.append(relativePath);
    VcsBaseOutputWindow *outwin = VcsBaseOutputWindow::instance();
    outwin->setRepository(workingDir);
    runSvn(workingDir, args, m_settings.timeOutMs(),
           ShowStdOutInLogWindow|ShowSuccessMessage);
    outwin->clearRepository();
}

void SubversionPlugin::filelog(const QString &workingDir,
                               const QString &file,
                               bool enableAnnotationContextMenu)
{
    // no need for temp file
    QStringList args(QLatin1String("log"));
    if (m_settings.intValue(SubversionSettings::logCountKey) > 0) {
        args << QLatin1String("-l")
             << QString::number(m_settings.intValue(SubversionSettings::logCountKey));
    }
    if (!file.isEmpty())
        args.append(QDir::toNativeSeparators(file));

    // subversion stores log in UTF-8 and returns it back in user system locale.
    // So we do not need to encode it.
    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(),
                   SshPasswordPrompt, 0/*codec*/);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file

    const QString id = VcsBaseEditorWidget::getTitleId(workingDir, QStringList(file));
    const QString tag = VcsBaseEditorWidget::editorTag(LogOutput, workingDir,
                                                                QStringList(file));
    if (IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn log %1").arg(id);
        const QString source = VcsBaseEditorWidget::getSource(workingDir, file);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, LogOutput, source, /*codec*/0);
        VcsBaseEditorWidget::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBaseEditorWidget::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
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
    args.push_back(QLatin1String(nonInteractiveOptionC));
    if (!relativePath.isEmpty())
        args.append(relativePath);
        const SubversionResponse response =
                runSvn(workingDir, args, 10 * m_settings.timeOutMs(),
                       SshPasswordPrompt|ShowStdOutInLogWindow);
    if (!response.error)
        subVersionControl()->emitRepositoryChanged(workingDir);
}

void SubversionPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::annotateVersion(const QString &workingDirectory,
                                       const QString &file,
                                       const QString &revision,
                                       int lineNr)
{
    vcsAnnotate(workingDirectory, file, revision, lineNr);
}

void SubversionPlugin::vcsAnnotate(const QString &workingDir, const QString &file,
                                const QString &revision /* = QString() */,
                                int lineNumber /* = -1 */)
{
    const QString source = VcsBaseEditorWidget::getSource(workingDir, file);
    QTextCodec *codec = VcsBaseEditorWidget::getCodec(source);

    QStringList args(QLatin1String("annotate"));
    if (m_settings.boolValue(SubversionSettings::spaceIgnorantAnnotationKey))
        args << QLatin1String("-x") << QLatin1String("-uw");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args.push_back(QLatin1String("-v"));
    args.append(QDir::toNativeSeparators(file));

    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ForceCLocale, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber <= 0)
        lineNumber = VcsBaseEditorWidget::lineNumberOfCurrentEditor(source);
    // Determine id
    const QStringList files = QStringList(file);
    const QString id = VcsBaseEditorWidget::getTitleId(workingDir, files, revision);
    const QString tag = VcsBaseEditorWidget::editorTag(AnnotateOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        VcsBaseEditorWidget::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, AnnotateOutput, source, codec);
        VcsBaseEditorWidget::tagEditor(newEditor, tag);
        VcsBaseEditorWidget::gotoLineOfEditor(newEditor, lineNumber);
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
    // Number must be > 1
    bool ok;
    const int number = changeNr.toInt(&ok);
    if (!ok || number < 2)
        return;
    // Run log to obtain message (local utf8)
    QString description;
    QStringList args(QLatin1String("log"));
    args.push_back(QLatin1String("-r"));
    args.push_back(changeNr);
    const SubversionResponse logResponse =
            runSvn(topLevel, args, m_settings.timeOutMs(), SshPasswordPrompt);
    if (logResponse.error)
        return;
    description = logResponse.stdOut;

    // Run diff (encoding via source codec)
    args.clear();
    args.push_back(QLatin1String("diff"));
    args.push_back(QLatin1String("-r"));
    QString diffArg;
    QTextStream(&diffArg) << (number - 1) << ':' << number;
    args.push_back(diffArg);

    QTextCodec *codec = VcsBaseEditorWidget::getCodec(source);
    const SubversionResponse response =
            runSvn(topLevel, args, m_settings.timeOutMs(),
                   SshPasswordPrompt, codec);
    if (response.error)
        return;
    description += response.stdOut;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditorWidget::editorTag(DiffOutput, source, QStringList(), changeNr);
    if (IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->document()->setContents(description.toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn describe %1#%2").arg(fi.fileName(), changeNr);
        IEditor *newEditor = showOutputInEditor(title, description, DiffOutput, source, codec);
        VcsBaseEditorWidget::tagEditor(newEditor, tag);
    }
}

void SubversionPlugin::slotDescribe()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QInputDialog inputDialog(ICore::dialogParent());
    inputDialog.setWindowFlags(inputDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    inputDialog.setInputMode(QInputDialog::IntInput);
    inputDialog.setIntRange(2, INT_MAX);
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
    EditorManager::closeEditor(EditorManager::currentEditor());
}

SubversionResponse
        SubversionPlugin::runSvn(const QString &workingDir,
                                 const QStringList &arguments,
                                 int timeOut,
                                 unsigned flags,
                                 QTextCodec *outputCodec) const
{
    const bool hasAuth = m_settings.hasAuthentication();
    return runSvn(workingDir,
                  hasAuth ? m_settings.stringValue(SubversionSettings::userKey) : QString(),
                  hasAuth ? m_settings.stringValue(SubversionSettings::passwordKey) : QString(),
                  arguments, timeOut, flags, outputCodec);
}

SubversionResponse SubversionPlugin::runSvn(const QString &workingDir,
                          const QString &userName, const QString &password,
                          const QStringList &arguments, int timeOut,
                          unsigned flags, QTextCodec *outputCodec) const
{
    const QString executable = m_settings.binaryPath();
    SubversionResponse response;
    if (executable.isEmpty()) {
        response.error = true;
        response.message =tr("No subversion executable specified!");
        return response;
    }

    const QStringList completeArguments = SubversionClient::addAuthenticationOptions(arguments, userName, password);
    const SynchronousProcessResponse sp_resp =
            VcsBasePlugin::runVcs(workingDir, executable, completeArguments, timeOut,
                                           flags, outputCodec);

    response.error = sp_resp.result != SynchronousProcessResponse::Finished;
    if (response.error)
        response.message = sp_resp.exitMessage(executable, timeOut);
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
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
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8(),
                                                          (EditorManager::OpenInOtherSplit
                                                           | EditorManager::NoNewSplits));
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)),
            this, SLOT(annotateVersion(QString,QString,QString,int)));
    SubversionEditor *e = qobject_cast<SubversionEditor*>(editor->widget());
    if (!e)
        return 0;
    e->setForceReadOnly(true);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->baseTextDocument()->setSuggestedFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

SubversionSettings SubversionPlugin::settings() const
{
    return m_settings;
}

void SubversionPlugin::setSettings(const SubversionSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        m_settings.writeSettings(ICore::settings());
        subVersionControl()->emitConfigurationChanged();
    }
}

SubversionPlugin *SubversionPlugin::instance()
{
    QTC_ASSERT(m_subversionPluginInstance, return m_subversionPluginInstance);
    return m_subversionPluginInstance;
}

bool SubversionPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(rawFileName);
    QStringList args;
    args << QLatin1String("add") << QLatin1String("--parents") << file;
    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return !response.error;
}

bool SubversionPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(rawFileName);

    QStringList args(QLatin1String("delete"));
    args.push_back(file);

    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return !response.error;
}

bool SubversionPlugin::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    QStringList args(QLatin1String("move"));
    args << QDir::toNativeSeparators(from) << QDir::toNativeSeparators(to);
    const SubversionResponse response =
            runSvn(workingDir, args, m_settings.timeOutMs(),
                   SshPasswordPrompt|ShowStdOutInLogWindow|FullySynchronously);
    return !response.error;
}

bool SubversionPlugin::vcsCheckout(const QString &directory, const QByteArray &url)
{
    QUrl tempUrl = QUrl::fromEncoded(url);
    QString username = tempUrl.userName();
    QString password = tempUrl.password();
    QStringList args = QStringList(QLatin1String("checkout"));
    args << QLatin1String(nonInteractiveOptionC) ;

    if (!username.isEmpty() && !password.isEmpty()) {
        // If url contains username and password we have to use separate username and password
        // arguments instead of passing those in the url. Otherwise the subversion 'non-interactive'
        // authentication will always fail (if the username and password data are not stored locally),
        // if for example we are logging into a new host for the first time using svn. There seems to
        // be a bug in subversion, so this might get fixed in the future.
        tempUrl.setUserInfo(QString());
        args << QLatin1String(tempUrl.toEncoded()) << directory;
        const SubversionResponse response = runSvn(directory, username, password, args,
                                                   10 * m_settings.timeOutMs(),
                                                   VcsBasePlugin::SshPasswordPrompt);
        return !response.error;
    } else {
        args << QLatin1String(url) << directory;
        const SubversionResponse response = runSvn(directory, args, 10 * m_settings.timeOutMs(),
                                                   VcsBasePlugin::SshPasswordPrompt);
        return !response.error;
    }
}

QString SubversionPlugin::vcsGetRepositoryURL(const QString &directory)
{
    QXmlStreamReader xml;
    QStringList args = QStringList(QLatin1String("info"));
    args << QLatin1String("--xml");

    const SubversionResponse response = runSvn(directory, args, 10 * m_settings.timeOutMs(), SuppressCommandLogging);
    xml.addData(response.stdOut);

    bool repo = false;
    bool root = false;

    while (!xml.atEnd() && !xml.hasError()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartDocument:
            break;
        case QXmlStreamReader::StartElement:
            if (xml.name() == QLatin1String("repository"))
                repo = true;
            else if (repo && xml.name() == QLatin1String("root"))
                root = true;
            break;
        case QXmlStreamReader::EndElement:
            if (xml.name() == QLatin1String("repository"))
                repo = false;
            else if (repo && xml.name() == QLatin1String("root"))
                root = false;
            break;
        case QXmlStreamReader::Characters:
            if (repo && root)
                return xml.text().toString();
            break;
        default:
            break;
        }
    }
    return QString();
}

bool SubversionPlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    const QDir dir(directory);
    if (!dir.exists())
        return false;

    if (topLevel)
        topLevel->clear();

    /* Subversion >= 1.7 has ".svn" directory in the root of the working copy. Check for
     * furthest parent containing ".svn/wc.db". Need to check for furthest parent as closer
     * parents may be svn:externals. */
    QDir parentDir = dir;
    while (!parentDir.isRoot() && parentDir.cdUp()) {
        if (checkSVNSubDir(parentDir, QLatin1String("wc.db"))) {
            if (topLevel)
                *topLevel = parentDir.absolutePath();
            return true;
        }
    }

    /* Subversion < 1.7 has ".svn" directory in each directory
     * it manages. The top level is the first directory
     * under the directory that does not have a  ".svn".*/
    if (!checkSVNSubDir(dir))
        return false;

     if (topLevel) {
         QDir lastDirectory = dir;
         for (parentDir = lastDirectory;
              !parentDir.isRoot() && parentDir.cdUp();
              lastDirectory = parentDir) {
             if (!checkSVNSubDir(parentDir)) {
                 *topLevel = lastDirectory.absolutePath();
                 break;
            }
        }
    }
    return true;
}

bool SubversionPlugin::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("status") << fileName;
    SubversionResponse response =
            runSvn(workingDirectory, args, m_settings.timeOutMs(), 0);
    return response.stdOut.isEmpty() || response.stdOut.at(0) != QLatin1Char('?');
}

// Check whether SVN management subdirs exist.
bool SubversionPlugin::checkSVNSubDir(const QDir &directory, const QString &fileName) const
{
    const int dirCount = m_svnDirectories.size();
    for (int i = 0; i < dirCount; i++) {
        const QString svnDir = directory.absoluteFilePath(m_svnDirectories.at(i));
        if (!QFileInfo(svnDir).isDir())
            continue;
        if (!fileName.isEmpty() && !QDir(svnDir).exists(fileName))
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
void SubversionPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "@@ -0,0 +125 @@\n\n")
        << QByteArray("src/plugins/subversion/subversioneditor.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -1,125 +0,0 @@\n\n")
        << QByteArray("src/plugins/subversion/subversioneditor.cpp");
    QTest::newRow("Normal") << QByteArray(
            "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -120,7 +120,7 @@\n\n")
        << QByteArray("src/plugins/subversion/subversioneditor.cpp");
}

void SubversionPlugin::testDiffFileResolving()
{
    SubversionEditor editor(editorParameters + 2, 0);
    editor.testDiffFileResolving();
}

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
    SubversionEditor editor(editorParameters, 0);
    editor.testLogResolving(data, "r1439551", "r1439540");
}
#endif

} // Internal
} // Subversion

Q_EXPORT_PLUGIN(Subversion::Internal::SubversionPlugin)

