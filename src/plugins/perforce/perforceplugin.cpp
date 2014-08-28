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

#include "perforceplugin.h"

#include "changenumberdialog.h"
#include "pendingchangesdialog.h"
#include "perforceconstants.h"
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
#include <coreplugin/mimedatabase.h>
#include <coreplugin/locator/commandlocator.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/fileutils.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>

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

const char PERFORCE_SUBMIT_EDITOR_ID[] = "Perforce.SubmitEditor";
const char PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce.SubmitEditor");
const char PERFORCESUBMITEDITOR_CONTEXT[] = "Perforce Submit Editor";

const char PERFORCE_LOG_EDITOR_ID[] = "Perforce.LogEditor";
const char PERFORCE_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Log Editor");
const char PERFORCE_LOG_EDITOR_CONTEXT[] = "Perforce Log Editor";

const char PERFORCE_DIFF_EDITOR_ID[] = "Perforce.DiffEditor";
const char PERFORCE_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Diff Editor");
const char PERFORCE_DIFF_EDITOR_CONTEXT[] = "Perforce Diff Editor";

const char PERFORCE_ANNOTATION_EDITOR_ID[] = "Perforce.AnnotationEditor";
const char PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Annotation Editor");
const char PERFORCE_ANNOTATION_EDITOR_CONTEXT[] = "Perforce Annotation Editor";

const VcsBaseEditorParameters editorParameters[] = {
{
    VcsBase::LogOutput,
    PERFORCE_LOG_EDITOR_ID,
    PERFORCE_LOG_EDITOR_DISPLAY_NAME,
    PERFORCE_LOG_EDITOR_CONTEXT,
    "text/vnd.qtcreator.p4.log"},
{    VcsBase::AnnotateOutput,
    PERFORCE_ANNOTATION_EDITOR_ID,
    PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME,
    PERFORCE_ANNOTATION_EDITOR_CONTEXT,
    "text/vnd.qtcreator.p4.annotation"},
{   VcsBase::DiffOutput,
    PERFORCE_DIFF_EDITOR_ID,
    PERFORCE_DIFF_EDITOR_DISPLAY_NAME,
    PERFORCE_DIFF_EDITOR_CONTEXT,
    "text/x-patch"}
};

// Utility to find a parameter set by type
static inline const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return VcsBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(editorParameters[0]), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
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

PerforcePlugin::PerforcePlugin() :
    m_commandLocator(0),
    m_editAction(0),
    m_addAction(0),
    m_deleteAction(0),
    m_openedAction(0),
    m_revertFileAction(0),
    m_diffFileAction(0),
    m_diffProjectAction(0),
    m_updateProjectAction(0),
    m_revertProjectAction(0),
    m_revertUnchangedAction(0),
    m_diffAllAction(0),
    m_submitProjectAction(0),
    m_pendingAction(0),
    m_describeAction(0),
    m_annotateCurrentAction(0),
    m_annotateAction(0),
    m_filelogCurrentAction(0),
    m_filelogAction(0),
    m_logProjectAction(0),
    m_logRepositoryAction(0),
    m_submitCurrentLogAction(0),
    m_updateAllAction(0),
    m_submitActionTriggered(false),
    m_diffSelectedFiles(0),
    m_undoAction(0),
    m_redoAction(0)
{
}

static const VcsBaseSubmitEditorParameters submitParameters = {
    SUBMIT_MIMETYPE,
    PERFORCE_SUBMIT_EDITOR_ID,
    PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME,
    PERFORCESUBMITEDITOR_CONTEXT,
    VcsBaseSubmitEditorParameters::DiffFiles
};

