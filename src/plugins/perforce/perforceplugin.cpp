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
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

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

using namespace Perforce::Internal;

enum { p4Timeout = 20000 };

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    "Perforce Command Log Editor",
    Perforce::Constants::C_PERFORCEEDITOR,
    "application/vnd.nokia.text.scs_commandlog",
    "scslog"},
{   VCSBase::LogOutput,
    "Perforce File Log Editor",
    Perforce::Constants::C_PERFORCEEDITOR,
    "application/vnd.nokia.text.scs_filelog",
    "scsfilelog"},
{    VCSBase::AnnotateOutput,
    "Perforce Annotation Editor",
    Perforce::Constants::C_PERFORCEEDITOR,
    "application/vnd.nokia.text.scs_annotation",
    "scsannotate"},
{   VCSBase::DiffOutput,
    "Perforce Diff Editor",
    Perforce::Constants::C_PERFORCEEDITOR,
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

// Return the project files relevant for VCS
static const QStringList currentProjectFiles(QString *name)
{
    QStringList files = VCSBase::VCSBaseSubmitEditor::currentProjectFiles(true, name);
    if (!files.empty()) {
        // Filter out mkspecs/qconfig.pri
        QString exclusion = QLatin1String("mkspecs");
        exclusion += QDir::separator();
        exclusion += QLatin1String("qconfig.pri");
        for (QStringList::iterator it = files.begin(); it != files.end(); ) {
            if (it->endsWith(exclusion)) {
                it = files.erase(it);
                break;
            } else {
                ++it;
            }
        }
    }
    return files;
}

static const char * const CMD_ID_PERFORCE_MENU = "Perforce.Menu";
static const char * const CMD_ID_EDIT = "Perforce.Edit";
static const char * const CMD_ID_ADD = "Perforce.Add";
static const char * const CMD_ID_DELETE_FILE = "Perforce.Delete";
static const char * const CMD_ID_OPENED = "Perforce.Opened";
static const char * const CMD_ID_REVERT = "Perforce.Revert";
static const char * const CMD_ID_DIFF_CURRENT = "Perforce.DiffCurrent";
static const char * const CMD_ID_DIFF_PROJECT = "Perforce.DiffProject";
static const char * const CMD_ID_UPDATE_PROJECT = "Perforce.UpdateProject";
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

////
// CoreListener
////

bool CoreListener::editorAboutToClose(Core::IEditor *editor)
{
    return m_plugin->editorAboutToClose(editor);
}

////
// PerforcePlugin
////

PerforcePlugin *PerforcePlugin::m_perforcePluginInstance = NULL;

PerforcePlugin::PerforcePlugin() :
    m_editAction(0),
    m_addAction(0),
    m_deleteAction(0),
    m_openedAction(0),
    m_revertAction(0),
    m_diffCurrentAction(0),
    m_diffProjectAction(0),
    m_updateProjectAction(0),
    m_diffAllAction(0),
    m_resolveAction(0),
    m_submitAction(0),
    m_pendingAction(0),
    m_describeAction(0),
    m_annotateCurrentAction(0),
    m_annotateAction(0),
    m_filelogCurrentAction(0),
    m_filelogAction(0),
    m_submitCurrentLogAction(0),
    m_updateAllAction(0),
    m_submitActionTriggered(false),
    m_diffSelectedFiles(0),
    m_undoAction(0),
    m_redoAction(0),
    m_versionControl(0)
{
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Perforce::Constants::SUBMIT_MIMETYPE,
    Perforce::Constants::PERFORCESUBMITEDITOR_KIND,
    Perforce::Constants::C_PERFORCESUBMITEDITOR
};

bool PerforcePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    typedef VCSBase::VCSEditorFactory<PerforceEditor> PerforceEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<PerforceSubmitEditor> PerforceSubmitEditorFactory;

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

    m_versionControl = new PerforceVersionControl(this);
    addAutoReleasedObject(m_versionControl);

    addAutoReleasedObject(new CoreListener(this));

    //register actions
    Core::ActionManager *am = Core::ICore::instance()->actionManager();

    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mperforce =
        am->createMenu(QLatin1String(CMD_ID_PERFORCE_MENU));
    mperforce->menu()->setTitle(tr("&Perforce"));
    mtools->addMenu(mperforce);
    if (QAction *ma = mperforce->menu()->menuAction()) {
        ma->setEnabled(m_versionControl->isEnabled());
        connect(m_versionControl, SIGNAL(enabledChanged(bool)), ma, SLOT(setVisible(bool)));
    }

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    QList<int> perforcesubmitcontext;
    perforcesubmitcontext << Core::UniqueIDManager::instance()->
            uniqueIdentifier(Constants::C_PERFORCESUBMITEDITOR);

    Core::Command *command;
    QAction *tmpaction;

    m_editAction = new Utils::ParameterAction(tr("Edit"), tr("Edit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_editAction, CMD_ID_EDIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+E")));
