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
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextCodec>

#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    Perforce::Constants::PERFORCE_COMMANDLOG_EDITOR_ID,
    Perforce::Constants::PERFORCE_COMMANDLOG_EDITOR_DISPLAY_NAME,
    Perforce::Constants::PERFORCE_COMMANDLOG_EDITOR_CONTEXT,
    "application/vnd.nokia.text.scs_commandlog",
    "scslog"},
{   VCSBase::LogOutput,
    Perforce::Constants::PERFORCE_LOG_EDITOR_ID,
    Perforce::Constants::PERFORCE_LOG_EDITOR_DISPLAY_NAME,
    Perforce::Constants::PERFORCE_LOG_EDITOR_CONTEXT,
    "application/vnd.nokia.text.scs_filelog",
    "scsfilelog"},
{    VCSBase::AnnotateOutput,
     Perforce::Constants::PERFORCE_ANNOTATION_EDITOR_ID,
     Perforce::Constants::PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME,
     Perforce::Constants::PERFORCE_ANNOTATION_EDITOR_CONTEXT,
    "application/vnd.nokia.text.scs_annotation",
    "scsannotate"},
{   VCSBase::DiffOutput,
    Perforce::Constants::PERFORCE_DIFF_EDITOR_ID,
    Perforce::Constants::PERFORCE_DIFF_EDITOR_DISPLAY_NAME,
    Perforce::Constants::PERFORCE_DIFF_EDITOR_CONTEXT,
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromAscii(c->name()) : QString::fromAscii("Null codec");
}

// Ensure adding "..." to relative paths which is p4's convention
// for the current directory
static inline QStringList perforceRelativeFileArguments(const QStringList &args)
{
    if (args.isEmpty())
        return QStringList(QLatin1String("..."));
    QTC_ASSERT(args.size() == 1, return QStringList())
    QStringList p4Args = args;
    p4Args.front() += QLatin1String("/...");
    return p4Args;
}

static inline QStringList perforceRelativeProjectDirectory(const VCSBase::VCSBasePluginState &s)
{
    return perforceRelativeFileArguments(s.relativeCurrentProject());
}

// Clean user setting off diff-binary for 'p4 resolve' and 'p4 diff'.
static inline QProcessEnvironment overrideDiffEnvironmentVariable()
{
    QProcessEnvironment rc = QProcessEnvironment::systemEnvironment();
    rc.remove(QLatin1String("P4DIFF"));
    return rc;
}

static const char * const CMD_ID_PERFORCE_MENU = "Perforce.Menu";
static const char * const CMD_ID_EDIT = "Perforce.Edit";
static const char * const CMD_ID_ADD = "Perforce.Add";
static const char * const CMD_ID_DELETE_FILE = "Perforce.Delete";
static const char * const CMD_ID_OPENED = "Perforce.Opened";
static const char * const CMD_ID_PROJECTLOG = "Perforce.ProjectLog";
static const char * const CMD_ID_REPOSITORYLOG = "Perforce.RepositoryLog";
static const char * const CMD_ID_REVERT = "Perforce.Revert";
static const char * const CMD_ID_DIFF_CURRENT = "Perforce.DiffCurrent";
static const char * const CMD_ID_DIFF_PROJECT = "Perforce.DiffProject";
static const char * const CMD_ID_UPDATE_PROJECT = "Perforce.UpdateProject";
static const char * const CMD_ID_REVERT_PROJECT = "Perforce.RevertProject";
static const char * const CMD_ID_REVERT_UNCHANGED_PROJECT = "Perforce.RevertUnchangedProject";
static const char * const CMD_ID_DIFF_ALL = "Perforce.DiffAll";
static const char * const CMD_ID_RESOLVE = "Perforce.Resolve";
static const char * const CMD_ID_SUBMIT = "Perforce.Submit";
static const char * const CMD_ID_PENDING_CHANGES = "Perforce.PendingChanges";
static const char * const CMD_ID_DESCRIBE = "Perforce.Describe";
static const char * const CMD_ID_ANNOTATE_CURRENT = "Perforce.AnnotateCurrent";
static const char * const CMD_ID_ANNOTATE = "Perforce.Annotate";
static const char * const CMD_ID_FILELOG_CURRENT = "Perforce.FilelogCurrent";
static const char * const CMD_ID_FILELOG = "Perforce.Filelog";
static const char * const CMD_ID_UPDATEALL = "Perforce.UpdateAll";
static const char * const CMD_ID_SEPARATOR1 = "Perforce.Separator1";
static const char * const CMD_ID_SEPARATOR2 = "Perforce.Separator2";
static const char * const CMD_ID_SEPARATOR3 = "Perforce.Separator3";
static const char * const CMD_ID_SEPARATOR4 = "Perforce.Separator4";

