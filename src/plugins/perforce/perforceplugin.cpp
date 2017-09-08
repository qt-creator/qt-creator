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

#include "perforceplugin.h"

#include "changenumberdialog.h"
#include "pendingchangesdialog.h"
#include "perforceeditor.h"
#include "perforcesubmiteditor.h"
#include "perforceversioncontrol.h"
#include "perforcechecker.h"
#include "settingspage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/locator/commandlocator.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/temporarydirectory.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseeditorconfig.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextCodec>
#include <QtPlugin>

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Perforce {
namespace Internal {

const char SUBMIT_CURRENT[] = "Perforce.SubmitCurrentLog";
const char DIFF_SELECTED[] = "Perforce.DiffSelectedFilesInLog";
const char SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.p4.submit";

const char PERFORCE_CONTEXT[] = "Perforce Context";
const char PERFORCE_SUBMIT_EDITOR_ID[] = "Perforce.SubmitEditor";
const char PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce.SubmitEditor");

const char PERFORCE_LOG_EDITOR_ID[] = "Perforce.LogEditor";
const char PERFORCE_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Log Editor");

const char PERFORCE_DIFF_EDITOR_ID[] = "Perforce.DiffEditor";
const char PERFORCE_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Diff Editor");

const char PERFORCE_ANNOTATION_EDITOR_ID[] = "Perforce.AnnotationEditor";
const char PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Annotation Editor");

const VcsBaseEditorParameters editorParameters[] = {
{
    LogOutput,
    PERFORCE_LOG_EDITOR_ID,
    PERFORCE_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.p4.log"},
{    AnnotateOutput,
    PERFORCE_ANNOTATION_EDITOR_ID,
    PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.p4.annotation"},
{   DiffOutput,
    PERFORCE_DIFF_EDITOR_ID,
    PERFORCE_DIFF_EDITOR_DISPLAY_NAME,
    "text/x-patch"}
};

// Utility to find a parameter set by type
static inline const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return VcsBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(editorParameters[0]), et);
}

// Ensure adding "..." to relative paths which is p4's convention
// for the current directory
static inline QString perforceRelativeFileArguments(const QString &args)
{
    if (args.isEmpty())
        return QLatin1String("...");
    return args + QLatin1String("/...");
}

static inline QStringList perforceRelativeProjectDirectory(const VcsBasePluginState &s)
{
    return QStringList(perforceRelativeFileArguments(s.relativeCurrentProject()));
}

// Clean user setting off diff-binary for 'p4 resolve' and 'p4 diff'.
static inline QProcessEnvironment overrideDiffEnvironmentVariable()
{
    QProcessEnvironment rc = QProcessEnvironment::systemEnvironment();
    rc.remove(QLatin1String("P4DIFF"));
    return rc;
}

const char CMD_ID_PERFORCE_MENU[] = "Perforce.Menu";
const char CMD_ID_EDIT[] = "Perforce.Edit";
const char CMD_ID_ADD[] = "Perforce.Add";
const char CMD_ID_DELETE_FILE[] = "Perforce.Delete";
const char CMD_ID_OPENED[] = "Perforce.Opened";
const char CMD_ID_PROJECTLOG[] = "Perforce.ProjectLog";
const char CMD_ID_REPOSITORYLOG[] = "Perforce.RepositoryLog";
const char CMD_ID_REVERT[] = "Perforce.Revert";
const char CMD_ID_DIFF_CURRENT[] = "Perforce.DiffCurrent";
const char CMD_ID_DIFF_PROJECT[] = "Perforce.DiffProject";
const char CMD_ID_UPDATE_PROJECT[] = "Perforce.UpdateProject";
const char CMD_ID_REVERT_PROJECT[] = "Perforce.RevertProject";
const char CMD_ID_REVERT_UNCHANGED_PROJECT[] = "Perforce.RevertUnchangedProject";
const char CMD_ID_DIFF_ALL[] = "Perforce.DiffAll";
const char CMD_ID_SUBMIT[] = "Perforce.Submit";
const char CMD_ID_PENDING_CHANGES[] = "Perforce.PendingChanges";
const char CMD_ID_DESCRIBE[] = "Perforce.Describe";
const char CMD_ID_ANNOTATE_CURRENT[] = "Perforce.AnnotateCurrent";
const char CMD_ID_ANNOTATE[] = "Perforce.Annotate";
const char CMD_ID_FILELOG_CURRENT[] = "Perforce.FilelogCurrent";
const char CMD_ID_FILELOG[] = "Perforce.Filelog";
const char CMD_ID_UPDATEALL[] = "Perforce.UpdateAll";

////
// PerforcePlugin
////

PerforceResponse::PerforceResponse() :
    error(true),
    exitCode(-1)
{
}

PerforcePlugin *PerforcePlugin::m_instance = NULL;

static const VcsBaseSubmitEditorParameters submitParameters = {
    SUBMIT_MIMETYPE,
    PERFORCE_SUBMIT_EDITOR_ID,
    PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

bool PerforcePlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Context context(PERFORCE_CONTEXT);

    initializeVcs<PerforceVersionControl>(context, this);

    m_instance = this;

    m_settings.fromSettings(ICore::settings());

    addAutoReleasedObject(new SettingsPage);

    // Editor factories
    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new PerforceSubmitEditor(&submitParameters); }));

    const auto describeFunc = [this](const QString &source, const QString &n) {
        describe(source, n);
    };
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new PerforceEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc));

    const QString prefix = QLatin1String("p4");
    m_commandLocator = new CommandLocator("Perforce", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *perforceContainer = ActionManager::createMenu(CMD_ID_PERFORCE_MENU);
    perforceContainer->menu()->setTitle(tr("&Perforce"));
    mtools->addMenu(perforceContainer);
    m_menuAction = perforceContainer->menu()->menuAction();

    Context perforcesubmitcontext(PERFORCE_SUBMIT_EDITOR_ID);

    Command *command;

    m_diffFileAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFileAction, CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Diff Current File"));
    connect(m_diffFileAction, &QAction::triggered, this, &PerforcePlugin::diffCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction, CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Annotate Current File"));
    connect(m_annotateCurrentAction, &QAction::triggered, this, &PerforcePlugin::annotateCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction, CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+F") : tr("Alt+P,Alt+F")));
    command->setDescription(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, &QAction::triggered, this, &PerforcePlugin::filelogCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_editAction = new ParameterAction(tr("Edit"), tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editAction, CMD_ID_EDIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+E") : tr("Alt+P,Alt+E")));
    command->setDescription(tr("Edit File"));
    connect(m_editAction, &QAction::triggered, this, &PerforcePlugin::openCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+A") : tr("Alt+P,Alt+A")));
    command->setDescription(tr("Add File"));
    connect(m_addAction, &QAction::triggered, this, &PerforcePlugin::addCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Delete File"));
    connect(m_deleteAction, &QAction::triggered, this, &PerforcePlugin::promptToDeleteCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFileAction = new ParameterAction(tr("Revert"), tr("Revert \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFileAction, CMD_ID_REVERT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+R") : tr("Alt+P,Alt+R")));
    command->setDescription(tr("Revert File"));
    connect(m_revertFileAction, &QAction::triggered, this, &PerforcePlugin::revertCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    const QString diffProjectDefaultText = tr("Diff Current Project/Session");
    m_diffProjectAction = new ParameterAction(diffProjectDefaultText, tr("Diff Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+D") : tr("Alt+P,Alt+D")));
    command->setDescription(diffProjectDefaultText);
    connect(m_diffProjectAction, &QAction::triggered, this, &PerforcePlugin::diffCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &PerforcePlugin::logProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_submitProjectAction = new ParameterAction(tr("Submit Project"), tr("Submit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_submitProjectAction, CMD_ID_SUBMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+S") : tr("Alt+P,Alt+S")));
    connect(m_submitProjectAction, &QAction::triggered, this, &PerforcePlugin::startSubmitProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    const QString updateProjectDefaultText = tr("Update Current Project");
    m_updateProjectAction = new ParameterAction(updateProjectDefaultText, tr("Update Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, context);
    command->setDescription(updateProjectDefaultText);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, &QAction::triggered, this, &PerforcePlugin::updateCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertUnchangedAction = new ParameterAction(tr("Revert Unchanged"), tr("Revert Unchanged Files of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertUnchangedAction, CMD_ID_REVERT_UNCHANGED_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertUnchangedAction, &QAction::triggered, this, &PerforcePlugin::revertUnchangedCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertProjectAction = new ParameterAction(tr("Revert Project"), tr("Revert Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertProjectAction, CMD_ID_REVERT_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertProjectAction, &QAction::triggered, this, &PerforcePlugin::revertCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_diffAllAction = new QAction(tr("Diff Opened Files"), this);
    command = ActionManager::registerAction(m_diffAllAction, CMD_ID_DIFF_ALL, context);
    connect(m_diffAllAction, &QAction::triggered, this, &PerforcePlugin::diffAllOpened);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_openedAction = new QAction(tr("Opened"), this);
    command = ActionManager::registerAction(m_openedAction, CMD_ID_OPENED, context);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+O") : tr("Alt+P,Alt+O")));
    connect(m_openedAction, &QAction::triggered, this, &PerforcePlugin::printOpenedFileList);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &PerforcePlugin::logRepository);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = ActionManager::registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, context);
    connect(m_pendingAction, &QAction::triggered, this, &PerforcePlugin::printPendingChanges);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateAllAction = new QAction(tr("Update All"), this);
    command = ActionManager::registerAction(m_updateAllAction, CMD_ID_UPDATEALL, context);
    connect(m_updateAllAction, &QAction::triggered, this, &PerforcePlugin::updateAll);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, context);
    connect(m_describeAction, &QAction::triggered, this, &PerforcePlugin::describeChange);
    perforceContainer->addAction(command);

    m_annotateAction = new QAction(tr("Annotate..."), this);
    command = ActionManager::registerAction(m_annotateAction, CMD_ID_ANNOTATE, context);
    connect(m_annotateAction, &QAction::triggered, this, &PerforcePlugin::annotateFile);
    perforceContainer->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = ActionManager::registerAction(m_filelogAction, CMD_ID_FILELOG, context);
    connect(m_filelogAction, &QAction::triggered, this, &PerforcePlugin::filelogFile);
    perforceContainer->addAction(command);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Submit"), this);
    command = ActionManager::registerAction(m_submitCurrentLogAction, SUBMIT_CURRENT, perforcesubmitcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, &QAction::triggered, this, &PerforcePlugin::submitCurrentLog);

    m_diffSelectedFiles = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_diffSelectedFiles, DIFF_SELECTED, perforcesubmitcontext);

    m_undoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, perforcesubmitcontext);

    m_redoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, perforcesubmitcontext);

    return true;
}

void PerforcePlugin::extensionsInitialized()
{
    VcsBasePlugin::extensionsInitialized();
    getTopLevel();
}

void PerforcePlugin::openCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsOpen(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QTextCodec *codec = VcsBaseEditor::getCodec(state.currentFile());
    QStringList args;
    args << QLatin1String("diff") << QLatin1String("-sa") << state.relativeCurrentFile();
    PerforceResponse result = runP4Cmd(state.currentFileTopLevel(), args,
                                       RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow,
                                       QStringList(), QByteArray(), codec);
    if (result.error)
        return;
    // "foo.cpp - file(s) not opened on this client."
    // also revert when the output is empty: The file is unchanged but open then.
    if (result.stdOut.contains(QLatin1String(" - ")) || result.stdErr.contains(QLatin1String(" - ")))
        return;

    bool doNotRevert = false;
    if (!result.stdOut.isEmpty())
        doNotRevert = (QMessageBox::warning(ICore::dialogParent(), tr("p4 revert"),
                                            tr("The file has been changed. Do you want to revert it?"),
                                            QMessageBox::Yes, QMessageBox::No) == QMessageBox::No);
    if (doNotRevert)
        return;

    FileChangeBlocker fcb(state.currentFile());
    args.clear();
    args << QLatin1String("revert") << state.relativeCurrentFile();
    PerforceResponse result2 = runP4Cmd(state.currentFileTopLevel(), args,
                                        CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (!result2.error)
        perforceVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void PerforcePlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    p4Diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void PerforcePlugin::diffCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    p4Diff(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePlugin::diffAllOpened()
{
    p4Diff(m_settings.topLevel(), QStringList());
}

void PerforcePlugin::updateCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    updateCheckout(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePlugin::updateAll()
{
    updateCheckout(m_settings.topLevel());
}

void PerforcePlugin::revertCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    const QString msg = tr("Do you want to revert all changes to the project \"%1\"?").arg(state.currentProjectName());
    if (QMessageBox::warning(ICore::dialogParent(), tr("p4 revert"), msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), false);
}

void PerforcePlugin::revertUnchangedCurrentProject()
{
    // revert -a.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), true);
}

bool PerforcePlugin::revertProject(const QString &workingDir, const QStringList &pathArgs, bool unchangedOnly)
{
    QStringList args(QLatin1String("revert"));
    if (unchangedOnly)
        args.push_back(QLatin1String("-a"));
    args.append(pathArgs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !resp.error;
}

void PerforcePlugin::updateCheckout(const QString &workingDir, const QStringList &dirs)
{
    QStringList args(QLatin1String("sync"));
    args.append(dirs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (dirs.empty()) {
        if (!workingDir.isEmpty())
            perforceVersionControl()->emitRepositoryChanged(workingDir);
    } else {
        const QChar slash = QLatin1Char('/');
        foreach (const QString &dir, dirs)
            perforceVersionControl()->emitRepositoryChanged(workingDir + slash + dir);
    }
}

void PerforcePlugin::printOpenedFileList()
{
    const PerforceResponse perforceResponse
            = runP4Cmd(m_settings.topLevel(), QStringList(QLatin1String("opened")),
                       CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (perforceResponse.error || perforceResponse.stdOut.isEmpty())
        return;
    // reformat "//depot/file.cpp#1 - description" into "file.cpp # - description"
    // for context menu opening to work. This produces absolute paths, then.
    QString errorMessage;
    QString mapped;
    const QChar delimiter = QLatin1Char('#');
    foreach (const QString &line, perforceResponse.stdOut.split(QLatin1Char('\n'))) {
        mapped.clear();
        const int delimiterPos = line.indexOf(delimiter);
        if (delimiterPos > 0)
            mapped = fileNameFromPerforceName(line.left(delimiterPos), true, &errorMessage);
        if (mapped.isEmpty())
            VcsOutputWindow::appendSilently(line);
        else
            VcsOutputWindow::appendSilently(mapped + QLatin1Char(' ') + line.mid(delimiterPos));
    }
    VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
}

void PerforcePlugin::startSubmitProject()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another submit is currently executed."));
        return;
    }

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    // Revert all unchanged files.
    if (!revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), true))
        return;
    // Start a change
    QStringList args;

    args << QLatin1String("change") << QLatin1String("-o");
    PerforceResponse result = runP4Cmd(state.currentProjectTopLevel(), args,
                                       RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result.error) {
        cleanCommitMessageFile();
        return;
    }

    TempFileSaver saver;
    saver.setAutoRemove(false);
    saver.write(result.stdOut.toLatin1());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        cleanCommitMessageFile();
        return;
    }
    m_commitMessageFileName = saver.fileName();

    args.clear();
    args << QLatin1String("files");
    args.append(perforceRelativeProjectDirectory(state));
    PerforceResponse filesResult = runP4Cmd(state.currentProjectTopLevel(), args,
                                            RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (filesResult.error) {
        cleanCommitMessageFile();
        return;
    }

    QStringList filesLines = filesResult.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    foreach (const QString &line, filesLines) {
        depotFileNames.append(line.left(line.lastIndexOf(QRegExp(QLatin1String("#[0-9]+\\s-\\s")))));
    }
    if (depotFileNames.isEmpty()) {
        VcsOutputWindow::appendWarning(tr("Project has no files"));
        cleanCommitMessageFile();
        return;
    }

    openPerforceSubmitEditor(m_commitMessageFileName, depotFileNames);
}

IEditor *PerforcePlugin::openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames)
{
    IEditor *editor = EditorManager::openEditor(fileName, PERFORCE_SUBMIT_EDITOR_ID);
    PerforceSubmitEditor *submitEditor = static_cast<PerforceSubmitEditor*>(editor);
    setSubmitEditor(submitEditor);
    submitEditor->restrictToProjectFiles(depotFileNames);
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentLogAction, m_diffSelectedFiles);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &PerforcePlugin::slotSubmitDiff);
    submitEditor->setCheckScriptWorkingDirectory(m_settings.topLevel());
    return editor;
}

void PerforcePlugin::printPendingChanges()
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    PendingChangesDialog dia(pendingChangesData(), ICore::mainWindow());
    QGuiApplication::restoreOverrideCursor();
    if (dia.exec() == QDialog::Accepted) {
        const int i = dia.changeNumber();
        QStringList args(QLatin1String("submit"));
        args << QLatin1String("-c") << QString::number(i);
        runP4Cmd(m_settings.topLevel(), args,
                 CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    }
}

void PerforcePlugin::describeChange()
{
    ChangeNumberDialog dia;
    if (dia.exec() == QDialog::Accepted && dia.number() > 0)
        describe(QString(), QString::number(dia.number()));
}

void PerforcePlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::annotateFile()
{
    const QString file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("p4 annotate"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        annotate(fi.absolutePath(), fi.fileName());
    }
}

void PerforcePlugin::vcsAnnotate(const QString &workingDirectory, const QString &file,
                                 const QString &revision, int lineNumber)
{
    annotate(workingDirectory, file, revision, lineNumber);
}

void PerforcePlugin::annotate(const QString &workingDir,
                              const QString &fileName,
                              const QString &changeList /* = QString() */,
                              int lineNumber /* = -1 */)
{
    const QStringList files = QStringList(fileName);
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, changeList);
    const QString source = VcsBaseEditor::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-cqi");
    if (changeList.isEmpty())
        args << fileName;
    else
        args << (fileName + QLatin1Char('@') + changeList);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        if (lineNumber < 1)
            lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor();
        IEditor *ed = showOutputInEditor(tr("p4 annotate %1").arg(id),
                                         result.stdOut, AnnotateOutput,
                                         source, codec);
        VcsBaseEditor::gotoLineOfEditor(ed, lineNumber);
    }
}

void PerforcePlugin::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void PerforcePlugin::filelogFile()
{
    const QString file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("p4 filelog"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        filelog(fi.absolutePath(), fi.fileName());
    }
}

void PerforcePlugin::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    changelists(state.currentProjectTopLevel(), perforceRelativeFileArguments(state.relativeCurrentProject()));
}

void PerforcePlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    changelists(state.topLevel(), perforceRelativeFileArguments(QString()));
}