#endif
    command->setDefaultText(tr("Edit File"));
    connect(m_editAction, SIGNAL(triggered()), this, SLOT(openCurrentFile()));
    mperforce->addAction(command);

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_addAction, CMD_ID_ADD, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+A")));
#endif
    command->setDefaultText(tr("Add File"));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mperforce->addAction(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete"), tr("Delete \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_deleteAction, CMD_ID_DELETE_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Delete File"));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentFile()));
    mperforce->addAction(command);

    m_revertAction = new Utils::ParameterAction(tr("Revert"), tr("Revert \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_revertAction, CMD_ID_REVERT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+R")));
#endif
    command->setDefaultText(tr("Revert File"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Edit"), globalcontext);
    mperforce->addAction(command);

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = am->registerAction(m_diffCurrentAction, CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Diff Current File"));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mperforce->addAction(command);

    const QString diffProjectDefaultText = tr("Diff Current Project/Session");
    m_diffProjectAction = new Utils::ParameterAction(diffProjectDefaultText, tr("Diff Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = am->registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+D")));
#endif
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
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+O")));
#endif
    connect(m_openedAction, SIGNAL(triggered()), this, SLOT(printOpenedFileList()));
    mperforce->addAction(command);

    m_submitAction = new QAction(tr("Submit Project"), this);
    command = am->registerAction(m_submitAction, CMD_ID_SUBMIT, globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+S")));
    connect(m_submitAction, SIGNAL(triggered()), this, SLOT(submit()));
    mperforce->addAction(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = am->registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, globalcontext);
    connect(m_pendingAction, SIGNAL(triggered()), this, SLOT(printPendingChanges()));
    mperforce->addAction(command);

    const QString updateProjectDefaultText = tr("Update Current Project/Session");
    m_updateProjectAction = new Utils::ParameterAction(updateProjectDefaultText, tr("Update Project \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = am->registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, globalcontext);
    command->setDefaultText(updateProjectDefaultText);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateCurrentProject()));
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
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+F")));
#endif
    command->setDefaultText(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this, SLOT(filelogCurrentFile()));
    mperforce->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = am->registerAction(m_filelogAction, CMD_ID_FILELOG, globalcontext);
    connect(m_filelogAction, SIGNAL(triggered()), this, SLOT(filelog()));
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

    connect(core, SIGNAL(contextChanged(Core::IContext *)),
        this, SLOT(updateActions()));

    connect(core->fileManager(), SIGNAL(currentFileChanged(const QString &)),
        this, SLOT(updateActions()));

    return true;
}

void PerforcePlugin::extensionsInitialized()
{
    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (m_projectExplorer) {
        connect(m_projectExplorer,
            SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(updateActions()));
    }
    updateActions();
}

void PerforcePlugin::openCurrentFile()
{
    vcsOpen(currentFileName());
}

void PerforcePlugin::addCurrentFile()
{
    vcsAdd(currentFileName());
}

void PerforcePlugin::deleteCurrentFile()
{
    vcsDelete(currentFileName());
}

void PerforcePlugin::revertCurrentFile()
{
    const QString fileName = currentFileName();
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(fileName);
    QStringList args;
    args << QLatin1String("diff") << QLatin1String("-sa");
    PerforceResponse result = runP4Cmd(args, QStringList(), CommandToWindow|StdErrToWindow|ErrorToWindow, codec);
    if (result.error)
        return;

    if (!result.stdOut.isEmpty()) {
        bool doNotRevert = QMessageBox::warning(0, tr("p4 revert"),
                                                tr("The file has been changed. Do you want to revert it?"),
                                                QMessageBox::Yes, QMessageBox::No)
                            == QMessageBox::No;
        if (doNotRevert)
            return;
    }

    Core::FileChangeBlocker fcb(fileName);
    fcb.setModifiedReload(true);
    PerforceResponse result2 = runP4Cmd(QStringList() << QLatin1String("revert") << fileName, QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
}

void PerforcePlugin::diffCurrentFile()
{
    p4Diff(QStringList(currentFileName()));
}

void PerforcePlugin::diffCurrentProject()
{
    QString name;
    const QStringList nativeFiles = currentProjectFiles(&name);
    p4Diff(nativeFiles, name);
}

void PerforcePlugin::diffAllOpened()
{
    p4Diff(QStringList());
}

// Add the directory of a project to a list of p4 directory specifications
// ("path/...")
static inline void addProjectP4Directory(const ProjectExplorer::Project *p, QStringList *p4dirs)
{
    if (const Core::IFile *file =  p->file()) {
        const QFileInfo fi(file->fileName());
        QString p4dir = fi.absolutePath();
        if (!p4dir.isEmpty()) {
            p4dir += QDir::separator();
            p4dir += QLatin1String("...");
            p4dirs->push_back(p4dir);
        }
    }
}

void PerforcePlugin::updateCurrentProject()
{
    if (!m_projectExplorer)
        return;
    // Compile a list of project directories
    QStringList p4Directories;
    if (const ProjectExplorer::Project *proj = m_projectExplorer->currentProject()) {
        addProjectP4Directory(proj, &p4Directories);
    } else {
        if (const ProjectExplorer::SessionManager *session = m_projectExplorer->session())
            foreach(const ProjectExplorer::Project *proj, session->projects())
                addProjectP4Directory(proj, &p4Directories);
    }
    if (!p4Directories.empty())
        updateCheckout(p4Directories);
}

void PerforcePlugin::updateAll()
{
    updateCheckout();
}

void PerforcePlugin::updateCheckout(const QStringList &dirs)
{
    QStringList args(QLatin1String("sync"));
    args.append(dirs);
    runP4Cmd(args, QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
}

void PerforcePlugin::printOpenedFileList()
{
    Core::IEditor *e = Core::EditorManager::instance()->currentEditor();
    if (e)
        e->widget()->setFocus();
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("opened"), QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
}

void PerforcePlugin::submit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    QString errorMessage;
    if (!checkP4Configuration(&errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return;
    }

    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another submit is currently executed."));
        return;
    }

    QTemporaryFile changeTmpFile;
    changeTmpFile.setAutoRemove(false);
    if (!changeTmpFile.open()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot create temporary file."));
        cleanCommitMessageFile();
        return;
    }

    PerforceResponse result = runP4Cmd(QStringList()<< QLatin1String("change") << QLatin1String("-o"), QStringList(),
                                       CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result.error) {
        cleanCommitMessageFile();
        return;
    }

    m_commitMessageFileName = changeTmpFile.fileName();
    changeTmpFile.write(result.stdOut.toAscii());
    changeTmpFile.close();

    // Assemble file list of project
    QString name;
    const QStringList nativeFiles = currentProjectFiles(&name);
    PerforceResponse result2 = runP4Cmd(QStringList(QLatin1String("fstat")), nativeFiles,
                                        CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result2.error) {
        cleanCommitMessageFile();
        return;
    }

    QStringList stdOutLines = result2.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    foreach (const QString &line, stdOutLines) {
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
    Core::IEditor *editor = editorManager->openEditor(fileName, Constants::PERFORCESUBMITEDITOR_KIND);
    editorManager->ensureEditorManagerVisible();
    PerforceSubmitEditor *submitEditor = static_cast<PerforceSubmitEditor*>(editor);
    submitEditor->restrictToProjectFiles(depotFileNames);
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentLogAction, m_diffSelectedFiles);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(slotDiff(QStringList)));
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
        runP4Cmd(args, QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
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
    const QString file = currentFileName();
    if (!file.isEmpty())
        annotate(file);
}

void PerforcePlugin::annotate()
{
    const QString file = QFileDialog::getOpenFileName(0, tr("p4 annotate"), currentFileName());
    if (!file.isEmpty())
        annotate(file);
}

void PerforcePlugin::annotate(const QString &fileName)
{
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(fileName);
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-cqi") << fileName;
    const PerforceResponse result = runP4Cmd(args, QStringList(),
                                             CommandToWindow|StdErrToWindow|ErrorToWindow, codec);
    if (!result.error) {
        const int lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor(fileName);
        const QFileInfo fi(fileName);
        Core::IEditor *ed = showOutputInEditor(tr("p4 annotate %1").arg(fi.fileName()),
                                               result.stdOut, VCSBase::AnnotateOutput, codec);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(ed, lineNumber);
    }
}

void PerforcePlugin::filelogCurrentFile()
{
    const QString file = currentFileName();
    if (!file.isEmpty())
        filelog(file);
}

void PerforcePlugin::filelog()
{
    const QString file = QFileDialog::getOpenFileName(0, tr("p4 filelog"), currentFileName());
    if (!file.isEmpty())
        filelog(file);
}

void PerforcePlugin::filelog(const QString &fileName)
{
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(fileName);
    QStringList args;
    args << QLatin1String("filelog") << QLatin1String("-li") << fileName;
    const PerforceResponse result = runP4Cmd(args, QStringList(),
                                             CommandToWindow|StdErrToWindow|ErrorToWindow, codec);
    if (!result.error) {
        const QFileInfo fi(fileName);
        showOutputInEditor(tr("p4 filelog %1").arg(fi.fileName()),
            result.stdOut, VCSBase::LogOutput, codec);
    }
}

void PerforcePlugin::updateActions()
{
    const QString fileName = currentFileName();
    const QString baseName = fileName.isEmpty() ? fileName : QFileInfo(fileName).fileName();

    m_editAction->setParameter(baseName);
    m_addAction->setParameter(baseName);
    m_deleteAction->setParameter(baseName);
    m_revertAction->setParameter(baseName);
    m_diffCurrentAction->setParameter(baseName);
    m_annotateCurrentAction->setParameter(baseName);
    m_filelogCurrentAction->setParameter(baseName);

    // The project actions refer to the current project or the projects
    // of a session, provided there are any.
    if (m_projectExplorer && m_projectExplorer->currentProject()) {
        const QString currentProjectName = m_projectExplorer->currentProject()->name();
        m_updateProjectAction->setParameter(currentProjectName);
        m_diffProjectAction->setParameter(currentProjectName);
        m_updateProjectAction->setEnabled(true);
        m_diffProjectAction->setEnabled(true);
    } else {
        // Nope, either all projects of a session or none
        m_updateProjectAction->setParameter(QString());
        m_diffProjectAction->setParameter(QString());
        const bool enabled = m_projectExplorer && m_projectExplorer->session() && !m_projectExplorer->session()->projects().empty();
        m_updateProjectAction->setEnabled(enabled);
        m_diffProjectAction->setEnabled(enabled);
    }

    m_diffAllAction->setEnabled(true);
    m_openedAction->setEnabled(true);
    m_describeAction->setEnabled(true);
    m_annotateAction->setEnabled(true);
    m_filelogAction->setEnabled(true);
    m_pendingAction->setEnabled(true);
    m_updateAllAction->setEnabled(true);
}

bool PerforcePlugin::managesDirectory(const QString &directory) const
{
    if (!checkP4Configuration())
        return false;
    const QString p4Path = directory + QLatin1String("/...");
    QStringList args;
    args << QLatin1String("fstat") << QLatin1String("-m1") << p4Path;

    const PerforceResponse result = runP4Cmd(args, QStringList(), 0u);
    return result.stdOut.contains("depotFile") || result.stdErr.contains("... - no such file(s)");
}

QString PerforcePlugin::findTopLevelForDirectory(const QString & /* dir */) const
{
    // First check with p4 client -o
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("client") << QLatin1String("-o"), QStringList(), 0u);
    if (result.error)
        return QString::null;

    QRegExp regExp(QLatin1String("(\\n|\\r\\n|\\r)Root:\\s*(.*)(\\n|\\r\\n|\\r)"));
    QTC_ASSERT(regExp.isValid(), /**/);
    regExp.setMinimal(true);
    if (regExp.indexIn(result.stdOut) != -1) {
        QString file = regExp.cap(2).trimmed();
        if (QFileInfo(file).exists())
            return file;
    }
    return QString::null;
}

bool PerforcePlugin::vcsOpen(const QString &fileName)
{
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("edit") << QDir::toNativeSeparators(fileName), QStringList(),
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !result.error;
}

bool PerforcePlugin::vcsAdd(const QString &fileName)
{
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("add") << fileName, QStringList(),
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !result.error;
}

bool PerforcePlugin::vcsDelete(const QString &fileName)
{
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("revert") << fileName, QStringList(),
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    PerforceResponse result2 = runP4Cmd(QStringList() << QLatin1String("delete") << fileName, QStringList(),
                                        CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    // TODO need to carefully parse the actual messages from perforce
    // or do a fstat before to decide what to do

    // Different states are:
    // File is in depot and unopened => p4 delete %
    // File is in depot and opened => p4 revert %, p4 delete %
    // File is not in depot => p4 revert %
    return !(result.error && result2.error);
}

static QString formatCommand(const QString &cmd, const QStringList &args)
{
    const QChar blank = QLatin1Char(' ');
    QString command = cmd;
    command += blank;
    command += args.join(QString(blank));
    return PerforcePlugin::tr("Executing: %1\n").arg(command);
}

PerforceResponse PerforcePlugin::runP4Cmd(const QStringList &args,
                                          const QStringList &extraArgs,
                                          unsigned logFlags,
                                          QTextCodec *outputCodec) const
{
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::runP4Cmd" << args << extraArgs << debugCodec(outputCodec);
    PerforceResponse response;
    response.error = true;
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    if (!checkP4Configuration(&response.message)) {
        outputWindow->appendError(response.message);
        return response;
    }

    // handle extra args
    QTemporaryFile tempfile;
    tempfile.setAutoRemove(true);
    const QChar newLine = QLatin1Char('\n');
    const QChar blank = QLatin1Char(' ');
    QStringList actualArgs = m_settings.basicP4Args();
    if (!extraArgs.isEmpty()) {
        if (tempfile.open()) {
            QTextStream stream(&tempfile);
            stream << extraArgs.join(QString(newLine));
            actualArgs << QLatin1String("-x") << tempfile.fileName();
            tempfile.close();
        } else {
            qWarning()<<"Could not create temporary file. Appending all file names to command line.";
            actualArgs << extraArgs;
        }
    }
    actualArgs << args;

    if (logFlags & CommandToWindow)
        outputWindow->appendCommand(formatCommand(m_settings.p4Command(), actualArgs));

    // Run, connect stderr to the output window
    Utils::SynchronousProcess process;
    process.setTimeout(p4Timeout);
    process.setStdOutCodec(outputCodec);
    process.setEnvironment(environment());

    // connect stderr to the output window if desired
    if (logFlags & StdErrToWindow) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }

    // connect stdout to the output window if desired
    if (logFlags & StdOutToWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }

    const Utils::SynchronousProcessResponse sp_resp = process.run(m_settings.p4Command(), actualArgs);
    if (Perforce::Constants::debug)
        qDebug() << sp_resp;

    response.error = true;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.message = tr("The process terminated with exit code %1.").arg(sp_resp.exitCode);
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
        response.message = tr("The process terminated abnormally.");
        break;
    case Utils::SynchronousProcessResponse::StartFailed:
        response.message = tr("Could not start perforce '%1'. Please check your settings in the preferences.").arg(m_settings.p4Command());
        break;
    case Utils::SynchronousProcessResponse::Hang:
        response.message = tr("Perforce did not respond within timeout limit (%1 ms).").arg(p4Timeout );
        break;
    }
    if (response.error) {
        if (Perforce::Constants::debug)
            qDebug() << response.message;
        if (logFlags & ErrorToWindow)
            outputWindow->appendError(response.message);
    }
    return response;
}

Core::IEditor * PerforcePlugin::showOutputInEditor(const QString& title, const QString output,
                                                   int editorType, QTextCodec *codec)
{
    const VCSBase::VCSBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const QString kind = QLatin1String(params->kind);
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::showOutputInEditor" << title << kind <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::instance()->openEditorWithContents(kind, &s, output);
    PerforceEditor *e = qobject_cast<PerforceEditor*>(editor->widget());
    if (!e)
        return 0;
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    if (codec)
        e->setCodec(codec);
    Core::IEditor *ie = e->editableInterface();
    Core::EditorManager::instance()->activateEditor(ie);
    return ie;
}

QStringList PerforcePlugin::environment() const
{
    QStringList newEnv = QProcess::systemEnvironment();
    const QString name = "P4DIFF";
    for (int i = 0; i < newEnv.count(); ++i) {
        if (newEnv.at(i).startsWith(name)) {
            newEnv.removeAt(i);
            return newEnv;
        }
    }
    return newEnv;
}

void PerforcePlugin::slotDiff(const QStringList &files)
{
    p4Diff(files);
}

void PerforcePlugin::p4Diff(const QStringList &files, QString diffname)
{
    Core::IEditor *editor = 0;
    bool displayInEditor = true;
    Core::IEditor *existingEditor = 0;
    QTextCodec *codec = files.empty() ? static_cast<QTextCodec *>(0) : VCSBase::VCSBaseEditor::getCodec(files.front());
    if (Perforce::Constants::debug)
        qDebug() << Q_FUNC_INFO << files << debugCodec(codec);

    // diff of a single file? re-use an existing view if possible to support the common
    // usage pattern of continuously changing and diffing a file
    if (files.count() == 1) {
        const QString fileName = files.at(0);
        if (diffname.isEmpty()) {
            const QFileInfo fi(fileName);
            diffname = fi.fileName();
        }

        foreach (Core::IEditor *ed, Core::EditorManager::instance()->openedEditors()) {
            if (ed->file()->property("originalFileName").toString() == fileName) {
                existingEditor = ed;
                displayInEditor = false;
                break;
            }
        }
    }

    const PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("diff") << QLatin1String("-du"), files, CommandToWindow|StdErrToWindow|ErrorToWindow, codec);
    if (result.error)
        return;

    if (displayInEditor)
        editor = showOutputInEditor(tr("p4 diff %1").arg(diffname), result.stdOut, VCSBase::DiffOutput, codec);


    if (files.count() == 1) {
        if (displayInEditor && editor != 0) {
            editor->file()->setProperty("originalFileName", files.at(0));
        } else if (!displayInEditor && existingEditor) {
            if (existingEditor) {
                existingEditor->createNew(result.stdOut);
                Core::EditorManager::instance()->activateEditor(existingEditor);
            }
        }
    }
}