////
// PerforcePlugin
////

namespace Perforce {
namespace Internal {

PerforceResponse::PerforceResponse() :
    error(true),
    exitCode(-1)
{
}

PerforcePlugin *PerforcePlugin::m_perforcePluginInstance = NULL;

PerforcePlugin::PerforcePlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(Constants::PERFORCE_SUBMIT_EDITOR_ID)),
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

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Perforce::Constants::SUBMIT_MIMETYPE,
    Perforce::Constants::PERFORCE_SUBMIT_EDITOR_ID,
    Perforce::Constants::PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME,
    Perforce::Constants::PERFORCESUBMITEDITOR_CONTEXT
};

bool PerforcePlugin::initialize(const QStringList & /* arguments */, QString * errorMessage)
{
    typedef VCSBase::VCSEditorFactory<PerforceEditor> PerforceEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<PerforceSubmitEditor> PerforceSubmitEditorFactory;

    VCSBase::VCSBasePlugin::initialize(new PerforceVersionControl(this));

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.perforce/Perforce.mimetypes.xml"), errorMessage))
        return false;
    m_perforcePluginInstance = this;

    if (QSettings *settings = core->settings())
        m_settings.fromSettings(settings);

    addAutoReleasedObject(new SettingsPage);

    // Editor factories
    addAutoReleasedObject(new PerforceSubmitEditorFactory(&submitParameters));

    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new PerforceEditorFactory(editorParameters + i, this, describeSlot));

    //register actions
    Core::ActionManager *am = Core::ICore::instance()->actionManager();

    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mperforce =
        am->createMenu(QLatin1String(CMD_ID_PERFORCE_MENU));
    mperforce->menu()->setTitle(tr("&Perforce"));
    mtools->addMenu(mperforce);
    m_menuAction = mperforce->menu()->menuAction();

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    QList<int> perforcesubmitcontext;
    perforcesubmitcontext << Core::UniqueIDManager::instance()->
            uniqueIdentifier(Constants::PERFORCESUBMITEDITOR_CONTEXT);

    Core::Command *command;
    QAction *tmpaction;

    m_editAction = new Utils::ParameterAction(tr("Edit"), tr("Edit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_editAction, CMD_ID_EDIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+E")));
    command->setDefaultText(tr("Edit File"));
    connect(m_editAction, SIGNAL(triggered()), this, SLOT(openCurrentFile()));
    mperforce->addAction(command);

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_addAction, CMD_ID_ADD, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+A")));
    command->setDefaultText(tr("Add File"));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mperforce->addAction(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_deleteAction, CMD_ID_DELETE_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Delete File"));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    mperforce->addAction(command);

    m_revertFileAction = new Utils::ParameterAction(tr("Revert"), tr("Revert \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_revertFileAction, CMD_ID_REVERT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+R")));
    command->setDefaultText(tr("Revert File"));
    connect(m_revertFileAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Edit"), globalcontext);
    mperforce->addAction(command);

    m_diffFileAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_diffFileAction, CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Diff Current File"));
    connect(m_diffFileAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mperforce->addAction(command);

    const QString diffProjectDefaultText = tr("Diff Current Project/Session");
    m_diffProjectAction = new Utils::ParameterAction(diffProjectDefaultText, tr("Diff Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = am->registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+D")));
    command->setDefaultText(diffProjectDefaultText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffCurrentProject()));
    mperforce->addAction(command);

    m_diffAllAction = new QAction(tr("Diff Opened Files"), this);
    command = am->registerAction(m_diffAllAction, CMD_ID_DIFF_ALL, globalcontext);
    connect(m_diffAllAction, SIGNAL(triggered()), this, SLOT(diffAllOpened()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Diff"), globalcontext);
    mperforce->addAction(command);

    m_openedAction = new QAction(tr("Opened"), this);
    command = am->registerAction(m_openedAction, CMD_ID_OPENED, globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+O")));
    connect(m_openedAction, SIGNAL(triggered()), this, SLOT(printOpenedFileList()));
    mperforce->addAction(command);

    m_logProjectAction = new Utils::ParameterAction(tr("Log Project Log"), tr("Log Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    mperforce->addAction(command);

    m_submitProjectAction = new Utils::ParameterAction(tr("Submit Project"), tr("Submit Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_submitProjectAction, CMD_ID_SUBMIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+S")));
    connect(m_submitProjectAction, SIGNAL(triggered()), this, SLOT(startSubmitProject()));
    mperforce->addAction(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = am->registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, globalcontext);
    connect(m_pendingAction, SIGNAL(triggered()), this, SLOT(printPendingChanges()));
    mperforce->addAction(command);

    const QString updateProjectDefaultText = tr("Update Current Project");
    m_updateProjectAction = new Utils::ParameterAction(updateProjectDefaultText, tr("Update Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = am->registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, globalcontext);
    command->setDefaultText(updateProjectDefaultText);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateCurrentProject()));
    mperforce->addAction(command);

    m_revertProjectAction = new Utils::ParameterAction(tr("Revert Project"), tr("Revert Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_revertProjectAction, CMD_ID_REVERT_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertProjectAction, SIGNAL(triggered()), this, SLOT(revertCurrentProject()));
    mperforce->addAction(command);

    m_revertUnchangedAction = new Utils::ParameterAction(tr("Revert Unchanged"), tr("Revert Unchanged Files of Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_revertUnchangedAction, CMD_ID_REVERT_UNCHANGED_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertUnchangedAction, SIGNAL(triggered()), this, SLOT(revertUnchangedCurrentProject()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Changes"), globalcontext);
    mperforce->addAction(command);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = am->registerAction(m_describeAction, CMD_ID_DESCRIBE, globalcontext);
    connect(m_describeAction, SIGNAL(triggered()), this, SLOT(describeChange()));
    mperforce->addAction(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_annotateCurrentAction, CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Annotate Current File"));
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mperforce->addAction(command);

    m_annotateAction = new QAction(tr("Annotate..."), this);
    command = am->registerAction(m_annotateAction, CMD_ID_ANNOTATE, globalcontext);
    connect(m_annotateAction, SIGNAL(triggered()), this, SLOT(annotate()));
    mperforce->addAction(command);

    m_filelogCurrentAction = new Utils::ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_filelogCurrentAction, CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+F")));
    command->setDefaultText(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this, SLOT(filelogCurrentFile()));
    mperforce->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = am->registerAction(m_filelogAction, CMD_ID_FILELOG, globalcontext);
    connect(m_filelogAction, SIGNAL(triggered()), this, SLOT(filelog()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String(CMD_ID_SEPARATOR4), globalcontext);
    mperforce->addAction(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = am->registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    mperforce->addAction(command);

    m_updateAllAction = new QAction(tr("Update All"), this);
    command = am->registerAction(m_updateAllAction, CMD_ID_UPDATEALL, globalcontext);
    connect(m_updateAllAction, SIGNAL(triggered()), this, SLOT(updateAll()));
    mperforce->addAction(command);

    m_submitCurrentLogAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Submit"), this);
    command = am->registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, perforcesubmitcontext);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFiles = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = am->registerAction(m_diffSelectedFiles, Constants::DIFF_SELECTED, perforcesubmitcontext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = am->registerAction(m_undoAction, Core::Constants::UNDO, perforcesubmitcontext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = am->registerAction(m_redoAction, Core::Constants::REDO, perforcesubmitcontext);

    return true;
}

void PerforcePlugin::extensionsInitialized()
{
    getTopLevel();
}

void PerforcePlugin::openCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsOpen(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)

    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(state.currentFile());
    QStringList args;
    args << QLatin1String("diff") << QLatin1String("-sa") << state.relativeCurrentFile();
    PerforceResponse result = runP4Cmd(state.currentFileTopLevel(), args,
                                       RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow,
                                       QStringList(), QByteArray(), codec);
    if (result.error)
        return;
    // "foo.cpp - file(s) not opened on this client."
    if (result.stdOut.isEmpty() || result.stdOut.contains(QLatin1String(" - ")))
        return;

    const bool doNotRevert = QMessageBox::warning(0, tr("p4 revert"),
                             tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No;
    if (doNotRevert)
        return;

    Core::FileChangeBlocker fcb(state.currentFile());
    fcb.setModifiedReload(true);
    args.clear();
    args << QLatin1String("revert") << state.relativeCurrentFile();
    PerforceResponse result2 = runP4Cmd(state.currentFileTopLevel(), args,
                                        CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (!result2.error)
        perforceVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void PerforcePlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    p4Diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void PerforcePlugin::diffCurrentProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    p4Diff(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePlugin::diffAllOpened()
{
    p4Diff(m_settings.topLevel(), QStringList());
}

void PerforcePlugin::updateCurrentProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    updateCheckout(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePlugin::updateAll()
{
    updateCheckout(m_settings.topLevel());
}

void PerforcePlugin::revertCurrentProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)

    const QString msg = tr("Do you want to revert all changes to the project \"%1\"?").arg(state.currentProjectName());
    if (QMessageBox::warning(0, tr("p4 revert"), msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), false);
}

void PerforcePlugin::revertUnchangedCurrentProject()
{
    // revert -a.
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
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
        foreach(const QString &dir, dirs)
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
    VCSBase::VCSBaseOutputWindow *outWin = VCSBase::VCSBaseOutputWindow::instance();
    QString errorMessage;
    QString mapped;
    const QChar delimiter = QLatin1Char('#');
    foreach (const QString &line, perforceResponse.stdOut.split(QLatin1Char('\n'))) {
        mapped.clear();
        const int delimiterPos = line.indexOf(delimiter);
        if (delimiterPos > 0)
            mapped = fileNameFromPerforceName(line.left(delimiterPos), true, &errorMessage);
        if (mapped.isEmpty()) {
            outWin->appendSilently(line);
        } else {
            outWin->appendSilently(mapped + QLatin1Char(' ') + line.mid(delimiterPos));
        }
    }
    outWin->popup();
}

void PerforcePlugin::startSubmitProject()
{

    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another submit is currently executed."));
        return;
    }

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)

    QTemporaryFile changeTmpFile;
    changeTmpFile.setAutoRemove(false);
    if (!changeTmpFile.open()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot create temporary file."));
        cleanCommitMessageFile();
        return;
    }
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

    m_commitMessageFileName = changeTmpFile.fileName();
    changeTmpFile.write(result.stdOut.toAscii());
    changeTmpFile.close();

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
        if (line.startsWith("... depotFile"))
            depotFileNames.append(line.mid(14));
    }
    if (depotFileNames.isEmpty()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Project has no files"));
        cleanCommitMessageFile();
        return;
    }

    openPerforceSubmitEditor(m_commitMessageFileName, depotFileNames);
}

Core::IEditor *PerforcePlugin::openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName, Constants::PERFORCE_SUBMIT_EDITOR_ID);
    editorManager->ensureEditorManagerVisible();
    PerforceSubmitEditor *submitEditor = static_cast<PerforceSubmitEditor*>(editor);
    submitEditor->restrictToProjectFiles(depotFileNames);
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentLogAction, m_diffSelectedFiles);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(slotSubmitDiff(QStringList)));
    submitEditor->setCheckScriptWorkingDirectory(m_commitWorkingDirectory);
    return editor;
}

void PerforcePlugin::printPendingChanges()
{
    qApp->setOverrideCursor(Qt::WaitCursor);
    PendingChangesDialog dia(pendingChangesData(), Core::ICore::instance()->mainWindow());
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
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePlugin::annotate()
{
    const QString file = QFileDialog::getOpenFileName(0, tr("p4 annotate"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        annotate(fi.absolutePath(), fi.fileName());
    }
}

void PerforcePlugin::annotateVersion(const QString &file, const QString &revision, int lineNumber)
{
    const QFileInfo fi(file);
    annotate(fi.absolutePath(), fi.fileName(), revision, lineNumber);
}

void PerforcePlugin::annotate(const QString &workingDir,
                              const QString &fileName,
                              const QString &changeList /* = QString() */,
                              int lineNumber /* = -1 */)
{
    const QStringList files = QStringList(fileName);
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files, changeList);
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-cqi");
    if (changeList.isEmpty()) {
        args << fileName;
    } else {
        args << (fileName + QLatin1Char('@') + changeList);
    }
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        if (lineNumber < 1)
            lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor();
        const QFileInfo fi(fileName);
        Core::IEditor *ed = showOutputInEditor(tr("p4 annotate %1").arg(id),
                                               result.stdOut, VCSBase::AnnotateOutput,
                                               source, codec);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(ed, lineNumber);
    }
}

void PerforcePlugin::filelogCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    filelog(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void PerforcePlugin::filelog()
{
    const QString file = QFileDialog::getOpenFileName(0, tr("p4 filelog"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        filelog(fi.absolutePath(), QStringList(fi.fileName()));
    }
}

void PerforcePlugin::logProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    filelog(state.currentProjectTopLevel(), perforceRelativeFileArguments(state.relativeCurrentProject()));
}

void PerforcePlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    filelog(state.topLevel(), perforceRelativeFileArguments(QStringList()));
}

void PerforcePlugin::filelog(const QString &workingDir, const QStringList &fileNames,
                             bool enableAnnotationContextMenu)
{
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, fileNames);
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, fileNames);
    QStringList args;
    args << QLatin1String("filelog") << QLatin1String("-li");
    if (m_settings.logCount() > 0)
        args << QLatin1String("-m") << QString::number(m_settings.logCount());
    args.append(fileNames);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, fileNames);
        Core::IEditor *editor = showOutputInEditor(tr("p4 filelog %1").arg(id), result.stdOut,
                                VCSBase::LogOutput, source, codec);
        if (enableAnnotationContextMenu)
            VCSBase::VCSBaseEditor::getVcsBaseEditor(editor)->setFileLogAnnotateEnabled(true);
    }
}

void PerforcePlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!VCSBase::VCSBasePlugin::enableMenuAction(as, m_menuAction))
        return;

    m_logRepositoryAction->setEnabled(currentState().hasTopLevel());

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

bool PerforcePlugin::managesDirectory(const QString &directory)
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
        const QStringList relativeDirArgs = m_settings.relativeToTopLevelArguments(directory);
        if (!relativeDirArgs.empty() && relativeDirArgs.front().startsWith(QLatin1String("..")))
            break;
        // Is it actually managed by perforce?
        QStringList args;
        args << QLatin1String("fstat") << QLatin1String("-m1") << perforceRelativeFileArguments(relativeDirArgs);
        const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args,
                                                 RunFullySynchronous);
        managed = result.stdOut.contains("depotFile") || result.stdErr.contains("... - no such file(s)");
    } while(false);

    m_managedDirectoryCache.insert(directory, managed);
    return managed;
}

QString PerforcePlugin::findTopLevelForDirectory(const QString &dir)
{
    if (!m_settings.isValid())
        return QString();
    return managesDirectory(dir) ? m_settings.topLevelSymLinkTarget() : QString();
}

bool PerforcePlugin::vcsOpen(const QString &workingDir, const QString &fileName)
{
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::vcsOpen" << workingDir << fileName;
    QStringList args;
    args << QLatin1String("edit") << QDir::toNativeSeparators(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
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

static QString formatCommand(const QString &cmd, const QStringList &args)
{
    const QChar blank = QLatin1Char(' ');
    QString command = cmd;
    command += blank;
    command += args.join(QString(blank));
    return PerforcePlugin::tr("Executing: %1\n").arg(command);
}

// Write extra args to temporary file
QSharedPointer<QTemporaryFile>
        PerforcePlugin::createTemporaryArgumentFile(const QStringList &extraArgs) const
{
    if (extraArgs.isEmpty())
        return QSharedPointer<QTemporaryFile>();
    // create pattern
    if (m_tempFilePattern.isEmpty()) {
        m_tempFilePattern = QDir::tempPath();
        if (!m_tempFilePattern.endsWith(QDir::separator()))
            m_tempFilePattern += QDir::separator();
        m_tempFilePattern += QLatin1String("qtc_p4_XXXXXX.args");
    }
    QSharedPointer<QTemporaryFile> rc(new QTemporaryFile(m_tempFilePattern));
    rc->setAutoRemove(true);
    if (!rc->open()) {
        qWarning("Could not create temporary file: %s. Appending all file names to command line.",
                 qPrintable(rc->errorString()));
        return QSharedPointer<QTemporaryFile>();
    }
    const int last = extraArgs.size() - 1;
    for (int i = 0; i <= last; i++) {
        rc->write(extraArgs.at(i).toLocal8Bit());
        if (i != last)
            rc->write("\n");
    }
    rc->close();
    return rc;
}

// Run messages

static inline QString msgNotStarted(const QString &cmd)
{
    return PerforcePlugin::tr("Could not start perforce '%1'. Please check your settings in the preferences.").arg(cmd);
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
                                                    QTextCodec *outputCodec) const
{
    QTC_ASSERT(stdInput.isEmpty(), return PerforceResponse()) // Not supported here

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    // Run, connect stderr to the output window
    Utils::SynchronousProcess process;
    const int timeOut = (flags & LongTimeOut) ? m_settings.longTimeOutMS() : m_settings.timeOutMS();
    process.setTimeout(timeOut);
    process.setStdOutCodec(outputCodec);
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
        connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::run syncp actual args [" << process.workingDirectory() << ']' << args;
    const Utils::SynchronousProcessResponse sp_resp = process.run(m_settings.p4Command(), args);
    if (Perforce::Constants::debug)
        qDebug() << sp_resp;

    PerforceResponse response;
    response.error = true;
    response.exitCode = sp_resp.exitCode;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.message = msgExitCode(sp_resp.exitCode);
        response.error = !(flags & IgnoreExitCode);
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
        response.message = msgCrash();
        break;
    case Utils::SynchronousProcessResponse::StartFailed:
        response.message = msgNotStarted(m_settings.p4Command());
        break;
    case Utils::SynchronousProcessResponse::Hang:
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
                                                         QTextCodec *outputCodec) const
{
    QProcess process;

    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::run fully syncp actual args [" << process.workingDirectory() << ']' << args;

    PerforceResponse response;
    process.start(m_settings.p4Command(), args);
    if (stdInput.isEmpty())
        process.closeWriteChannel();

    if (!process.waitForStarted(3000)) {
        response.error = true;
        response.message = msgNotStarted(m_settings.p4Command());
        return response;
    }
    if (!stdInput.isEmpty()) {
        if (process.write(stdInput) == -1) {
            PerforceChecker::ensureProcessStopped(process);
            response.error = true;
            response.message = tr("Unable to write input data to process %1: %2").arg(m_settings.p4Command(), process.errorString());
            return response;
        }
        process.closeWriteChannel();
    }

    const int timeOut = (flags & LongTimeOut) ? m_settings.longTimeOutMS() : m_settings.timeOutMS();
    if (!process.waitForFinished(timeOut)) {
        PerforceChecker::ensureProcessStopped(process);
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
    response.stdErr = QString::fromLocal8Bit(process.readAllStandardError());
    const QByteArray stdOut = process.readAllStandardOutput();
    response.stdOut = outputCodec ? outputCodec->toUnicode(stdOut.constData(), stdOut.size()) :
                                    QString::fromLocal8Bit(stdOut);
    const QChar cr = QLatin1Char('\r');
    response.stdErr.remove(cr);
    response.stdOut.remove(cr);
    // Logging
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    if ((flags & StdErrToWindow) && !response.stdErr.isEmpty())
        outputWindow->append(response.stdErr);
    if ((flags & StdOutToWindow) && !response.stdOut.isEmpty())
        outputWindow->append(response.stdOut);
    return response;
}

PerforceResponse PerforcePlugin::runP4Cmd(const QString &workingDir,
                                          const QStringList &args,
                                          unsigned flags,
                                          const QStringList &extraArgs,
                                          const QByteArray &stdInput,
                                          QTextCodec *outputCodec) const
{
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::runP4Cmd [" << workingDir << ']' << args << extraArgs << stdInput << debugCodec(outputCodec);

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    if (!m_settings.isValid()) {
        PerforceResponse invalidConfigResponse;
        invalidConfigResponse.error = true;
        invalidConfigResponse.message = tr("Perforce is not correctly configured.");
        outputWindow->appendError(invalidConfigResponse.message);
        return invalidConfigResponse;
    }
    QStringList actualArgs = m_settings.commonP4Arguments(workingDir);
    QSharedPointer<QTemporaryFile> tempFile = createTemporaryArgumentFile(extraArgs);
    if (!tempFile.isNull())
        actualArgs << QLatin1String("-x") << tempFile->fileName();
    actualArgs.append(args);

    if (flags & CommandToWindow)
        outputWindow->appendCommand(formatCommand(m_settings.p4Command(), actualArgs));

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
            outputWindow->appendError(response.message);
    }
    return response;
}

Core::IEditor * PerforcePlugin::showOutputInEditor(const QString& title, const QString output,
                                                   int editorType,
                                                   const QString &source,
                                                   QTextCodec *codec)
{
    const VCSBase::VCSBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const QString id = params->id;
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::showOutputInEditor" << title << id <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::instance()->openEditorWithContents(id, &s, output);
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(annotateVersion(QString,QString,int)));
    PerforceEditor *e = qobject_cast<PerforceEditor*>(editor->widget());
    if (!e)
        return 0;
    e->setSource(source);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    if (codec)
        e->setCodec(codec);
    Core::IEditor *ie = e->editableInterface();
    Core::EditorManager::instance()->activateEditor(ie);
    return ie;
}

void PerforcePlugin::slotSubmitDiff(const QStringList &files)
{
    p4Diff(m_commitWorkingDirectory, files);
}

void PerforcePlugin::p4Diff(const QString &workingDir, const QStringList &files)
{
    Core::IEditor *existingEditor = 0;

    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files);
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);

    // Reuse existing editors for that id
    foreach (Core::IEditor *ed, Core::EditorManager::instance()->openedEditors()) {
        if (ed->file()->property("originalFileName").toString() == id) {
            existingEditor = ed;
            break;
        }
    }
    // Split arguments according to size
    QStringList args;
    args << QLatin1String("diff") << QLatin1String("-du");
    QStringList extraArgs;
    if (files.size() > 1) {
        extraArgs = files;
    } else {
        args.append(files);
    }
    const unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow|OverrideDiffEnvironment;
    const PerforceResponse result = runP4Cmd(workingDir, args, flags,
                                             extraArgs, QByteArray(), codec);
    if (result.error)
        return;

    if (existingEditor) {
        existingEditor->createNew(result.stdOut);
        Core::EditorManager::instance()->activateEditor(existingEditor);
    } else {
        Core::IEditor *editor = showOutputInEditor(tr("p4 diff %1").arg(id), result.stdOut, VCSBase::DiffOutput,
                                                   VCSBase::VCSBaseEditor::getSource(workingDir, files),
                                                   codec);
        editor->file()->setProperty("originalFileName", id);
    }
}

void PerforcePlugin::describe(const QString & source, const QString &n)
{
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VCSBase::VCSBaseEditor::getCodec(source);
    QStringList args;
    args << QLatin1String("describe") << QLatin1String("-du") << n;
    const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args, CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error)
        showOutputInEditor(tr("p4 describe %1").arg(n), result.stdOut, VCSBase::DiffOutput, source, codec);
}

void PerforcePlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    Core::EditorManager *em = Core::EditorManager::instance();
    em->closeEditors(QList<Core::IEditor*>() << em->currentEditor());
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

bool PerforcePlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return true;
    Core::IFile *fileIFace = submitEditor->file();
    const PerforceSubmitEditor *perforceEditor = qobject_cast<PerforceSubmitEditor *>(submitEditor);
    if (!fileIFace || !perforceEditor)
        return true;
    // Prompt the user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool wantsPrompt = m_settings.promptToSubmit();
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            perforceEditor->promptSubmit(tr("Closing p4 Editor"),
                                         tr("Do you want to submit this change list?"),
                                         tr("The commit message check failed. Do you want to submit this change list"),
                                         &wantsPrompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;

    if (answer == VCSBase::VCSBaseSubmitEditor::SubmitCanceled)
        return false;

    // Set without triggering the checking mechanism
    if (wantsPrompt != m_settings.promptToSubmit()) {
        m_settings.setPromptToSubmit(wantsPrompt);
        m_settings.toSettings(Core::ICore::instance()->settings());
    }
    Core::FileManager *fileManager = Core::ICore::instance()->fileManager();
    fileManager->blockFileChange(fileIFace);
    fileIFace->save();
    fileManager->unblockFileChange(fileIFace);
    if (answer == VCSBase::VCSBaseSubmitEditor::SubmitDiscarded) {
        cleanCommitMessageFile();
        return true;
    }
    // Pipe file into p4 submit -i
    QFile commitMessageFile(m_commitMessageFileName);
    if (!commitMessageFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot open temporary file."));
        return false;
    }

    const QByteArray changeDescription = commitMessageFile.readAll();
    commitMessageFile.close();
    QStringList submitArgs;
    submitArgs << QLatin1String("submit") << QLatin1String("-i");
    const PerforceResponse submitResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), submitArgs,
                                                     LongTimeOut|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow|ShowBusyCursor,
                                                     QStringList(), changeDescription);
    if (submitResponse.error) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("p4 submit failed: %1").arg(submitResponse.message));
        return false;
    }
    VCSBase::VCSBaseOutputWindow::instance()->append(submitResponse.stdOut);
    if (submitResponse.stdOut.contains(QLatin1String("Out of date files must be resolved or reverted)")))
        QMessageBox::warning(submitEditor->widget(), tr("Pending change"), tr("Could not submit the change, because your workspace was out of date. Created a pending submit instead."));

    cleanCommitMessageFile();
    return true;
}

