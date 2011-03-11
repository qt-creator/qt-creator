/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <locator/commandlocator.h>

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
    Constants::COMMANDLOG_ID, // id
    Constants::COMMANDLOG_DISPLAY_NAME, // display name
    Constants::COMMANDLOG, // context
    Constants::COMMANDAPP, // mime type
    Constants::COMMANDEXT}, //extension

{   VCSBase::LogOutput,
    Constants::FILELOG_ID,
    Constants::FILELOG_DISPLAY_NAME,
    Constants::FILELOG,
    Constants::LOGAPP,
    Constants::LOGEXT},

{    VCSBase::AnnotateOutput,
     Constants::ANNOTATELOG_ID,
     Constants::ANNOTATELOG_DISPLAY_NAME,
     Constants::ANNOTATELOG,
     Constants::ANNOTATEAPP,
     Constants::ANNOTATEEXT},

{   VCSBase::DiffOutput,
    Constants::DIFFLOG_ID,
    Constants::DIFFLOG_DISPLAY_NAME,
    Constants::DIFFLOG,
    Constants::DIFFAPP,
    Constants::DIFFEXT}
};

static const VCSBase::VCSBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    Constants::COMMIT_ID
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditorWidget::findType(editorParameters,
                                             sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

MercurialPlugin *MercurialPlugin::m_instance = 0;

MercurialPlugin::MercurialPlugin() :
        VCSBase::VCSBasePlugin(QLatin1String(Constants::COMMIT_ID)),
        optionsPage(0),
        m_client(0),
        core(0),
        m_commandLocator(0),
        changeLog(0),
        m_addAction(0),
        m_deleteAction(0),
        m_createRepositoryAction(0),
        m_menuAction(0)
{
    m_instance = this;
}

MercurialPlugin::~MercurialPlugin()
{
    if (m_client) {
        delete m_client;
        m_client = 0;
    }

    deleteCommitLog();

    m_instance = 0;
}

bool MercurialPlugin::initialize(const QStringList & /* arguments */, QString * /*error_message */)
{
    typedef VCSBase::VCSEditorFactory<MercurialEditor> MercurialEditorFactory;

    m_client = new MercurialClient(mercurialSettings);
    VCSBase::VCSBasePlugin::initialize(new MercurialControl(m_client));

    core = Core::ICore::instance();
    actionManager = core->actionManager();

    optionsPage = new OptionsPage();
    addAutoReleasedObject(optionsPage);
    mercurialSettings.readSettings(core->settings(), QLatin1String("Mercurial"));

    connect(optionsPage, SIGNAL(settingsChanged()), m_client, SLOT(settingsChanged()));

    connect(m_client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new MercurialEditorFactory(editorParameters + i, m_client, describeSlot));

    addAutoReleasedObject(new VCSBase::VCSSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    const QString prefix = QLatin1String("hg");
    m_commandLocator = new Locator::CommandLocator(QLatin1String("Mercurial"), prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createMenu();

    createSubmitEditorActions();

    return true;
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
    Core::Context context(Core::Constants::C_GLOBAL);

    // Create menu item for Mercurial
    mercurialContainer = actionManager->createMenu(Core::Id("Mercurial.MercurialMenu"));
    QMenu *menu = mercurialContainer->menu();
    menu->setTitle(tr("Mercurial"));

    createFileActions(context);
    createSeparator(context, Core::Id("Mercurial.FileDirSeperator"));
    createDirectoryActions(context);
    createSeparator(context, Core::Id("Mercurial.DirRepoSeperator"));
    createRepositoryActions(context);
    createSeparator(context, Core::Id("Mercurial.Repository Management"));
    createRepositoryManagementActions(context);
    createSeparator(context, Core::Id("Mercurial.LessUsedfunctionality"));
    createLessUsedActions(context);

    // Request the Tools menu and add the Mercurial menu to it
    Core::ActionContainer *toolsMenu = actionManager->actionContainer(Core::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(mercurialContainer);
    m_menuAction = mercurialContainer->menu()->menuAction();
}

void MercurialPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    annotateFile = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(annotateFile, Core::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    diffFile = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(diffFile, Core::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+D")));
    connect(diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    logFile = new Utils::ParameterAction(tr("Log Current File"), tr("Log \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(logFile, Core::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+L")));
    connect(logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    statusFile = new Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(statusFile, Core::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+S")));
    connect(statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    createSeparator(context, Core::Id("Mercurial.FileDirSeperator1"));

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(m_addAction, Core::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    revertFile = new Utils::ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = actionManager->registerAction(revertFile, Core::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void MercurialPlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void MercurialPlugin::logCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void MercurialPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client->revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
}

void MercurialPlugin::statusCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::createDirectoryActions(const Core::Context &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertMulti()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}


void MercurialPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_client->diff(state.topLevel());
}

void MercurialPlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_client->log(state.topLevel());
}

void MercurialPlugin::revertMulti()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), reverter.revision());
}

void MercurialPlugin::statusMulti()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    m_client->status(state.topLevel());
}

void MercurialPlugin::createRepositoryActions(const Core::Context &context)
{
    QAction *action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = actionManager->registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::IMPORT), context);
    connect(action, SIGNAL(triggered()), this, SLOT(import()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::INCOMING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(incoming()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::OUTGOING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(outgoing()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = actionManager->registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+H,Alt+C")));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = actionManager->registerAction(m_createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
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
    m_client->synchronousPull(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::push()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->synchronousPush(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::update()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    RevertDialog updateDialog;
    updateDialog.setWindowTitle(tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), updateDialog.revision());
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
    m_client->import(state.topLevel(), fileNames);
}

void MercurialPlugin::incoming()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Incoming Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->incoming(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::outgoing()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_client->outgoing(state.topLevel());
}

void MercurialPlugin::createSubmitEditorActions()
{
    Core::Context context(Constants::COMMIT_ID);
    Core::Command *command;

    editorCommit = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = actionManager->registerAction(editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    editorDiff = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(editorDiff, Core::Id(Constants::DIFFEDITOR), context);

    editorUndo = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(editorUndo, Core::Id(Core::Constants::UNDO), context);

    editorRedo = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(editorRedo, Core::Id(Core::Constants::REDO), context);
}

void MercurialPlugin::commit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    m_submitRepository = state.topLevel();

    connect(m_client, SIGNAL(parsedStatus(QList<QPair<QString,QString> >)),
            this, SLOT(showCommitWidget(QList<QPair<QString,QString> >)));
    m_client->statusWithSignal(m_submitRepository);
}

void MercurialPlugin::showCommitWidget(const QList<QPair<QString, QString> > &status)
{

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(m_client, SIGNAL(parsedStatus(QList<QPair<QString,QString> >)),
               this, SLOT(showCommitWidget(QList<QPair<QString,QString> >)));

    if (status.isEmpty()) {
        outputWindow->appendError(tr("There are no changes to commit."));
        return;
    }

    deleteCommitLog();

    // Open commit log
    QString changeLogPattern = QDir::tempPath();
    if (!changeLogPattern.endsWith(QLatin1Char('/')))
        changeLogPattern += QLatin1Char('/');
    changeLogPattern += QLatin1String("qtcreator-hg-XXXXXX.msg");
    changeLog = new QTemporaryFile(changeLogPattern,  this);
    if (!changeLog->open()) {
        outputWindow->appendError(tr("Unable to generate a temporary file for the commit editor."));
        return;
    }

    Core::IEditor *editor = core->editorManager()->openEditor(changeLog->fileName(),
                                                              QLatin1String(Constants::COMMIT_ID),
                                                              Core::EditorManager::ModeSwitch);
    if (!editor) {
        outputWindow->appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    QTC_ASSERT(qobject_cast<CommitEditor *>(editor), return)
    CommitEditor *commitEditor = static_cast<CommitEditor *>(editor);

    const QString msg = tr("Commit changes for \"%1\".").
                        arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->setDisplayName(msg);

    QString branch = m_client->branchQuerySync(m_submitRepository);

    commitEditor->setFields(m_submitRepository, branch, mercurialSettings.userName(),
                            mercurialSettings.email(), status);

    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(QStringList)),
            this, SLOT(diffFromEditorSelected(QStringList)));
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
}

void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
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
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
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

        QHash<int, QVariant> extraOptions;
        extraOptions[MercurialClient::AuthorCommitOptionId] = commitEditor->committerInfo();
        m_client->commit(m_submitRepository, files, editorFile->fileName(),
                         extraOptions);
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

void MercurialPlugin::createRepositoryManagementActions(const Core::Context &context)
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

void MercurialPlugin::createLessUsedActions(const Core::Context &context)
{
    //TODO create menue for these options
    Q_UNUSED(context);
    return;
}

void MercurialPlugin::createSeparator(const Core::Context &context, const Core::Id &id)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    mercurialContainer->addAction(actionManager->registerAction(action, id, context));
}

void MercurialPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const QString filename = currentState().currentFileName();
    const bool repoEnabled = currentState().hasTopLevel();
    m_commandLocator->setEnabled(repoEnabled);

    annotateFile->setParameter(filename);
    diffFile->setParameter(filename);
    logFile->setParameter(filename);
    m_addAction->setParameter(filename);
    m_deleteAction->setParameter(filename);
    revertFile->setParameter(filename);
    statusFile->setParameter(filename);

    foreach (QAction *repoAction, m_repositoryActionList)
        repoAction->setEnabled(repoEnabled);
}

Q_EXPORT_PLUGIN(MercurialPlugin)