void PerforcePlugin::describe(const QString & source, const QString &n)
{
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VCSBase::VCSBaseEditor::getCodec(source);
    QStringList args;
    args << QLatin1String("describe") << QLatin1String("-du") << n;
    const PerforceResponse result = runP4Cmd(args, QStringList(), CommandToWindow|StdErrToWindow|ErrorToWindow, codec);
    if (!result.error)
        showOutputInEditor(tr("p4 describe %1").arg(n), result.stdOut, VCSBase::DiffOutput, codec);
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
    }
}

bool PerforcePlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

bool PerforcePlugin::editorAboutToClose(Core::IEditor *editor)
{
    if (!editor || !isCommitEditorOpen())
        return true;
    Core::ICore *core = Core::ICore::instance();
    Core::IFile *fileIFace = editor->file();
    if (!fileIFace)
        return true;
    const PerforceSubmitEditor *perforceEditor = qobject_cast<PerforceSubmitEditor *>(editor);
    if (!perforceEditor)
        return true;
    QFileInfo editorFile(fileIFace->fileName());
    QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() == changeFile.absoluteFilePath()) {
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
        core->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        core->fileManager()->unblockFileChange(fileIFace);
        if (answer == VCSBase::VCSBaseSubmitEditor::SubmitConfirmed) {
            QFile commitMessageFile(m_commitMessageFileName);
            if (!commitMessageFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
                VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot open temporary file."));
                return false;
            }
            QByteArray change = commitMessageFile.readAll();
            commitMessageFile.close();
            QString errorMessage;
            if (!checkP4Configuration(&errorMessage)) {
                VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
                return false;
            }
            QProcess proc;
            proc.setEnvironment(environment());

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            QStringList submitArgs = m_settings.basicP4Args();
            submitArgs << QLatin1String("submit") << QLatin1String("-i");
            VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(m_settings.p4Command(), submitArgs));
            proc.start(m_settings.p4Command(),submitArgs);
            if (!proc.waitForStarted(p4Timeout)) {
                VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot execute p4 submit."));
                QApplication::restoreOverrideCursor();
                return false;
            }
            proc.write(change);
            proc.closeWriteChannel();

            if (!proc.waitForFinished()) {
                VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot execute p4 submit."));
                QApplication::restoreOverrideCursor();
                return false;
            }
            const int exitCode = proc.exitCode();
            if (exitCode) {
                VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("p4 submit failed (exit code %1).").arg(exitCode));
                QApplication::restoreOverrideCursor();
                return false;
            }
            const QString output = QString::fromLocal8Bit(proc.readAll());
            VCSBase::VCSBaseOutputWindow::instance()->append(output);
            if (output.contains(QLatin1String("Out of date files must be resolved or reverted)"))) {
                QMessageBox::warning(editor->widget(), tr("Pending change"), tr("Could not submit the change, because your workspace was out of date. Created a pending submit instead."));
            }
            QApplication::restoreOverrideCursor();
        }
        cleanCommitMessageFile();
    }
    return true;
}

