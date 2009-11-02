/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialplugin.h"
#include "optionspage.h"
#include "mercurialoutputwindow.h"
#include "constants.h"
#include "mercurialclient.h"
#include "mercurialcontrol.h"
#include "mercurialeditor.h"
#include "revertdialog.h"
#include "srcdestdialog.h"
#include "commiteditor.h"
#include "clonewizard.h"
#include "mercurialsettings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>


#include <projectexplorer/projectexplorer.h>
#include <utils/parameteraction.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>

#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMainWindow>
#include <QtCore/QtDebug>
#include <QtCore/QtGlobal>
#include <QtCore/QDir>
#include <QtGui/QDialog>
#include <QtGui/QFileDialog>
#include <QtCore/QTemporaryFile>


using namespace Mercurial::Internal;
using namespace Mercurial;

bool ListenForClose::editorAboutToClose(Core::IEditor *editor)
{
    return MercurialPlugin::instance()->closeEditor(editor);
}

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput, //type
    Constants::COMMANDLOG, // kind
    Constants::COMMANDLOG, // context
    Constants::COMMANDAPP, // mime type
    Constants::COMMANDEXT}, //extension

{   VCSBase::LogOutput,
    Constants::FILELOG,
    Constants::FILELOG,
    Constants::LOGAPP,
    Constants::LOGEXT},

{    VCSBase::AnnotateOutput,
     Constants::ANNOTATELOG,
     Constants::ANNOTATELOG,
     Constants::ANNOTATEAPP,
     Constants::ANNOTATEEXT},

{   VCSBase::DiffOutput,
    Constants::DIFFLOG,
    Constants::DIFFLOG,
    Constants::DIFFAPP,
    Constants::DIFFEXT}
};

static const VCSBase::VCSBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMITKIND,
    Constants::COMMITKIND
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters,
                                             sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

MercurialPlugin *MercurialPlugin::m_instance = 0;

MercurialPlugin::MercurialPlugin()
        :   mercurialSettings(new MercurialSettings),
        outputWindow(0),
        optionsPage(0),
        client(0),
        mercurialVC(0),
        projectExplorer(0),
        changeLog(0)
{
    m_instance = this;
}

MercurialPlugin::~MercurialPlugin()
{
    if (client) {
        delete client;
        client = 0;
    }

    if (mercurialSettings) {
        delete mercurialSettings;
        mercurialSettings = 0;
    }

    deleteCommitLog();

    m_instance = 0;
}

bool MercurialPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    typedef VCSBase::VCSEditorFactory<MercurialEditor> MercurialEditorFactory;

    core = Core::ICore::instance();
    actionManager = core->actionManager();

    optionsPage = new OptionsPage();
    addAutoReleasedObject(optionsPage);

    outputWindow = new MercurialOutputWindow();
    addAutoReleasedObject(outputWindow);

    client = new MercurialClient();
    connect(optionsPage, SIGNAL(settingsChanged()), client, SLOT(settingsChanged()));

    mercurialVC = new MercurialControl(client);
    addAutoReleasedObject(mercurialVC);

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new MercurialEditorFactory(editorParameters + i, client, describeSlot));

    addAutoReleasedObject(new VCSBase::VCSSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    addAutoReleasedObject(new ListenForClose);

    createMenu();

    createSubmitEditorActions();

    return true;
}

void MercurialPlugin::extensionsInitialized()
{
    projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (projectExplorer)
        connect(projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project *)),
                this, SLOT(currentProjectChanged(ProjectExplorer::Project *)));
}

MercurialOutputWindow *MercurialPlugin::outputPane()
{
    return outputWindow;
}

MercurialSettings *MercurialPlugin::settings()
{
    return mercurialSettings;
}