QString PerforcePlugin::clientFilePath(const QString &serverFilePath)
{
    QTC_ASSERT(m_settings.isValid(), return QString())

    QStringList args;
    args << QLatin1String("fstat") << serverFilePath;
    const PerforceResponse response = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               ShowBusyCursor|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (response.error)
        return QString::null;

    QRegExp r(QLatin1String("\\.\\.\\.\\sclientFile\\s(.+)\n"));
    r.setMinimal(true);
    const QString path = r.indexIn(response.stdOut) != -1 ? r.cap(1).trimmed() : QString();
    if (Perforce::Constants::debug)
        qDebug() << "clientFilePath" << serverFilePath << path;
    return path;
}

QString PerforcePlugin::pendingChangesData()
{
    QTC_ASSERT(m_settings.isValid(), return QString())

    QStringList args = QStringList(QLatin1String("info"));
    const PerforceResponse userResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (userResponse.error)
        return QString::null;

    QRegExp r(QLatin1String("User\\sname:\\s(\\S+)\\s*\n"));
    QTC_ASSERT(r.isValid(), return QString())
    r.setMinimal(true);
    const QString user = r.indexIn(userResponse.stdOut) != -1 ? r.cap(1).trimmed() : QString::null;
    if (user.isEmpty())
        return QString::null;
    args.clear();
    args << QLatin1String("changes") << QLatin1String("-s") << QLatin1String("pending") << QLatin1String("-u") << user;
    const PerforceResponse dataResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                                   RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    return dataResponse.error ? QString::null : dataResponse.stdOut;
}