void PerforcePlugin::filelog(const QString &workingDir, const QString &fileName,
                             bool enableAnnotationContextMenu)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("filelog") << QLatin1String("-li");
    if (m_settings.logCount() > 0)
        args << QLatin1String("-m") << QString::number(m_settings.logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        const QString source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(tr("p4 filelog %1").arg(id), result.stdOut,
                                LogOutput, source, codec);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(editor)->setFileLogAnnotateEnabled(true);
    }
}

void PerforcePlugin::changelists(const QString &workingDir, const QString &fileName)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("changelists") << QLatin1String("-lit");
    if (m_settings.logCount() > 0)
        args << QLatin1String("-m") << QString::number(m_settings.logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        const QString source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(tr("p4 changelists %1").arg(id), result.stdOut,
                                             LogOutput, source, codec);
        VcsBaseEditor::gotoLineOfEditor(editor, 1);
    }
}

void PerforcePlugin::updateActions(VcsBasePlugin::ActionState as)
{
    const bool menuActionEnabled = enableMenuAction(as, m_menuAction);
    const bool enableActions = currentState().hasTopLevel() && menuActionEnabled;
    m_commandLocator->setEnabled(enableActions);
    if (!menuActionEnabled)
        return;

    const QString fileName = currentState().currentFileName();
    m_editAction->setParameter(fileName);
    m_addAction->setParameter(fileName);
    m_deleteAction->setParameter(fileName);
    m_revertFileAction->setParameter(fileName);
    m_diffFileAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
    m_filelogCurrentAction->setParameter(fileName);

    const QString projectName = currentState().currentProjectName();
    m_logProjectAction->setParameter(projectName);
    m_updateProjectAction->setParameter(projectName);
    m_diffProjectAction->setParameter(projectName);
    m_submitProjectAction->setParameter(projectName);
    m_revertProjectAction->setParameter(projectName);
    m_revertUnchangedAction->setParameter(projectName);
}