void PerforcePlugin::openFiles(const QStringList &files)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    foreach (const QString &s, files)
        em->openEditor(clientFilePath(s));
    em->ensureEditorManagerVisible();
}

QString PerforcePlugin::clientFilePath(const QString &serverFilePath)
{
    if (!checkP4Configuration())
        return QString();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QProcess proc;
    proc.setEnvironment(environment());
    proc.start(m_settings.p4Command(),
        m_settings.basicP4Args() << QLatin1String("fstat") << serverFilePath);

    QString path;
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        if (!output.isEmpty()) {
            QRegExp r(QLatin1String("\\.\\.\\.\\sclientFile\\s(.+)\n"));
            r.setMinimal(true);
            if (r.indexIn(output) != -1)
                path = r.cap(1).trimmed();
        }
    }
    QApplication::restoreOverrideCursor();
    return path;
}

QString PerforcePlugin::currentFileName()
{
    QString fileName = Core::ICore::instance()->fileManager()->currentFile();

    // TODO: Use FileManager::fixPath
    const QFileInfo fileInfo(fileName);
    if (fileInfo.exists())
        fileName = fileInfo.absoluteFilePath();
    fileName = QDir::toNativeSeparators(fileName);
    return fileName;
}

bool PerforcePlugin::checkP4Configuration(QString *errorMessage /* = 0 */) const
{
    if (m_settings.isValid())
        return true;
    if (errorMessage)
        *errorMessage = tr("Invalid configuration: %1").arg(m_settings.errorString());
    return false;
}

