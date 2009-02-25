/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "perforceplugin.h"

#include "changenumberdialog.h"
#include "p4.h"
#include "pendingchangesdialog.h"
#include "perforceconstants.h"
#include "perforceeditor.h"
#include "perforceoutputwindow.h"
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
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>

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

const char * const PerforcePlugin::PERFORCE_MENU = "Perforce.Menu";
const char * const PerforcePlugin::EDIT = "Perforce.Edit";
const char * const PerforcePlugin::ADD = "Perforce.Add";
const char * const PerforcePlugin::DELETE_FILE = "Perforce.Delete";
const char * const PerforcePlugin::OPENED = "Perforce.Opened";
const char * const PerforcePlugin::REVERT = "Perforce.Revert";
const char * const PerforcePlugin::DIFF_CURRENT = "Perforce.DiffCurrent";
const char * const PerforcePlugin::DIFF_PROJECT = "Perforce.DiffProject";
const char * const PerforcePlugin::DIFF_ALL = "Perforce.DiffAll";
const char * const PerforcePlugin::RESOLVE = "Perforce.Resolve";
const char * const PerforcePlugin::SUBMIT = "Perforce.Submit";
const char * const PerforcePlugin::PENDING_CHANGES = "Perforce.PendingChanges";
const char * const PerforcePlugin::DESCRIBE = "Perforce.Describe";
const char * const PerforcePlugin::ANNOTATE_CURRENT = "Perforce.AnnotateCurrent";
const char * const PerforcePlugin::ANNOTATE = "Perforce.Annotate";
const char * const PerforcePlugin::FILELOG_CURRENT = "Perforce.FilelogCurrent";
const char * const PerforcePlugin::FILELOG = "Perforce.Filelog";
const char * const PerforcePlugin::SEPARATOR1 = "Perforce.Separator1";
const char * const PerforcePlugin::SEPARATOR2 = "Perforce.Separator2";
const char * const PerforcePlugin::SEPARATOR3 = "Perforce.Separator3";

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
    m_perforceOutputWindow(0),
    m_settingsPage(0),
    m_editAction(0),
    m_addAction(0),
    m_deleteAction(0),
    m_openedAction(0),
    m_revertAction(0),
    m_diffCurrentAction(0),
    m_diffProjectAction(0),
    m_diffAllAction(0),
    m_resolveAction(0),
    m_submitAction(0),
    m_pendingAction(0),
    m_describeAction(0),
    m_annotateCurrentAction(0),
    m_annotateAction(0),
    m_filelogCurrentAction(0),
    m_filelogAction(0),
    m_changeTmpFile(0),
#ifdef USE_P4_API
    m_workbenchClientUser(0),