bool PerforcePlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */)
{
    const bool rc = managesDirectoryFstat(directory);
    if (topLevel) {
        if (rc)
            *topLevel = m_settings.topLevelSymLinkTarget();
        else
            topLevel->clear();
    }
    return rc;
}

bool PerforcePlugin::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("fstat") << QLatin1String("-m1") << fileName;
    const PerforceResponse result = runP4Cmd(workingDirectory, args, RunFullySynchronous);
    return result.stdOut.contains(QLatin1String("depotFile"));
}

bool PerforcePlugin::managesDirectoryFstat(const QString &directory)
{
    // Cached?
    const ManagedDirectoryCache::const_iterator cit = m_managedDirectoryCache.constFind(directory);
    if (cit != m_managedDirectoryCache.constEnd()) {
        const DirectoryCacheEntry &entry = cit.value();
        setTopLevel(entry.m_topLevel);
        return entry.m_isManaged;
    }
    if (!m_settings.isValid()) {
        if (m_settings.topLevel().isEmpty() && m_settings.defaultEnv())
            getTopLevel(directory, true);

        if (!m_settings.isValid())
            return false;
    }
    // Determine value and insert into cache
    bool managed = false;
    do {
        // Quick check: Must be at or below top level and not "../../other_path"
        const QString relativeDirArgs = m_settings.relativeToTopLevelArguments(directory);
        if (!relativeDirArgs.isEmpty() && relativeDirArgs.startsWith(QLatin1String(".."))) {
            if (!m_settings.defaultEnv())
                break;
            else
                getTopLevel(directory, true);
        }
        // Is it actually managed by perforce?
        QStringList args;
        args << QLatin1String("fstat") << QLatin1String("-m1") << perforceRelativeFileArguments(relativeDirArgs);
        const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args,
                                                 RunFullySynchronous);

        managed = result.stdOut.contains(QLatin1String("depotFile"))
                  || result.stdErr.contains(QLatin1String("... - no such file(s)"));
    } while (false);

    m_managedDirectoryCache.insert(directory, DirectoryCacheEntry(managed, m_settings.topLevel()));
    return managed;
}