PerforcePlugin::~PerforcePlugin()
{
}

const PerforceSettings& PerforcePlugin::settings() const
{
    return m_settings;
}

void PerforcePlugin::setSettings(const Settings &newSettings)
{
    if (newSettings != m_settings.settings()) {
        m_settings.setSettings(newSettings);
        m_managedDirectoryCache.clear();
        m_settings.toSettings(Core::ICore::instance()->settings());
        getTopLevel();
    }
}

static inline QString msgWhereFailed(const QString & file, const QString &why)
{
    return PerforcePlugin::tr("Error running \"where\" on %1: %2").arg(file, why);
}

// Map a perforce name "//xx" to its real name in the file system
QString PerforcePlugin::fileNameFromPerforceName(const QString& perforceName,
                                                 bool quiet,
                                                 QString *errorMessage) const
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
    const PerforceResponse response = runP4Cmd(m_settings.topLevelSymLinkTarget(), args, flags);
    if (response.error) {
        *errorMessage = msgWhereFailed(perforceName, response.message);
        return QString::null;
    }

    QString output = response.stdOut;
    if (output.endsWith(QLatin1Char('\r')))
        output.chop(1);
    if (output.endsWith(QLatin1Char('\n')))
        output.chop(1);

    if (output.isEmpty()) {
        *errorMessage = msgWhereFailed(perforceName, tr("The file is not mapped"));
        return QString();
    }
    const QString p4fileSpec = output.mid(output.lastIndexOf(QLatin1Char(' ')) + 1);
    const QString rc = m_settings.mapToFileSystem(p4fileSpec);
    if (Perforce::Constants::debug)
        qDebug() << "fileNameFromPerforceName" << perforceName << p4fileSpec << rc;
    return rc;
}