void MercurialPlugin::createMenu()
{
    QList<int> context = QList<int>()<< core->uniqueIDManager()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create menu item for Mercurial
    mercurialContainer = actionManager->createMenu("Mercurial.MercurialMenu");
    QMenu *menu = mercurialContainer->menu();
    menu->setTitle(tr("Mercurial"));

    if (QAction *visibleAction = menu->menuAction()) {
        visibleAction->setEnabled(mercurialVC->isEnabled());
        connect(mercurialVC, SIGNAL(enabledChanged(bool)), visibleAction, SLOT(setVisible(bool)));
    }

    createFileActions(context);
    createSeparator(context, "FileDirSeperator");
    createDirectoryActions(context);
    createSeparator(context, "DirRepoSeperator");
    createRepositoryActions(context);
    createSeparator(context, "Repository Management");
    createRepositoryManagementActions(context);
    createSeparator(context, "LessUsedfunctionality");
    createLessUsedActions(context);

    // Request the Tools menu and add the Mercurial menu to it
    Core::ActionContainer *toolsMenu = actionManager->actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(mercurialContainer);

    connect(core, SIGNAL(contextChanged(Core::IContext *)), this, SLOT(updateActions()));
    connect(core->fileManager(), SIGNAL(currentFileChanged(const QString &)),
            this, SLOT(updateActions()));

}