bool PerforcePlugin::vcsOpen(const QString &workingDir, const QString &fileName, bool silently)
{
    QStringList args;
    args << QLatin1String("edit") << QDir::toNativeSeparators(fileName);

    int flags = CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow;
    if (silently) {
        flags |= SilentStdOut;
    }
    const PerforceResponse result = runP4Cmd(workingDir, args, flags);
    return !result.error;
}

bool PerforcePlugin::vcsAdd(const QString &workingDir, const QString &fileName)
{
    QStringList args;
    args << QLatin1String("add") << fileName;
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !result.error;
}

bool PerforcePlugin::vcsDelete(const QString &workingDir, const QString &fileName)
{

    QStringList args;
    args << QLatin1String("revert") << fileName;
    const PerforceResponse revertResult = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (revertResult.error)
        return false;
    args.clear();
    args << QLatin1String("delete") << fileName;
    const PerforceResponse deleteResult = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    // TODO need to carefully parse the actual messages from perforce
    // or do a fstat before to decide what to do

    // Different states are:
    // File is in depot and unopened => p4 delete %
    // File is in depot and opened => p4 revert %, p4 delete %
    // File is not in depot => p4 revert %
    return !deleteResult.error;
}

bool PerforcePlugin::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    // TODO verify this works
    QStringList args;
    args << QLatin1String("edit") << from;
    const PerforceResponse editResult = runP4Cmd(workingDir, args,
                                                 RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (editResult.error)
        return false;
    args.clear();
    args << QLatin1String("move") << from << to;
    const PerforceResponse moveResult = runP4Cmd(workingDir, args,
                                                 RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !moveResult.error;
}

// Write extra args to temporary file
QSharedPointer<TempFileSaver>
PerforcePlugin::createTemporaryArgumentFile(const QStringList &extraArgs,
                                            QString *errorString)
{
    if (extraArgs.isEmpty())
        return QSharedPointer<TempFileSaver>();
    // create pattern
    QString pattern = m_instance->m_tempFilePattern;
    if (pattern.isEmpty()) {
        pattern = Utils::TemporaryDirectory::masterDirectoryPath() + "/qtc_p4_XXXXXX.args";
        m_instance->m_tempFilePattern = pattern;
    }
    QSharedPointer<TempFileSaver> rc(new TempFileSaver(pattern));
    rc->setAutoRemove(true);
    const int last = extraArgs.size() - 1;
    for (int i = 0; i <= last; i++) {
        rc->write(extraArgs.at(i).toLocal8Bit());
        if (i != last)
            rc->write("\n", 1);
    }
    if (!rc->finalize(errorString))
        return QSharedPointer<TempFileSaver>();
    return rc;
}

// Run messages

static inline QString msgNotStarted(const QString &cmd)
{
    return PerforcePlugin::tr("Could not start perforce \"%1\". Please check your settings in the preferences.").arg(cmd);
}

static inline QString msgTimeout(int timeOutS)
{
    return PerforcePlugin::tr("Perforce did not respond within timeout limit (%1 s).").arg(timeOutS);
}

static inline QString msgCrash()
{
    return PerforcePlugin::tr("The process terminated abnormally.");
}

static inline QString msgExitCode(int ex)
{
    return PerforcePlugin::tr("The process terminated with exit code %1.").arg(ex);
}

// Run using a SynchronousProcess, emitting signals to the message window
PerforceResponse PerforcePlugin::synchronousProcess(const QString &workingDir,
                                                    const QStringList &args,
                                                    unsigned flags,
                                                    const QByteArray &stdInput,
                                                    QTextCodec *outputCodec)
{
    QTC_ASSERT(stdInput.isEmpty(), return PerforceResponse()); // Not supported here

    VcsOutputWindow *outputWindow = VcsOutputWindow::instance();
    // Run, connect stderr to the output window
    SynchronousProcess process;
    const int timeOutS = (flags & LongTimeOut) ? settings().longTimeOutS() : settings().timeOutS();
    process.setTimeoutS(timeOutS);
    process.setCodec(outputCodec);
    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    // connect stderr to the output window if desired
    if (flags & StdErrToWindow) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, &SynchronousProcess::stdErrBuffered,
                outputWindow, [outputWindow](const QString &lines) {
            outputWindow->append(lines);
        });
    }

    // connect stdout to the output window if desired
    if (flags & StdOutToWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        if (flags & SilentStdOut) {
            connect(&process, &SynchronousProcess::stdOutBuffered,
                    outputWindow, &VcsOutputWindow::appendSilently);
        } else {
            connect(&process, &SynchronousProcess::stdOutBuffered,
                    outputWindow, [outputWindow](const QString &lines) {
                outputWindow->append(lines);
            });
        }
    }
    process.setTimeOutMessageBoxEnabled(true);
    const SynchronousProcessResponse sp_resp = process.run(settings().p4BinaryPath(), args);

    PerforceResponse response;
    response.error = true;
    response.exitCode = sp_resp.exitCode;
    response.stdErr = sp_resp.stdErr();
    response.stdOut = sp_resp.stdOut();
    switch (sp_resp.result) {
    case SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case SynchronousProcessResponse::FinishedError:
        response.message = msgExitCode(sp_resp.exitCode);
        response.error = !(flags & IgnoreExitCode);
        break;
    case SynchronousProcessResponse::TerminatedAbnormally:
        response.message = msgCrash();
        break;
    case SynchronousProcessResponse::StartFailed:
        response.message = msgNotStarted(settings().p4BinaryPath());
        break;
    case SynchronousProcessResponse::Hang:
        response.message = msgCrash();
        break;
    }
    return response;
}