QString PerforcePlugin::pendingChangesData()
{
    QString data;
    if (!checkP4Configuration())
        return data;

    QString user;
    QProcess proc;
    proc.setEnvironment(environment());
    proc.start(m_settings.p4Command(),
        m_settings.basicP4Args() << QLatin1String("info"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        if (!output.isEmpty()) {
            QRegExp r(QLatin1String("User\\sname:\\s(\\S+)\\s*\n"));
            r.setMinimal(true);
            if (r.indexIn(output) != -1)
                user = r.cap(1).trimmed();
        }
    }
    if (user.isEmpty())
        return data;
    proc.start(m_settings.p4Command(),
        m_settings.basicP4Args() << QLatin1String("changes") << QLatin1String("-s") << QLatin1String("pending") << QLatin1String("-u") << user);
    if (proc.waitForFinished(3000))
        data = QString::fromUtf8(proc.readAllStandardOutput());
    return data;
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
        m_settings.toSettings(Core::ICore::instance()->settings());
    }
}

// Map a perforce name "//xx" to its real name in the file system
QString PerforcePlugin::fileNameFromPerforceName(const QString& perforceName,
                                                 QString *errorMessage) const
{
    // All happy, already mapped
    if (!perforceName.startsWith(QLatin1String("//")))
        return perforceName;
    // "where" remaps the file to client file tree
    QProcess proc;
    QStringList args(m_settings.basicP4Args());
    args << QLatin1String("where") << perforceName;
    proc.start(m_settings.p4Command(), args);
    if (!proc.waitForFinished()) {
        *errorMessage = tr("Timeout waiting for \"where\" (%1).").arg(perforceName);
        return QString();
    }

    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    if (output.endsWith(QLatin1Char('\r')))
        output.chop(1);
    if (output.endsWith(QLatin1Char('\n')))
        output.chop(1);

    if (output.isEmpty()) {
        *errorMessage = tr("Error running \"where\" on %1: The file is not mapped").arg(perforceName);
        return QString();
    }
    const QString rc = output.mid(output.lastIndexOf(QLatin1Char(' ')) + 1);
    if (Perforce::Constants::debug)
        qDebug() << "fileNameFromPerforceName" << perforceName << rc;
    return rc;
}

PerforcePlugin *PerforcePlugin::perforcePluginInstance()
{
    QTC_ASSERT(m_perforcePluginInstance, return 0);
    return m_perforcePluginInstance;
}

Q_EXPORT_PLUGIN(PerforcePlugin)

