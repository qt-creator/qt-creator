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

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

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

MercurialPlugin::MercurialPlugin() :
        VCSBase::VCSBasePlugin(QLatin1String(Constants::COMMITKIND)),
        optionsPage(0),
        client(0),
        changeLog(0),
        m_menuAction(0)
{
    m_instance = this;
}

MercurialPlugin::~MercurialPlugin()
{
    if (client) {
        delete client;
        client = 0;
    }

    deleteCommitLog();

    m_instance = 0;
}

bool MercurialPlugin::initialize(const QStringList & /* arguments */, QString * /*error_message */)
{
    typedef VCSBase::VCSEditorFactory<MercurialEditor> MercurialEditorFactory;

    VCSBase::VCSBasePlugin::initialize(new MercurialControl(client));

    core = Core::ICore::instance();
    actionManager = core->actionManager();

    optionsPage = new OptionsPage();
    addAutoReleasedObject(optionsPage);
    mercurialSettings.readSettings(core->settings());

    client = new MercurialClient();
    connect(optionsPage, SIGNAL(settingsChanged()), client, SLOT(settingsChanged()));

    connect(client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new MercurialEditorFactory(editorParameters + i, client, describeSlot));

    addAutoReleasedObject(new VCSBase::VCSSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    createMenu();

    createSubmitEditorActions();

    return true;
}

void MercurialPlugin::extensionsInitialized()
{
}

const MercurialSettings &MercurialPlugin::settings() const
{
    return mercurialSettings;
}

void MercurialPlugin::setSettings(const MercurialSettings &settings)
{
    if (settings != mercurialSettings) {
        mercurialSettings = settings;
    }
}

QStringList MercurialPlugin::standardArguments() const
{
    return mercurialSettings.standardArguments();
}

void MercurialPlugin::createMenu()
{
    QList<int> context = QList<int>()<< core->uniqueIDManager()->uniqueIdentifier(QLatin1String(Core::Constants::C_GLOBAL));

    // Create menu item for Mercurial
    mercurialContainer = actionManager->createMenu(QLatin1String("Mercurial.MercurialMenu"));
    QMenu *menu = mercurialContainer->menu();
    menu->setTitle(tr("Mercurial"));

    createFileActions(context);
    createSeparator(context, QLatin1String("FileDirSeperator"));
    createDirectoryActions(context);
    createSeparator(context, QLatin1String("DirRepoSeperator"));
    createRepositoryActions(context);
    createSeparator(context, QLatin1String("Repository Management"));
    createRepositoryManagementActions(context);
    createSeparator(context, QLatin1String("LessUsedfunctionality"));
    createLessUsedActions(context);

    // Request the Tools menu and add the Mercurial menu to it
    Core::ActionContainer *toolsMenu = actionManager->actionContainer(QLatin1String(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(mercurialContainer);
    m_menuAction = mercurialContainer->menu()->menuAction();
}

void MercurialPlugin::createFileActions(const QList<int> &context)
{
    Core::Command *command;

    annotateFile = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(annotateFile, QLatin1String(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mercurialContainer->addAction(command);

    diffFile = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(diffFile, QLatin1String(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+D")));
    connect(diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mercurialContainer->addAction(command);

    logFile = new Utils::ParameterAction(tr("Log Current File"), tr("Log \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(logFile, QLatin1String(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+L")));
    connect(logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    mercurialContainer->addAction(command);

    revertFile = new Utils::ParameterAction(tr("Revert Current File"), tr("Revert \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(revertFile, QLatin1String(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mercurialContainer->addAction(command);

    statusFile = new Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = actionManager->registerAction(statusFile, QLatin1String(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+S")));
    connect(statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void MercurialPlugin::logCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void MercurialPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    client->revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
}

void MercurialPlugin::statusCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::createDirectoryActions(const QList<int> &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::DIFFMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::LOGMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::REVERTMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertMulti()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::STATUSMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    client->diff(state.topLevel());
}

void MercurialPlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    client->log(state.topLevel());
}

void MercurialPlugin::revertMulti()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    client->revertRepository(state.topLevel(), reverter.revision());
}

void MercurialPlugin::statusMulti()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    client->status(state.topLevel());
}

void MercurialPlugin::createRepositoryActions(const QList<int> &context)
{
    QAction *action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = actionManager->registerAction(action, QLatin1String(Constants::PULL), context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::PUSH), context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::UPDATE), context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::IMPORT), context);
    connect(action, SIGNAL(triggered()), this, SLOT(import()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::INCOMING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(incoming()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::OUTGOING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(outgoing()));
    mercurialContainer->addAction(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, QLatin1String(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+C")));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::pull()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Pull Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    client->pull(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::push()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    client->push(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::update()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    RevertDialog updateDialog;
    updateDialog.setWindowTitle(tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    client->update(state.topLevel(), updateDialog.revision());
}

void MercurialPlugin::import()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    QFileDialog importDialog;
    importDialog.setFileMode(QFileDialog::ExistingFiles);
    importDialog.setViewMode(QFileDialog::Detail);

    if (importDialog.exec() != QDialog::Accepted)
        return;

    const QStringList fileNames = importDialog.selectedFiles();
    client->import(state.topLevel(), fileNames);
}

void MercurialPlugin::incoming()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Incoming Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    client->incoming(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::outgoing()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    client->outgoing(state.topLevel());
}

void MercurialPlugin::createSubmitEditorActions()
{
    QList<int> context = QList<int>()<< core->uniqueIDManager()->uniqueIdentifier(QLatin1String(Constants::COMMITKIND));
    Core::Command *command;

    editorCommit = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = actionManager->registerAction(editorCommit, QLatin1String(Constants::COMMIT), context);
    connect(editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    editorDiff = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(editorDiff, QLatin1String(Constants::DIFFEDITOR), context);

    editorUndo = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(editorUndo, QLatin1String(Core::Constants::UNDO), context);

    editorRedo = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(editorRedo, QLatin1String(Core::Constants::REDO), context);
}

void MercurialPlugin::commit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    m_submitRepository = state.topLevel();

    connect(client, SIGNAL(parsedStatus(QList<QPair<QString,QString> >)),
            this, SLOT(showCommitWidget(QList<QPair<QString,QString> >)));
    client->statusWithSignal(m_submitRepository);
}

void MercurialPlugin::showCommitWidget(const QList<QPair<QString, QString> > &status)
{

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(client, SIGNAL(parsedStatus(QList<QPair<QString,QString> >)),
               this, SLOT(showCommitWidget(QList<QPair<QString,QString> >)));

    if (status.isEmpty()) {
        outputWindow->appendError(tr("There are no changes to commit."));
        return;
    }

    deleteCommitLog();

    changeLog = new QTemporaryFile(this);
    if (!changeLog->open()) {
        outputWindow->appendError(tr("Unable to generate a temporary file for the commit editor."));
        return;
    }

    Core::IEditor *editor = core->editorManager()->openEditor(changeLog->fileName(),
                                                              QLatin1String(Constants::COMMITKIND));
    if (!editor) {
        outputWindow->appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    core->editorManager()->ensureEditorManagerVisible();

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        outputWindow->appendError(tr("Unable to create a commit editor."));
        return;
    }

    const QString msg = tr("Commit changes for \"%1\".").arg(m_submitRepository);
    commitEditor->setDisplayName(msg);

    QString branch = client->branchQuerySync(m_submitRepository);

    commitEditor->setFields(m_submitRepository, branch, mercurialSettings.userName(),
                            mercurialSettings.email(), status);

    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(QStringList)),
            this, SLOT(diffFromEditorSelected(QStringList)));
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
}

void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
{
    client->diff(m_submitRepository, files);
}

void MercurialPlugin::commitFromEditor()
{
    if (!changeLog)
        return;

    //use the same functionality than if the user closes the file without completing the commit
    core->editorManager()->closeEditors(core->editorManager()->editorsForFileName(changeLog->fileName()));
}

bool MercurialPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!changeLog)
        return true;
    Core::IFile *editorFile = submitEditor->file();
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor);
    if (!editorFile || !commitEditor)
        return true;

    bool dummyPrompt = mercurialSettings.prompt();
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close commit editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, mercurialSettings.prompt());

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

        client->commit(commitEditor->repoRoot(), files, commitEditor->committerInfo(),
                       editorFile->fileName());
    }
    return true;
}
void MercurialPlugin::deleteCommitLog()
{
    if (changeLog) {
        delete changeLog;
        changeLog = 0;
        m_submitRepository.clear();
    }
}

void MercurialPlugin::createRepositoryManagementActions(const QList<int> &context)
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

void MercurialPlugin::createLessUsedActions(const QList<int> &context)
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

void MercurialPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!VCSBase::VCSBasePlugin::enableMenuAction(as, m_menuAction))
        return;

    const QString filename = currentState().currentFileName();
    const bool fileEnabled = !filename.isEmpty();
    const bool repoEnabled = currentState().hasTopLevel();

    annotateFile->setParameter(filename);
    annotateFile->setEnabled(fileEnabled);
    diffFile->setParameter(filename);
    diffFile->setEnabled(fileEnabled);
    logFile->setParameter(filename);
    logFile->setEnabled(fileEnabled);
    revertFile->setParameter(filename);
    revertFile->setEnabled(fileEnabled);
    statusFile->setParameter(filename);
    statusFile->setEnabled(fileEnabled);

    foreach (QAction *repoAction, m_repositoryActionList)
        repoAction->setEnabled(repoEnabled);
}

Q_EXPORT_PLUGIN(MercurialPlugin)