// Run using a QProcess, for short queries
PerforceResponse PerforcePlugin::fullySynchronousProcess(const QString &workingDir,
                                                         const QStringList &args,
                                                         unsigned flags,
                                                         const QByteArray &stdInput,
                                                         QTextCodec *outputCodec)
{
    QProcess process;

    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    PerforceResponse response;
    process.start(settings().p4BinaryPath(), args);
    if (stdInput.isEmpty())
        process.closeWriteChannel();

    if (!process.waitForStarted(3000)) {
        response.error = true;
        response.message = msgNotStarted(settings().p4BinaryPath());
        return response;
    }
    if (!stdInput.isEmpty()) {
        if (process.write(stdInput) == -1) {
            SynchronousProcess::stopProcess(process);
            response.error = true;
            response.message = tr("Unable to write input data to process %1: %2").
                               arg(QDir::toNativeSeparators(settings().p4BinaryPath()),
                                   process.errorString());
            return response;
        }
        process.closeWriteChannel();
    }

    QByteArray stdOut;
    QByteArray stdErr;
    const int timeOutS = (flags & LongTimeOut) ? settings().longTimeOutS() : settings().timeOutS();
    if (!SynchronousProcess::readDataFromProcess(process, timeOutS, &stdOut, &stdErr, true)) {
        SynchronousProcess::stopProcess(process);
        response.error = true;
        response.message = msgTimeout(timeOutS);
        return response;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        response.error = true;
        response.message = msgCrash();
        return response;
    }
    response.exitCode = process.exitCode();
    response.error = response.exitCode ? !(flags & IgnoreExitCode) : false;
    response.stdErr = QString::fromLocal8Bit(stdErr);
    response.stdOut = outputCodec ? outputCodec->toUnicode(stdOut.constData(), stdOut.size()) :
                                    QString::fromLocal8Bit(stdOut);
    const QChar cr = QLatin1Char('\r');
    response.stdErr.remove(cr);
    response.stdOut.remove(cr);
    // Logging
    if ((flags & StdErrToWindow) && !response.stdErr.isEmpty())
        VcsOutputWindow::appendError(response.stdErr);
    if ((flags & StdOutToWindow) && !response.stdOut.isEmpty())
        VcsOutputWindow::append(response.stdOut, VcsOutputWindow::None, flags & SilentStdOut);
    return response;
}