#endif
    m_coreListener(0),
    m_submitEditorFactory(0),
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
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    typedef VCSBase::VCSEditorFactory<PerforceEditor> PerforceEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<PerforceSubmitEditor> PerforceSubmitEditorFactory;

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.perforce/Perforce.mimetypes.xml"), errorMessage))
        return false;
    m_perforcePluginInstance = this;

    if (QSettings *settings = core->settings())
        m_settings.fromSettings(settings);

    m_perforceOutputWindow = new PerforceOutputWindow(this);
    addObject(m_perforceOutputWindow);

    m_settingsPage = new SettingsPage;
    addObject(m_settingsPage);

    // Editor factories
    m_submitEditorFactory = new PerforceSubmitEditorFactory(&submitParameters);
    addObject(m_submitEditorFactory);

    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++) {
        m_editorFactories.push_back(new PerforceEditorFactory(editorParameters + i, this, describeSlot));
        addObject(m_editorFactories.back());
    }

    m_versionControl = new PerforceVersionControl(this);
    addObject(m_versionControl);

    m_coreListener = new CoreListener(this);
    addObject(m_coreListener);

    //register actions
    Core::ActionManager *am = Core::ICore::instance()->actionManager();

    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mperforce =
        am->createMenu(QLatin1String(PERFORCE_MENU));
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

    m_editAction = new QAction(tr("Edit"), this);
    command = am->registerAction(m_editAction, PerforcePlugin::EDIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+E")));
    command->setDefaultText(tr("Edit File"));
    connect(m_editAction, SIGNAL(triggered()), this, SLOT(openCurrentFile()));
    mperforce->addAction(command);

    m_addAction = new QAction(tr("Add"), this);
    command = am->registerAction(m_addAction, PerforcePlugin::ADD, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+A")));
    command->setDefaultText(tr("Add File"));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mperforce->addAction(command);

    m_deleteAction = new QAction(tr("Delete"), this);
    command = am->registerAction(m_deleteAction, PerforcePlugin::DELETE_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Delete File"));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentFile()));
    mperforce->addAction(command);

    m_revertAction = new QAction(tr("Revert"), this);
    command = am->registerAction(m_revertAction, PerforcePlugin::REVERT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+R")));
    command->setDefaultText(tr("Revert File"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Edit"), globalcontext);
    mperforce->addAction(command);

    m_diffCurrentAction = new QAction(tr("Diff Current File"), this);
    command = am->registerAction(m_diffCurrentAction, PerforcePlugin::DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Diff Current File"));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mperforce->addAction(command);

    m_diffProjectAction = new QAction(tr("Diff Current Project/Session"), this);
    command = am->registerAction(m_diffProjectAction, PerforcePlugin::DIFF_PROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+D")));
    command->setDefaultText(tr("Diff Current Project/Session"));
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffCurrentProject()));
    mperforce->addAction(command);

    m_diffAllAction = new QAction(tr("Diff Opened Files"), this);
    command = am->registerAction(m_diffAllAction, PerforcePlugin::DIFF_ALL, globalcontext);
    connect(m_diffAllAction, SIGNAL(triggered()), this, SLOT(diffAllOpened()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Diff"), globalcontext);
    mperforce->addAction(command);

    m_openedAction = new QAction(tr("Opened"), this);
    command = am->registerAction(m_openedAction, PerforcePlugin::OPENED, globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+O")));
    connect(m_openedAction, SIGNAL(triggered()), this, SLOT(printOpenedFileList()));
    mperforce->addAction(command);

#ifdef USE_P4_API
    m_resolveAction = new QAction(tr("Resolve"), this);
    command = am->registerAction(m_resolveAction, PerforcePlugin::RESOLVE, globalcontext);
    connect(m_resolveAction, SIGNAL(triggered()), this, SLOT(resolve()));
    mperforce->addAction(command);
#endif

    m_submitAction = new QAction(tr("Submit Project"), this);
    command = am->registerAction(m_submitAction, PerforcePlugin::SUBMIT, globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+S")));
    connect(m_submitAction, SIGNAL(triggered()), this, SLOT(submit()));
    mperforce->addAction(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = am->registerAction(m_pendingAction, PerforcePlugin::PENDING_CHANGES, globalcontext);
    connect(m_pendingAction, SIGNAL(triggered()), this, SLOT(printPendingChanges()));
    mperforce->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = am->registerAction(tmpaction, QLatin1String("Perforce.Sep.Changes"), globalcontext);
    mperforce->addAction(command);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = am->registerAction(m_describeAction, PerforcePlugin::DESCRIBE, globalcontext);
    connect(m_describeAction, SIGNAL(triggered()), this, SLOT(describeChange()));
    mperforce->addAction(command);

    m_annotateCurrentAction = new QAction(tr("Annotate Current File"), this);
    command = am->registerAction(m_annotateCurrentAction, PerforcePlugin::ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultText(tr("Annotate Current File"));
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mperforce->addAction(command);

    m_annotateAction = new QAction(tr("Annotate..."), this);
    command = am->registerAction(m_annotateAction, PerforcePlugin::ANNOTATE, globalcontext);
    connect(m_annotateAction, SIGNAL(triggered()), this, SLOT(annotate()));
    mperforce->addAction(command);

    m_filelogCurrentAction = new QAction(tr("Filelog Current File"), this);
    command = am->registerAction(m_filelogCurrentAction, PerforcePlugin::FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+P,Alt+F")));
    command->setDefaultText(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this, SLOT(filelogCurrentFile()));
    mperforce->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = am->registerAction(m_filelogAction, PerforcePlugin::FILELOG, globalcontext);
    connect(m_filelogAction, SIGNAL(triggered()), this, SLOT(filelog()));
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
    m_projectExplorer = ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::ProjectExplorerPlugin>();
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

    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    QList<Core::IFile *> files = fm->managedFiles(fileName);
    foreach (Core::IFile *file, files) {
        fm->blockFileChange(file);
    }
    PerforceResponse result2 = runP4Cmd(QStringList() << QLatin1String("revert") << fileName, QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    Core::IFile::ReloadBehavior tempBehavior =
            Core::IFile::ReloadAll;
    foreach (Core::IFile *file, files) {
        file->modified(&tempBehavior);
        fm->unblockFileChange(file);
    }
}

void PerforcePlugin::diffCurrentFile()
{
    p4Diff(QStringList(currentFileName()));
}

void PerforcePlugin::diffCurrentProject()
{
    QString name;
    const QStringList nativeFiles = VCSBase::VCSBaseSubmitEditor::currentProjectFiles(true, &name);
    p4Diff(nativeFiles, name);
}

void PerforcePlugin::diffAllOpened()
{
    p4Diff(QStringList());
}

void PerforcePlugin::printOpenedFileList()
{
    Core::IEditor *e = Core::EditorManager::instance()->currentEditor();
    if (e)
        e->widget()->setFocus();
    PerforceResponse result = runP4Cmd(QStringList() << QLatin1String("opened"), QStringList(), CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
}

#ifdef USE_P4_API
void PerforcePlugin::resolve()
{
    m_workbenchClientUser->setMode(WorkbenchClientUser::Resolve);
    runP4APICmd(QLatin1String("resolve"));
}
#endif

void PerforcePlugin::submit()
{
    if (!checkP4Command()) {
        showOutput(tr("No p4 executable specified!"), true);
        return;
    }

    if (m_changeTmpFile) {
        showOutput(tr("Another submit is currently executed."), true);
        m_perforceOutputWindow->popup(false);
        return;
    }

    m_changeTmpFile = new QTemporaryFile(this);
    if (!m_changeTmpFile->open()) {
        showOutput(tr("Cannot create temporary file."), true);
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
        return;
    }

    PerforceResponse result = runP4Cmd(QStringList()<< QLatin1String("change") << QLatin1String("-o"), QStringList(),
                                       CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result.error) {
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
        return;
    }

    m_changeTmpFile->write(result.stdOut.toAscii());
    m_changeTmpFile->seek(0);

    // Assemble file list of project
    QString name;
    const QStringList nativeFiles = VCSBase::VCSBaseSubmitEditor::currentProjectFiles(true, &name);
    PerforceResponse result2 = runP4Cmd(QStringList(QLatin1String("fstat")), nativeFiles,
                                        CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result2.error) {
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
        return;
    }

    QStringList stdOutLines = result2.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    foreach (const QString &line, stdOutLines) {
        if (line.startsWith("... depotFile"))
            depotFileNames.append(line.mid(14));
    }
    if (depotFileNames.isEmpty()) {
        showOutput(tr("Project has no files"));
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
        return;
    }

    openPerforceSubmitEditor(m_changeTmpFile->fileName(), depotFileNames);
}

Core::IEditor *PerforcePlugin::openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName, Constants::PERFORCESUBMITEDITOR_KIND);
    editorManager->ensureEditorManagerVisible();
    PerforceSubmitEditor *submitEditor = dynamic_cast<PerforceSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
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
        const QFileInfo fi(fileName);
        showOutputInEditor(tr("p4 annotate %1").arg(fi.fileName()),
            result.stdOut, VCSBase::AnnotateOutput, codec);
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
    QString fileName = currentFileName();
    QString baseName = QFileInfo(fileName).fileName();
    const bool hasFile = !currentFileName().isEmpty();
    m_editAction->setEnabled(hasFile);
    m_addAction->setEnabled(hasFile);
    m_deleteAction->setEnabled(hasFile);
    m_revertAction->setEnabled(hasFile);
    m_diffCurrentAction->setEnabled(hasFile);
    m_annotateCurrentAction->setEnabled(hasFile);
    m_filelogCurrentAction->setEnabled(hasFile);
    if (hasFile) {
        m_editAction->setText(tr("Edit %1").arg(baseName));
        m_addAction->setText(tr("Add %1").arg(baseName));
        m_deleteAction->setText(tr("Delete %1").arg(baseName));
        m_revertAction->setText(tr("Revert %1").arg(baseName));
        m_diffCurrentAction->setText(tr("Diff %1").arg(baseName));
        m_annotateCurrentAction->setText(tr("Annotate %1").arg(baseName));
        m_filelogCurrentAction->setText(tr("Filelog %1").arg(baseName));
    } else {
        m_editAction->setText(tr("Edit"));
        m_addAction->setText(tr("Add"));
        m_deleteAction->setText(tr("Delete"));
        m_revertAction->setText(tr("Revert"));
        m_diffCurrentAction->setText(tr("Diff"));
        m_annotateCurrentAction->setText(tr("Annotate Current File"));
        m_filelogCurrentAction->setText(tr("Filelog Current File"));
    }
    if (m_projectExplorer && m_projectExplorer->currentProject()) {
        m_diffProjectAction->setEnabled(true);
        m_diffProjectAction->setText(tr("Diff Project %1").arg(m_projectExplorer->currentProject()->name()));
        m_submitAction->setEnabled(true);
    } else {
        m_diffProjectAction->setEnabled(false);
        m_diffProjectAction->setText(tr("Diff Current Project/Soluion"));
        m_submitAction->setEnabled(false);
    }
    m_diffAllAction->setEnabled(true);
    m_openedAction->setEnabled(true);
    m_describeAction->setEnabled(true);
    m_annotateAction->setEnabled(true);
    m_filelogAction->setEnabled(true);
    m_pendingAction->setEnabled(true);


#ifdef USE_P4_API
    m_resolveAction->setEnabled(m_enableP4APIActions);
#endif
}

bool PerforcePlugin::managesDirectory(const QString &directory) const
{
    if (!checkP4Command())
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

PerforceResponse PerforcePlugin::runP4Cmd(const QStringList &args,
                                          const QStringList &extraArgs,
                                          unsigned logFlags,
                                          QTextCodec *outputCodec) const
{
    if (Perforce::Constants::debug)
        qDebug() << "PerforcePlugin::runP4Cmd" << args << extraArgs << debugCodec(outputCodec);
    PerforceResponse response;
    response.error = true;
    if (!checkP4Command()) {
        response.message = tr("No p4 executable specified!");
        m_perforceOutputWindow->append(response.message, true);
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

    if (logFlags & CommandToWindow) {
        QString command = m_settings.p4Command();
        command += blank;
        command += actualArgs.join(QString(blank));
        const QString timeStamp = QTime::currentTime().toString(QLatin1String("HH:mm"));
        const QString outputText = tr("%1 Executing: %2\n").arg(timeStamp, command);
        showOutput(outputText, false);
    }

    // Run, connect stderr to the output window
    Core::Utils::SynchronousProcess process;
    process.setTimeout(p4Timeout);
    process.setStdOutCodec(outputCodec);
    process.setEnvironment(environment());

    // connect stderr to the output window if desired
    if (logFlags & StdErrToWindow) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdErrBuffered(QString,bool)), m_perforceOutputWindow, SLOT(append(QString,bool)));
    }

    // connect stdout to the output window if desired
    if (logFlags & StdOutToWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdOutBuffered(QString,bool)), m_perforceOutputWindow, SLOT(append(QString,bool)));
    }

    const Core::Utils::SynchronousProcessResponse sp_resp = process.run(m_settings.p4Command(), actualArgs);
    if (Perforce::Constants::debug)
        qDebug() << sp_resp;

    response.error = true;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Core::Utils::SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case Core::Utils::SynchronousProcessResponse::FinishedError:
        response.message = tr("The process terminated with exit code %1.").arg(sp_resp.exitCode);
        break;
    case Core::Utils::SynchronousProcessResponse::TerminatedAbnormally:
        response.message = tr("The process terminated abnormally.");
        break;
    case Core::Utils::SynchronousProcessResponse::StartFailed:
        response.message = tr("Could not start perforce '%1'. Please check your settings in the preferences.").arg(m_settings.p4Command());
        break;
    case Core::Utils::SynchronousProcessResponse::Hang:
        response.message = tr("Perforce did not respond within timeout limit (%1 ms).").arg(p4Timeout );
        break;
    }
    if (response.error) {
        if (Perforce::Constants::debug)
            qDebug() << response.message;
        if (logFlags & ErrorToWindow)
            m_perforceOutputWindow->append(response.message, true);
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
    Core::IEditor *editor = Core::EditorManager::instance()->
        newFile(kind, &s, output.toLocal8Bit());
    PerforceEditor *e = qobject_cast<PerforceEditor*>(editor->widget());
    if (!e)
        return 0;
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    if (codec)
        e->setCodec(codec);
    return e->editableInterface();
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
            if (ed->property("originalFileName").toString() == fileName) {
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
            editor->setProperty("originalFileName", files.at(0));
        } else if (!displayInEditor && existingEditor) {
            if (existingEditor) {
                existingEditor->createNew(result.stdOut);
                Core::EditorManager::instance()->setCurrentEditor(existingEditor);
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
    Core::EditorManager *em = Core::EditorManager::instance();
    em->closeEditors(QList<Core::IEditor*>() << em->currentEditor());
}

bool PerforcePlugin::editorAboutToClose(Core::IEditor *editor)
{
    if (!m_changeTmpFile || !editor)
        return true;
    Core::ICore *core = Core::ICore::instance();
    Core::IFile *fileIFace = editor->file();
    if (!fileIFace)
        return true;
    QFileInfo editorFile(fileIFace->fileName());
    QFileInfo changeFile(m_changeTmpFile->fileName());
    if (editorFile.absoluteFilePath() == changeFile.absoluteFilePath()) {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(core->mainWindow(),
                tr("Closing p4 Editor"),
                tr("Do you want to submit this change list?"),
                QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
        if (answer == QMessageBox::Cancel)
            return false;

        core->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        core->fileManager()->unblockFileChange(fileIFace);
        if (answer == QMessageBox::Yes) {
            QByteArray change = m_changeTmpFile->readAll();
            m_changeTmpFile->close();
            if (!checkP4Command()) {
                showOutput(tr("No p4 executable specified!"), true);
                delete m_changeTmpFile;
                m_changeTmpFile = 0;
                return false;
            }
            QProcess proc;
            proc.setEnvironment(environment());

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            proc.start(m_settings.p4Command(),
                m_settings.basicP4Args() << QLatin1String("submit") << QLatin1String("-i"));
            if (!proc.waitForStarted(p4Timeout)) {
                showOutput(tr("Cannot execute p4 submit."), true);
                QApplication::restoreOverrideCursor();
                delete m_changeTmpFile;
                m_changeTmpFile = 0;
                return false;
            }
            proc.write(change);
            proc.closeWriteChannel();

            if (!proc.waitForFinished()) {
                showOutput(tr("Cannot execute p4 submit."), true);
                QApplication::restoreOverrideCursor();
                delete m_changeTmpFile;
                m_changeTmpFile = 0;
                return false;
            }
            QString output = QString::fromUtf8(proc.readAll());
            showOutput(output);
            if (output.contains("Out of date files must be resolved or reverted"), true) {
                QMessageBox::warning(editor->widget(), "Pending change", "Could not submit the change, because your workspace was out of date. Created a pending submit instead.");
            }
            QApplication::restoreOverrideCursor();
        }
        m_changeTmpFile->close();
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
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
    if (!checkP4Command())
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

bool PerforcePlugin::checkP4Command() const
{
    return m_settings.isValid();
}

QString PerforcePlugin::pendingChangesData()
{
    QString data;
    if (!checkP4Command())
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

void PerforcePlugin::showOutput(const QString &output, bool popup) const
{
    m_perforceOutputWindow->append(output, popup);
}

PerforcePlugin::~PerforcePlugin()
{
    if (m_settingsPage) {
        removeObject(m_settingsPage);
        delete m_settingsPage;
        m_settingsPage = 0;
    }

    if (m_perforceOutputWindow) {
        removeObject(m_perforceOutputWindow);
        delete  m_perforceOutputWindow;
        m_perforceOutputWindow = 0;
    }

    if (m_submitEditorFactory) {
        removeObject(m_submitEditorFactory);
        delete m_submitEditorFactory;
        m_submitEditorFactory = 0;
    }

    if (m_versionControl) {
        removeObject(m_versionControl);
        delete m_versionControl;
        m_versionControl = 0;
    }

    if (!m_editorFactories.empty()) {
        foreach (Core::IEditorFactory *pf, m_editorFactories)
            removeObject(pf);
        qDeleteAll(m_editorFactories);
        m_editorFactories.clear();
    }

    if (m_coreListener) {
        removeObject(m_coreListener);
        delete m_coreListener;
        m_coreListener = 0;
    }
}

const PerforceSettings& PerforcePlugin::settings() const
{
    return m_settings;
}

void PerforcePlugin::setSettings(const QString &p4Command, const QString &p4Port, const QString &p4Client, const QString p4User, bool defaultEnv)
{

    if (m_settings.p4Command() == p4Command
        && m_settings.p4Port() == p4Port
        && m_settings.p4Client() == p4Client
        && m_settings.p4User() == p4User
        && m_settings.defaultEnv() == defaultEnv)
    {
        // Nothing to do
    } else {
        m_settings.setSettings(p4Command, p4Port, p4Client, p4User, defaultEnv);
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