bool PerforcePlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    typedef VcsSubmitEditorFactory<PerforceSubmitEditor> PerforceSubmitEditorFactory;

    initializeVcs(new PerforceVersionControl(this));

    if (!MimeDatabase::addMimeTypes(QLatin1String(":/trolltech.perforce/Perforce.mimetypes.xml"), errorMessage))
        return false;
    m_instance = this;

    m_settings.fromSettings(ICore::settings());

    addAutoReleasedObject(new SettingsPage);

    // Editor factories
    addAutoReleasedObject(new PerforceSubmitEditorFactory(&submitParameters));

    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new PerforceEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, this, describeSlot));

    const QString prefix = QLatin1String("p4");
    m_commandLocator = new CommandLocator("Perforce", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *mperforce = ActionManager::createMenu(CMD_ID_PERFORCE_MENU);
    mperforce->menu()->setTitle(tr("&Perforce"));
    mtools->addMenu(mperforce);
    m_menuAction = mperforce->menu()->menuAction();

    Context globalcontext(Core::Constants::C_GLOBAL);
    Context perforcesubmitcontext(PERFORCESUBMITEDITOR_CONTEXT);

    Core::Command *command;

    m_diffFileAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFileAction, CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(tr("Diff Current File"));
    connect(m_diffFileAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction, CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(tr("Annotate Current File"));
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction, CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+F") : tr("Alt+P,Alt+F")));
    command->setDescription(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this, SLOT(filelogCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    mperforce->addSeparator(globalcontext);

    m_editAction = new ParameterAction(tr("Edit"), tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editAction, CMD_ID_EDIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+E") : tr("Alt+P,Alt+E")));
    command->setDescription(tr("Edit File"));
    connect(m_editAction, SIGNAL(triggered()), this, SLOT(openCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+A") : tr("Alt+P,Alt+A")));
    command->setDescription(tr("Add File"));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(tr("Delete File"));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFileAction = new ParameterAction(tr("Revert"), tr("Revert \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFileAction, CMD_ID_REVERT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+R") : tr("Alt+P,Alt+R")));
    command->setDescription(tr("Revert File"));
    connect(m_revertFileAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    mperforce->addSeparator(globalcontext);

    const QString diffProjectDefaultText = tr("Diff Current Project/Session");
    m_diffProjectAction = new ParameterAction(diffProjectDefaultText, tr("Diff Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+D") : tr("Alt+P,Alt+D")));
    command->setDescription(diffProjectDefaultText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffCurrentProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_submitProjectAction = new ParameterAction(tr("Submit Project"), tr("Submit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_submitProjectAction, CMD_ID_SUBMIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+S") : tr("Alt+P,Alt+S")));
    connect(m_submitProjectAction, SIGNAL(triggered()), this, SLOT(startSubmitProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    const QString updateProjectDefaultText = tr("Update Current Project");
    m_updateProjectAction = new ParameterAction(updateProjectDefaultText, tr("Update Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, globalcontext);
    command->setDescription(updateProjectDefaultText);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateCurrentProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertUnchangedAction = new ParameterAction(tr("Revert Unchanged"), tr("Revert Unchanged Files of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertUnchangedAction, CMD_ID_REVERT_UNCHANGED_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertUnchangedAction, SIGNAL(triggered()), this, SLOT(revertUnchangedCurrentProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertProjectAction = new ParameterAction(tr("Revert Project"), tr("Revert Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertProjectAction, CMD_ID_REVERT_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertProjectAction, SIGNAL(triggered()), this, SLOT(revertCurrentProject()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    mperforce->addSeparator(globalcontext);

    m_diffAllAction = new QAction(tr("Diff Opened Files"), this);
    command = ActionManager::registerAction(m_diffAllAction, CMD_ID_DIFF_ALL, globalcontext);
    connect(m_diffAllAction, SIGNAL(triggered()), this, SLOT(diffAllOpened()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_openedAction = new QAction(tr("Opened"), this);
    command = ActionManager::registerAction(m_openedAction, CMD_ID_OPENED, globalcontext);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+P,Meta+O") : tr("Alt+P,Alt+O")));
    connect(m_openedAction, SIGNAL(triggered()), this, SLOT(printOpenedFileList()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = ActionManager::registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, globalcontext);
    connect(m_pendingAction, SIGNAL(triggered()), this, SLOT(printPendingChanges()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateAllAction = new QAction(tr("Update All"), this);
    command = ActionManager::registerAction(m_updateAllAction, CMD_ID_UPDATEALL, globalcontext);
    connect(m_updateAllAction, SIGNAL(triggered()), this, SLOT(updateAll()));
    mperforce->addAction(command);
    m_commandLocator->appendCommand(command);

    mperforce->addSeparator(globalcontext);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, globalcontext);
    connect(m_describeAction, SIGNAL(triggered()), this, SLOT(describeChange()));
    mperforce->addAction(command);

    m_annotateAction = new QAction(tr("Annotate..."), this);
    command = ActionManager::registerAction(m_annotateAction, CMD_ID_ANNOTATE, globalcontext);
    connect(m_annotateAction, SIGNAL(triggered()), this, SLOT(annotate()));
    mperforce->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = ActionManager::registerAction(m_filelogAction, CMD_ID_FILELOG, globalcontext);
    connect(m_filelogAction, SIGNAL(triggered()), this, SLOT(filelog()));
    mperforce->addAction(command);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Submit"), this);
    command = ActionManager::registerAction(m_submitCurrentLogAction, SUBMIT_CURRENT, perforcesubmitcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFiles = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = ActionManager::registerAction(m_diffSelectedFiles, DIFF_SELECTED, perforcesubmitcontext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, perforcesubmitcontext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = ActionManager::registerAction(m_redoAction, Core::Constants::REDO, perforcesubmitcontext);

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
    args << QLatin1String("fstat");
    args.append(perforceRelativeProjectDirectory(state));
    PerforceResponse fstatResult = runP4Cmd(state.currentProjectTopLevel(), args,
                                            RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (fstatResult.error) {
        cleanCommitMessageFile();
        return;
    }

    QStringList fstatLines = fstatResult.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    foreach (const QString &line, fstatLines) {
        if (line.startsWith(QLatin1String("... depotFile")))
            depotFileNames.append(line.mid(14));
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
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(slotSubmitDiff(QStringList)));
    submitEditor->setCheckScriptWorkingDirectory(m_commitWorkingDirectory);
    return editor;
}

void PerforcePlugin::printPendingChanges()
{
    qApp->setOverrideCursor(Qt::WaitCursor);
    PendingChangesDialog dia(pendingChangesData(), ICore::mainWindow());
    qApp->restoreOverrideCursor();
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

void PerforcePlugin::annotate()
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
                                         result.stdOut, VcsBase::AnnotateOutput,
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

void PerforcePlugin::filelog()
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
    filelog(state.currentProjectTopLevel(), perforceRelativeFileArguments(state.relativeCurrentProject()));
}

void PerforcePlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel(), perforceRelativeFileArguments(QString()));
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
                                VcsBase::LogOutput, source, codec);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(editor)->setFileLogAnnotateEnabled(true);
    }
}

void PerforcePlugin::updateActions(VcsBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);

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

    m_diffAllAction->setEnabled(true);
    m_openedAction->setEnabled(true);
    m_describeAction->setEnabled(true);
    m_annotateAction->setEnabled(true);
    m_filelogAction->setEnabled(true);
    m_pendingAction->setEnabled(true);
    m_updateAllAction->setEnabled(true);
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
    if (!m_settings.isValid())
        return false;
    // Cached?
    const ManagedDirectoryCache::const_iterator cit = m_managedDirectoryCache.constFind(directory);
    if (cit != m_managedDirectoryCache.constEnd())
        return cit.value();
    // Determine value and insert into cache
    bool managed = false;
    do {
        // Quick check: Must be at or below top level and not "../../other_path"
        const QString relativeDirArgs = m_settings.relativeToTopLevelArguments(directory);
        if (!relativeDirArgs.isEmpty() && relativeDirArgs.startsWith(QLatin1String("..")))
            break;
        // Is it actually managed by perforce?
        QStringList args;
        args << QLatin1String("fstat") << QLatin1String("-m1") << perforceRelativeFileArguments(relativeDirArgs);
        const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args,
                                                 RunFullySynchronous);
        managed = result.stdOut.contains(QLatin1String("depotFile"))
                  || result.stdErr.contains(QLatin1String("... - no such file(s)"));
    } while (false);

    m_managedDirectoryCache.insert(directory, managed);
    return managed;
}

bool PerforcePlugin::vcsOpen(const QString &workingDir, const QString &fileName, bool silently)
{
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::vcsOpen" << workingDir << fileName;
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
        pattern = QDir::tempPath();
        if (!pattern.endsWith(QDir::separator()))
            pattern += QDir::separator();
        pattern += QLatin1String("qtc_p4_XXXXXX.args");
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

static inline QString msgTimeout(int timeOut)
{
    return PerforcePlugin::tr("Perforce did not respond within timeout limit (%1 ms).").arg(timeOut );
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
    const int timeOut = (flags & LongTimeOut) ? settings().longTimeOutMS() : settings().timeOutMS();
    process.setTimeout(timeOut);
    process.setCodec(outputCodec);
    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    // connect stderr to the output window if desired
    if (flags & StdErrToWindow) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }

    // connect stdout to the output window if desired
    if (flags & StdOutToWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        if (flags & SilentStdOut) {
            connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(appendSilently(QString)));
        }
        else {
            connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
        }
    }
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::run syncp actual args [" << process.workingDirectory() << ']' << args;
    process.setTimeOutMessageBoxEnabled(true);
    const SynchronousProcessResponse sp_resp = process.run(settings().p4BinaryPath(), args);
    if (Perforce::Constants::debug)
        qDebug() << sp_resp;

    PerforceResponse response;
    response.error = true;
    response.exitCode = sp_resp.exitCode;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
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

    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::run fully syncp actual args [" << process.workingDirectory() << ']' << args;

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
    const int timeOut = (flags & LongTimeOut) ? settings().longTimeOutMS() : settings().timeOutMS();
    if (!SynchronousProcess::readDataFromProcess(process, timeOut, &stdOut, &stdErr, true)) {
        SynchronousProcess::stopProcess(process);
        response.error = true;
        response.message = msgTimeout(timeOut);
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
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::runP4Cmd [" << workingDir << ']' << args << extraArgs << stdInput << debugCodec(outputCodec);

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

    if (response.error) {
        if (Perforce::Constants::debug)
            qDebug() << response.message;
        if (flags & ErrorToWindow)
            VcsOutputWindow::appendError(response.message);
    }
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
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::showOutputInEditor" << title << id.name()
                 <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)),
            this, SLOT(vcsAnnotate(QString,QString,QString,int)));
    PerforceEditorWidget *e = qobject_cast<PerforceEditorWidget*>(editor->widget());
    if (!e)
        return 0;
    e->setForceReadOnly(true);
    e->setSource(source);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setSuggestedFileName(s);
    if (codec)
        e->setCodec(codec);
    return editor;
}

void PerforcePlugin::slotSubmitDiff(const QStringList &files)
{
    p4Diff(m_commitWorkingDirectory, files);
}

struct PerforceDiffParameters
{
    QString workingDir;
    QStringList diffArguments;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class PerforceDiffParameterWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    explicit PerforceDiffParameterWidget(const PerforceDiffParameters &p, QWidget *parent = 0);

signals:
    void reRunDiff(const Perforce::Internal::PerforceDiffParameters &);

private slots:
    void triggerReRun();

private:
    const PerforceDiffParameters m_parameters;
};

PerforceDiffParameterWidget::PerforceDiffParameterWidget(const PerforceDiffParameters &p, QWidget *parent) :
    VcsBaseEditorParameterWidget(parent), m_parameters(p)
{
    setBaseArguments(p.diffArguments);
    addToggleButton(QLatin1String("w"), tr("Ignore Whitespace"));
    connect(this, SIGNAL(argumentsChanged()), this, SLOT(triggerReRun()));
}

void PerforceDiffParameterWidget::triggerReRun()
{
    PerforceDiffParameters effectiveParameters = m_parameters;
    effectiveParameters.diffArguments = arguments();
    emit reRunDiff(effectiveParameters);
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
    const QString tag = VcsBaseEditor::editorTag(VcsBase::DiffOutput, p.workingDir, p.files);
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
    VcsBaseEditorWidget *diffEditorWidget = qobject_cast<VcsBaseEditorWidget *>(editor->widget());
    // Wire up the parameter widget to trigger a re-run on
    // parameter change and 'revert' from inside the diff editor.
    PerforceDiffParameterWidget *pw = new PerforceDiffParameterWidget(p);
    connect(pw, SIGNAL(reRunDiff(Perforce::Internal::PerforceDiffParameters)),
            this, SLOT(p4Diff(Perforce::Internal::PerforceDiffParameters)));
    connect(diffEditorWidget, SIGNAL(diffChunkReverted(VcsBase::DiffChunk)),
            pw, SLOT(triggerReRun()));
    diffEditorWidget->setConfigurationWidget(pw);
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
        m_commitWorkingDirectory.clear();
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
    PerforceSubmitEditor *perforceEditor = qobject_cast<PerforceSubmitEditor *>(submitEditor());
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
    const QString path = r.indexIn(response.stdOut) != -1 ? r.cap(1).trimmed() : QString();
    if (Perforce::Constants::debug)
        qDebug() << "clientFilePath" << serverFilePath << path;
    return path;
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

PerforcePlugin::~PerforcePlugin()
{
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
    const QString rc = m_instance->m_settings.mapToFileSystem(p4fileSpec);
    if (Perforce::Constants::debug)
        qDebug() << "fileNameFromPerforceName" << perforceName << p4fileSpec << rc;
    return rc;
}

PerforceVersionControl *PerforcePlugin::perforceVersionControl()
{
    return static_cast<PerforceVersionControl *>(m_instance->versionControl());
}

void PerforcePlugin::slotTopLevelFound(const QString &t)
{
    m_settings.setTopLevel(t);
    const QString msg = tr("Perforce repository: %1").
                        arg(QDir::toNativeSeparators(t));
    VcsOutputWindow::appendSilently(msg);
    if (Perforce::Constants::debug)
        qDebug() << "P4: " << t;
}

void PerforcePlugin::slotTopLevelFailed(const QString &errorMessage)
{
    VcsOutputWindow::appendSilently(tr("Perforce: Unable to determine the repository: %1").arg(errorMessage));
    if (Perforce::Constants::debug)
        qDebug() << errorMessage;
}

void PerforcePlugin::getTopLevel()
{
    // Run a new checker
    if (m_instance->m_settings.p4BinaryPath().isEmpty())
        return;
    PerforceChecker *checker = new PerforceChecker(m_instance);
    connect(checker, SIGNAL(failed(QString)), m_instance, SLOT(slotTopLevelFailed(QString)));
    connect(checker, SIGNAL(failed(QString)), checker, SLOT(deleteLater()));
    connect(checker, SIGNAL(succeeded(QString)), m_instance, SLOT(slotTopLevelFound(QString)));
    connect(checker, SIGNAL(succeeded(QString)),checker, SLOT(deleteLater()));
    checker->start(settings().p4BinaryPath(), settings().commonP4Arguments(QString()), 30000);
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