PerforceResponse PerforcePlugin::runP4Cmd(const QString &workingDir,
                                          const QStringList &args,
                                          unsigned flags,
                                          const QStringList &extraArgs,
                                          const QByteArray &stdInput,
                                          QTextCodec *outputCodec)
{
    if (!settings().isValid()) {
        PerforceResponse invalidConfigResponse;
        invalidConfigResponse.error = true;
        invalidConfigResponse.message = tr("Perforce is not correctly configured.");
        VcsOutputWindow::appendError(invalidConfigResponse.message);
        return invalidConfigResponse;
    }
    QStringList actualArgs = settings().commonP4Arguments(workingDir);
    QString errorMessage;
    QSharedPointer<TempFileSaver> tempFile = createTemporaryArgumentFile(extraArgs, &errorMessage);
    if (!tempFile.isNull()) {
        actualArgs << QLatin1String("-x") << tempFile->fileName();
    } else if (!errorMessage.isEmpty()) {
        PerforceResponse tempFailResponse;
        tempFailResponse.error = true;
        tempFailResponse.message = errorMessage;
        return tempFailResponse;
    }
    actualArgs.append(args);

    if (flags & CommandToWindow)
        VcsOutputWindow::appendCommand(workingDir, FileName::fromString(settings().p4BinaryPath()), actualArgs);

    if (flags & ShowBusyCursor)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    const PerforceResponse  response = (flags & RunFullySynchronous)  ?
        fullySynchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec) :
        synchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec);

    if (flags & ShowBusyCursor)
        QApplication::restoreOverrideCursor();

    if (response.error && (flags & ErrorToWindow))
        VcsOutputWindow::appendError(response.message);
    return response;
}

IEditor *PerforcePlugin::showOutputInEditor(const QString &title,
                                            const QString &output,
                                            int editorType,
                                            const QString &source,
                                             QTextCodec *codec)
{
    const VcsBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const Id id = params->id;
    QString s = title;
    QString content = output;
    const int maxSize = int(EditorManager::maxTextFileSize() / 2  - 1000L); // ~25 MB, 600000 lines
    if (content.size() >= maxSize) {
        content = content.left(maxSize);
        content += QLatin1Char('\n') + tr("[Only %n MB of output shown]", 0, maxSize / 1024 / 1024);
    }
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, content.toUtf8());
    QTC_ASSERT(editor, return 0);
    auto e = qobject_cast<PerforceEditorWidget*>(editor->widget());
    if (!e)
        return 0;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested, this, &PerforcePlugin::annotate);
    e->setForceReadOnly(true);
    e->setSource(source);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (codec)
        e->setCodec(codec);
    return editor;
}

void PerforcePlugin::slotSubmitDiff(const QStringList &files)
{
    p4Diff(m_settings.topLevel(), files);
}

struct PerforceDiffParameters
{
    QString workingDir;
    QStringList diffArguments;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class PerforceDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    explicit PerforceDiffConfig(const PerforceDiffParameters &p, QToolBar *toolBar);
    void triggerReRun();

signals:
    void reRunDiff(const Perforce::Internal::PerforceDiffParameters &);

private:
    const PerforceDiffParameters m_parameters;
};