PerforcePlugin *PerforcePlugin::perforcePluginInstance()
{
    QTC_ASSERT(m_perforcePluginInstance, return 0);
    return m_perforcePluginInstance;
}

PerforceVersionControl *PerforcePlugin::perforceVersionControl() const
{
    return static_cast<PerforceVersionControl *>(versionControl());
}

void PerforcePlugin::slotTopLevelFound(const QString &t)
{
    m_settings.setTopLevel(t);
    VCSBase::VCSBaseOutputWindow::instance()->appendSilently(tr("Perforce repository: %1").arg(t));
    if (Perforce::Constants::debug)
        qDebug() << "P4: " << t;
}

void PerforcePlugin::slotTopLevelFailed(const QString &errorMessage)
{
    VCSBase::VCSBaseOutputWindow::instance()->appendSilently(tr("Perforce: Unable to determine the repository: %1").arg(errorMessage));
    if (Perforce::Constants::debug)
        qDebug() << errorMessage;
}

void PerforcePlugin::getTopLevel()
{
    // Run a new checker
    if (m_settings.p4Command().isEmpty())
        return;
    PerforceChecker *checker = new PerforceChecker(this);
    connect(checker, SIGNAL(failed(QString)), this, SLOT(slotTopLevelFailed(QString)));
    connect(checker, SIGNAL(failed(QString)), checker, SLOT(deleteLater()));
    connect(checker, SIGNAL(succeeded(QString)), this, SLOT(slotTopLevelFound(QString)));
    connect(checker, SIGNAL(succeeded(QString)),checker, SLOT(deleteLater()));
    checker->start(m_settings.p4Command(), m_settings.commonP4Arguments(QString()), 30000);
}

}
}

Q_EXPORT_PLUGIN(Perforce::Internal::PerforcePlugin)