void MercurialPlugin::createFileActions(QList<int> &context)
{
    Core::Command *command;

    annotateFile = new Core::Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Core::Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(annotateFile, Constants::ANNOTATE, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mercurialContainer->addAction(command);

    diffFile = new Core::Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Core::Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(diffFile, Constants::DIFF, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr(Constants::MENUKEY) + tr(Constants::MODIFIER) + "D"));
    connect(diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mercurialContainer->addAction(command);

    logFile = new Core::Utils::ParameterAction(tr("Log Current File"), tr("Log \"%1\""), Core::Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(logFile, Constants::LOG, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr(Constants::MENUKEY) + tr(Constants::MODIFIER) + "L"));
    connect(logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    mercurialContainer->addAction(command);

    revertFile = new Core::Utils::ParameterAction(tr("Revert Current File"), tr("Revert \"%1\""), Core::Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(revertFile, Constants::REVERT, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mercurialContainer->addAction(command);

    statusFile = new Core::Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Core::Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(statusFile, Constants::STATUS, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr(Constants::MENUKEY) + tr(Constants::MODIFIER) + "S"));
    connect(statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::annotateCurrentFile()
{
    client->annotate(currentFile());
}

void MercurialPlugin::diffCurrentFile()
{
    client->diff(currentFile());
}

void MercurialPlugin::logCurrentFile()
{
    client->log(currentFile());
}

void MercurialPlugin::revertCurrentFile()
{
    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    const QString revision = reverter.m_ui->revisionLineEdit->text();
    client->revert(currentFile(), revision);
}

void MercurialPlugin::statusCurrentFile()
{
    client->status(currentFile());
}

void MercurialPlugin::createDirectoryActions(QList<int> &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::DIFFMULTI, context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Log"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::LOGMULTI, context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Revert"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::REVERTMULTI, context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertMulti()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Status"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::STATUSMULTI, context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::diffRepository()
{
    client->diff(currentProjectRoot());
}

void MercurialPlugin::logRepository()
{
    client->log(currentProjectRoot());
}

void MercurialPlugin::revertMulti()
{
    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    const QString revision = reverter.m_ui->revisionLineEdit->text();
    client->revert(currentProjectRoot(), revision);
}

void MercurialPlugin::statusMulti()
{
    client->status(currentProjectRoot());
}

void MercurialPlugin::createRepositoryActions(QList<int> &context)
{
    QAction *action = new QAction(tr("Pull"), this);
    actionList.append(action);
    Core::Command *command = actionManager->registerAction(action, Constants::PULL, context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Push"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::PUSH, context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Update"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::UPDATE, context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Import"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::IMPORT, context);
    connect(action, SIGNAL(triggered()), this, SLOT(import()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Incoming"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::INCOMING, context);
    connect(action, SIGNAL(triggered()), this, SLOT(incoming()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Outgoing"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::OUTGOING, context);
    connect(action, SIGNAL(triggered()), this, SLOT(outgoing()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Commit"), this);
    actionList.append(action);
    command = actionManager->registerAction(action, Constants::COMMIT, context);
    command->setDefaultKeySequence(QKeySequence(tr(Constants::MENUKEY) + tr(Constants::MODIFIER) + "C"));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::pull()
{
    SrcDestDialog dialog;
    dialog.setWindowTitle("Pull Source");
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString repository = dialog.getRepositoryString();
    client->pull(currentProjectRoot(), repository);
}

void MercurialPlugin::push()
{
    SrcDestDialog dialog;
    dialog.setWindowTitle("Push Destination");
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString repository = dialog.getRepositoryString();
    client->push(currentProjectRoot(), repository);
}

void MercurialPlugin::update()
{
    RevertDialog updateDialog;
    updateDialog.setWindowTitle("Update");
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    const QString revision = updateDialog.m_ui->revisionLineEdit->text();
    client->update(currentProjectRoot(), revision);
}

void MercurialPlugin::import()
{
    QFileDialog importDialog;
    importDialog.setFileMode(QFileDialog::ExistingFiles);
    importDialog.setViewMode(QFileDialog::Detail);

    if (importDialog.exec() != QDialog::Accepted)
        return;

    const QStringList fileNames = importDialog.selectedFiles();
    client->import(currentProjectRoot(), fileNames);
}

void MercurialPlugin::incoming()
{
    SrcDestDialog dialog;
    dialog.setWindowTitle("Incoming Source");
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString repository = dialog.getRepositoryString();
    client->incoming(currentProjectRoot(), repository);
}

void MercurialPlugin::outgoing()
{
    client->outgoing(currentProjectRoot());
}

void MercurialPlugin::createSubmitEditorActions()
{
    QList<int> context = QList<int>()<< core->uniqueIDManager()->uniqueIdentifier(Constants::COMMITKIND);
    Core::Command *command;

    editorCommit = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = actionManager->registerAction(editorCommit, Constants::COMMIT, context);
    connect(editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    editorDiff = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(editorDiff, Constants::DIFFEDITOR, context);

    editorUndo = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(editorUndo, Core::Constants::UNDO, context);

    editorRedo = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(editorRedo, Core::Constants::REDO, context);
}

void MercurialPlugin::commit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    connect(client, SIGNAL(parsedStatus(const QList<QPair<QString,QString> > &)),
            this, SLOT(showCommitWidget(const QList<QPair<QString,QString> > &)));
    client->statusWithSignal(currentProjectRoot());
}

void MercurialPlugin::showCommitWidget(const QList<QPair<QString, QString> > &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(client, SIGNAL(parsedStatus(const QList<QPair<QString,QString> > &)),
               this, SLOT(showCommitWidget(const QList<QPair<QString,QString> > &)));

    if (status.isEmpty()) {
        outputWindow->append(tr("There are no changes to commit"));
        return;
    }

    deleteCommitLog();

    changeLog = new QTemporaryFile(this);
    if (!changeLog->open()) {
        outputWindow->append(tr("Unable to generate a Tempory File for the Commit Editor"));
        return;
    }

    Core::IEditor *editor = core->editorManager()->openEditor(changeLog->fileName(),
                                                              Constants::COMMITKIND);
    if (!editor) {
        outputWindow->append(tr("Unable to generate an Editor for the commit"));
        return;
    }

    core->editorManager()->ensureEditorManagerVisible();

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        outputWindow->append(tr("Unable to generate a Commit Editor"));
        return;
    }

    commitEditor->setDisplayName(tr("Commit changes for \"") + currentProjectName() + tr("\""));

    QString branch = client->branchQuerySync(currentProjectRoot());

    commitEditor->setFields(currentProjectRoot(), branch, mercurialSettings->userName(),
                            mercurialSettings->email(), status);

    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(const QStringList &)),
            this, SLOT(diffFromEditorSelected(const QStringList &)));
}

void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
{
    foreach (QString file, files) {
        QFileInfo toDiff(QDir(currentProjectRoot().absoluteFilePath()).absoluteFilePath(file));
        client->diff(toDiff);
    }
}

void MercurialPlugin::commitFromEditor()
{
    if (!changeLog)
        return;

    //use the same functionality than if the user closes the file without completing the commit
    core->editorManager()->closeEditors(core->editorManager()->editorsForFileName(changeLog->fileName()));
}

bool MercurialPlugin::closeEditor(Core::IEditor *editor)
{
    if (!changeLog || !editor || qstrcmp(editor->kind(), Constants::COMMITKIND))
        return true;
    Core::IFile *editorFile = editor->file();
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);
    if (!editorFile || !commitEditor)
        return true;

    bool dummyPrompt = settings()->prompt();
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close commit editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, settings()->prompt());

    switch (response) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false;
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        deleteCommitLog();
        return true;
    default:
        break;
    }

    const QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        core->fileManager()->blockFileChange(editorFile);
        editorFile->save();
        core->fileManager()->unblockFileChange(editorFile);

        const QFileInfo repoRoot(commitEditor->repoRoot());
        client->commit(repoRoot, files, commitEditor->committerInfo(),
                       editorFile->fileName());
    }
    return true;
}
void MercurialPlugin::deleteCommitLog()
{
    if (changeLog) {
        delete changeLog;
        changeLog = 0;
    }
}

void MercurialPlugin::createRepositoryManagementActions(QList<int> &context)
{
    //TODO create menu for these options
    Q_UNUSED(context);
    return;
    //    QAction *action = new QAction(tr("Branch"), this);
    //    actionList.append(action);
    //    Core::Command *command = actionManager->registerAction(action, Constants::BRANCH, context);
    //    //    connect(action, SIGNAL(triggered()), this, SLOT(branch()));
    //    mercurialContainer->addAction(command);
}

void MercurialPlugin::createLessUsedActions(QList<int> &context)
{
    //TODO create menue for these options
    Q_UNUSED(context);
    return;
}

void MercurialPlugin::createSeparator(const QList<int> &context, const QString &id)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    mercurialContainer->addAction(actionManager->registerAction(action, id, context));
}

void MercurialPlugin::updateActions()
{
    const QFileInfo  file = currentFile();
    const QString filename = file.fileName();
    const QString repoRoot = client->findTopLevelForFile(file);
    bool enable = false;

    //File menu Items should only be enabled for files that are below a mercurial repository
    enable = !repoRoot.isEmpty();
    annotateFile->setParameter(filename);
    annotateFile->setEnabled(enable);
    diffFile->setParameter(filename);
    diffFile->setEnabled(enable);
    logFile->setParameter(filename);
    logFile->setEnabled(enable);
    revertFile->setParameter(filename);
    revertFile->setEnabled(enable);
    statusFile->setParameter(filename);
    statusFile->setEnabled(enable);

    //repository actions
    if (projectMapper.contains(currentProjectName()))
        enable = true;
    else
        enable = false;

    foreach (QAction *action, actionList)
        action->setEnabled(enable);
}

QFileInfo MercurialPlugin::currentFile()
{
    QString fileName = core->fileManager()->currentFile();
    QFileInfo fileInfo(fileName);
    return fileInfo;
}

QString MercurialPlugin::currentProjectName()
{
    if (projectExplorer)
        if (projectExplorer->currentProject())
            return projectExplorer->currentProject()->name();
    return QString();
}

void MercurialPlugin::currentProjectChanged(ProjectExplorer::Project *project)
{
    if (!project)
        return;

    if (projectMapper.contains(project->name()))
        return;

    QString repoRoot = client->findTopLevelForFile(QFileInfo(project->file()->fileName()));

    if (!repoRoot.isEmpty())
        projectMapper.insert(project->name(), QFileInfo(repoRoot));
}

QFileInfo MercurialPlugin::currentProjectRoot()
{
    return projectMapper.value(currentProjectName());
}

Q_EXPORT_PLUGIN(MercurialPlugin)