PerforceDiffConfig::PerforceDiffConfig(const PerforceDiffParameters &p, QToolBar *toolBar) :
    VcsBaseEditorConfig(toolBar), m_parameters(p)
{
    setBaseArguments(p.diffArguments);
    addToggleButton(QLatin1String("w"), tr("Ignore Whitespace"));
    connect(this, &VcsBaseEditorConfig::argumentsChanged, this, &PerforceDiffConfig::triggerReRun);
}

void PerforceDiffConfig::triggerReRun()
{
    PerforceDiffParameters effectiveParameters = m_parameters;
    effectiveParameters.diffArguments = arguments();
    emit reRunDiff(effectiveParameters);
}

QString PerforcePlugin::commitDisplayName() const
{
    return tr("submit", "\"commit\" action for perforce");
}

void PerforcePlugin::p4Diff(const QString &workingDir, const QStringList &files)
{
    PerforceDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.diffArguments.push_back(QString(QLatin1Char('u')));
    p4Diff(p);
}

void PerforcePlugin::p4Diff(const PerforceDiffParameters &p)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(p.workingDir, p.files);
    const QString id = VcsBaseEditor::getTitleId(p.workingDir, p.files);
    // Reuse existing editors for that id
    const QString tag = VcsBaseEditor::editorTag(DiffOutput, p.workingDir, p.files);
    IEditor *existingEditor = VcsBaseEditor::locateEditorByTag(tag);
    // Split arguments according to size
    QStringList args;
    args << QLatin1String("diff");
    if (!p.diffArguments.isEmpty()) // -duw..
        args << (QLatin1String("-d") + p.diffArguments.join(QString()));
    QStringList extraArgs;
    if (p.files.size() > 1)
        extraArgs = p.files;
    else
        args.append(p.files);
    const unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow|OverrideDiffEnvironment;
    const PerforceResponse result = runP4Cmd(p.workingDir, args, flags,
                                             extraArgs, QByteArray(), codec);
    if (result.error)
        return;

    if (existingEditor) {
        existingEditor->document()->setContents(result.stdOut.toUtf8());
        EditorManager::activateEditor(existingEditor);
        return;
    }
    // Create new editor
    IEditor *editor = showOutputInEditor(tr("p4 diff %1").arg(id), result.stdOut, VcsBase::DiffOutput,
                                         VcsBaseEditor::getSource(p.workingDir, p.files),
                                         codec);
    VcsBaseEditor::tagEditor(editor, tag);
    auto diffEditorWidget = qobject_cast<VcsBaseEditorWidget *>(editor->widget());
    // Wire up the parameter widget to trigger a re-run on
    // parameter change and 'revert' from inside the diff editor.
    auto pw = new PerforceDiffConfig(p, diffEditorWidget->toolBar());
    connect(pw, &PerforceDiffConfig::reRunDiff,
            this, [this](const PerforceDiffParameters &p) { p4Diff(p); });
    connect(diffEditorWidget, &VcsBaseEditorWidget::diffChunkReverted,
            pw, &PerforceDiffConfig::triggerReRun);
    diffEditorWidget->setEditorConfig(pw);
}

void PerforcePlugin::describe(const QString & source, const QString &n)
{
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VcsBaseEditor::getCodec(source);
    QStringList args;
    args << QLatin1String("describe") << QLatin1String("-du") << n;
    const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args, CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error)
        showOutputInEditor(tr("p4 describe %1").arg(n), result.stdOut, VcsBase::DiffOutput, source, codec);
}

void PerforcePlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

void PerforcePlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
    }
}

bool PerforcePlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

bool PerforcePlugin::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;
    auto perforceEditor = qobject_cast<PerforceSubmitEditor *>(submitEditor());
    QTC_ASSERT(perforceEditor, return true);
    IDocument *editorDocument = perforceEditor->document();
    QTC_ASSERT(editorDocument, return true);
    // Prompt the user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool wantsPrompt = m_settings.promptToSubmit();
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            perforceEditor->promptSubmit(tr("Closing p4 Editor"),
                                         tr("Do you want to submit this change list?"),
                                         tr("The commit message check failed. Do you want to submit this change list?"),
                                         &wantsPrompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;

    if (answer == VcsBaseSubmitEditor::SubmitCanceled)
        return false;

    // Set without triggering the checking mechanism
    if (wantsPrompt != m_settings.promptToSubmit()) {
        m_settings.setPromptToSubmit(wantsPrompt);
        m_settings.toSettings(ICore::settings());
    }
    if (!DocumentManager::saveDocument(editorDocument))
        return false;
    if (answer == VcsBaseSubmitEditor::SubmitDiscarded) {
        cleanCommitMessageFile();
        return true;
    }
    // Pipe file into p4 submit -i
    FileReader reader;
    if (!reader.fetch(m_commitMessageFileName, QIODevice::Text)) {
        VcsOutputWindow::appendError(reader.errorString());
        return false;
    }

    QStringList submitArgs;
    submitArgs << QLatin1String("submit") << QLatin1String("-i");
    const PerforceResponse submitResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), submitArgs,
                                                     LongTimeOut|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow|ShowBusyCursor,
                                                     QStringList(), reader.data());
    if (submitResponse.error) {
        VcsOutputWindow::appendError(tr("p4 submit failed: %1").arg(submitResponse.message));
        return false;
    }
    VcsOutputWindow::append(submitResponse.stdOut);
    if (submitResponse.stdOut.contains(QLatin1String("Out of date files must be resolved or reverted)")))
        QMessageBox::warning(perforceEditor->widget(), tr("Pending change"), tr("Could not submit the change, because your workspace was out of date. Created a pending submit instead."));

    cleanCommitMessageFile();
    return true;
}

QString PerforcePlugin::clientFilePath(const QString &serverFilePath)
{
    QTC_ASSERT(m_settings.isValid(), return QString());

    QStringList args;
    args << QLatin1String("fstat") << serverFilePath;
    const PerforceResponse response = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               ShowBusyCursor|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (response.error)
        return QString();

    QRegExp r(QLatin1String("\\.\\.\\.\\sclientFile\\s(.+)\n"));
    r.setMinimal(true);
    return r.indexIn(response.stdOut) != -1 ? r.cap(1).trimmed() : QString();
}

QString PerforcePlugin::pendingChangesData()
{
    QTC_ASSERT(m_settings.isValid(), return QString());

    QStringList args = QStringList(QLatin1String("info"));
    const PerforceResponse userResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (userResponse.error)
        return QString();

    QRegExp r(QLatin1String("User\\sname:\\s(\\S+)\\s*\n"));
    QTC_ASSERT(r.isValid(), return QString());
    r.setMinimal(true);
    const QString user = r.indexIn(userResponse.stdOut) != -1 ? r.cap(1).trimmed() : QString();
    if (user.isEmpty())
        return QString();
    args.clear();
    args << QLatin1String("changes") << QLatin1String("-s") << QLatin1String("pending") << QLatin1String("-u") << user;
    const PerforceResponse dataResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                                   RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    return dataResponse.error ? QString() : dataResponse.stdOut;
}

const PerforceSettings& PerforcePlugin::settings()
{
    return m_instance->m_settings;
}

void PerforcePlugin::setSettings(const Settings &newSettings)
{
    if (newSettings != m_instance->m_settings.settings()) {
        m_instance->m_settings.setSettings(newSettings);
        m_instance->m_managedDirectoryCache.clear();
        m_instance->m_settings.toSettings(ICore::settings());
        getTopLevel();
        perforceVersionControl()->emitConfigurationChanged();
    }
}

static inline QString msgWhereFailed(const QString & file, const QString &why)
{
    //: Failed to run p4 "where" to resolve a Perforce file name to a local
    //: file system name.
    return PerforcePlugin::tr("Error running \"where\" on %1: %2").
            arg(QDir::toNativeSeparators(file), why);
}

// Map a perforce name "//xx" to its real name in the file system
QString PerforcePlugin::fileNameFromPerforceName(const QString& perforceName,
                                                 bool quiet,
                                                 QString *errorMessage)
{
    // All happy, already mapped
    if (!perforceName.startsWith(QLatin1String("//")))
        return perforceName;
    // "where" remaps the file to client file tree
    QStringList args;
    args << QLatin1String("where") << perforceName;
    unsigned flags = RunFullySynchronous;
    if (!quiet)
        flags |= CommandToWindow|StdErrToWindow|ErrorToWindow;
    const PerforceResponse response = runP4Cmd(settings().topLevelSymLinkTarget(), args, flags);
    if (response.error) {
        *errorMessage = msgWhereFailed(perforceName, response.message);
        return QString();
    }

    QString output = response.stdOut;
    if (output.endsWith(QLatin1Char('\r')))
        output.chop(1);
    if (output.endsWith(QLatin1Char('\n')))
        output.chop(1);

    if (output.isEmpty()) {
        //: File is not managed by Perforce
        *errorMessage = msgWhereFailed(perforceName, tr("The file is not mapped"));
        return QString();
    }
    const QString p4fileSpec = output.mid(output.lastIndexOf(QLatin1Char(' ')) + 1);
    return m_instance->m_settings.mapToFileSystem(p4fileSpec);
}

PerforceVersionControl *PerforcePlugin::perforceVersionControl()
{
    return static_cast<PerforceVersionControl *>(m_instance->versionControl());
}

void PerforcePlugin::setTopLevel(const QString &topLevel)
{
    if (m_settings.topLevel() == topLevel)
        return;

    m_settings.setTopLevel(topLevel);

    const QString msg = tr("Perforce repository: %1").arg(QDir::toNativeSeparators(topLevel));
    VcsOutputWindow::appendSilently(msg);
}

void PerforcePlugin::slotTopLevelFailed(const QString &errorMessage)
{
    VcsOutputWindow::appendSilently(tr("Perforce: Unable to determine the repository: %1").arg(errorMessage));
}

void PerforcePlugin::getTopLevel(const QString &workingDirectory, bool isSync)
{
    // Run a new checker
    if (m_instance->m_settings.p4BinaryPath().isEmpty())
        return;
    auto checker = new PerforceChecker(m_instance);
    connect(checker, &PerforceChecker::failed, m_instance, &PerforcePlugin::slotTopLevelFailed);
    connect(checker, &PerforceChecker::failed, checker, &QObject::deleteLater);
    connect(checker, &PerforceChecker::succeeded, m_instance, &PerforcePlugin::setTopLevel);
    connect(checker, &PerforceChecker::succeeded,checker, &QObject::deleteLater);

    checker->start(settings().p4BinaryPath(), workingDirectory,
                   settings().commonP4Arguments(QString()), 30000);

    if (isSync)
        checker->waitForFinished();
}

#ifdef WITH_TESTS
void PerforcePlugin::testLogResolving()
{
    // Source: http://mail.opensolaris.org/pipermail/opengrok-discuss/2008-October/001668.html
    QByteArray data(
                "... #4 change 12345 edit on 2013/01/28 by User at UserWorkspaceName(text)\n"
                "\n"
                "        Comment\n"
                "... #3 change 12344 edit on 2013/01/27 by User at UserWorkspaceName(text)\n"
                "\n"
                "        Comment\n"
                );
    VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "12345", "12344");
}
#endif

} // namespace Internal
} // namespace Perforce

#include "perforceplugin.moc"
