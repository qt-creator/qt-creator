/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include "fossilplugin.h"
#include "constants.h"
#include "fossilclient.h"
#include "fossilcontrol.h"
#include "optionspage.h"
#include "fossilcommitwidget.h"
#include "fossileditor.h"
#include "pullorpushdialog.h"
#include "configuredialog.h"
#include "commiteditor.h"
#include "wizard/fossiljsextension.h"

#include "ui_revertdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/jsexpander.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QtPlugin>
#include <QAction>
#include <QMenu>
#include <QDir>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QRegularExpression>

namespace Fossil {
namespace Internal {

static const VcsBase::VcsBaseEditorParameters editorParameters[] = {
    {   VcsBase::LogOutput,
        Constants::FILELOG_ID,
        Constants::FILELOG_DISPLAY_NAME,
        Constants::LOGAPP},

    {    VcsBase::AnnotateOutput,
         Constants::ANNOTATELOG_ID,
         Constants::ANNOTATELOG_DISPLAY_NAME,
         Constants::ANNOTATEAPP},

    {   VcsBase::DiffOutput,
        Constants::DIFFLOG_ID,
        Constants::DIFFLOG_DISPLAY_NAME,
        Constants::DIFFAPP}
};

static const VcsBase::VcsBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    VcsBase::VcsBaseSubmitEditorParameters::DiffFiles
};


FossilPlugin *FossilPlugin::m_instance = nullptr;

FossilPlugin::FossilPlugin()
{
    m_instance = this;
}

FossilPlugin::~FossilPlugin()
{
    delete m_client;
    m_client = nullptr;
    m_instance = nullptr;
}

bool FossilPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    Core::Context context(Constants::FOSSIL_CONTEXT);

    m_client = new FossilClient;
    auto vcsCtrl = initializeVcs<FossilControl>(context, m_client);
    connect(m_client, &VcsBase::VcsBaseClient::changed, vcsCtrl, &FossilControl::changed);

    new OptionsPage(vcsCtrl, this);

    const auto describeFunc = [this](const QString &source, const QString &id) {
        m_client->view(source, id);
    };

    const int editorCount = sizeof(editorParameters) / sizeof(VcsBase::VcsBaseEditorParameters);
    const auto widgetCreator = []() { return new FossilEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        new VcsBase::VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc, this);

    new VcsBase::VcsSubmitEditorFactory(&submitEditorParameters,
        []() { return new CommitEditor(&submitEditorParameters); }, this);

    m_commandLocator = new Core::CommandLocator("Fossil", "fossil", "fossil", this);

    ProjectExplorer::JsonWizardFactory::addWizardPath(Utils::FileName::fromString(Constants::WIZARD_PATH));
    Core::JsExpander::registerQObjectForJs("Fossil", new FossilJsExtension);

    createMenu(context);

    Core::HelpManager::registerDocumentation({Core::ICore::documentationPath()
                                              + "/fossil.qch"});

    return true;
}

FossilPlugin *FossilPlugin::instance()
{
    return m_instance;
}

FossilClient *FossilPlugin::client() const
{
    return m_client;
}


void FossilPlugin::createMenu(const Core::Context &context)
{
    // Create menu item for Fossil
    m_fossilContainer = Core::ActionManager::createMenu("Fossil.FossilMenu");
    QMenu *menu = m_fossilContainer->menu();
    menu->setTitle(tr("&Fossil"));

    createFileActions(context);
    m_fossilContainer->addSeparator(context);
    createDirectoryActions(context);
    m_fossilContainer->addSeparator(context);
    createRepositoryActions(context);
    m_fossilContainer->addSeparator(context);

    // Request the Tools menu and add the Fossil menu to it
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(m_fossilContainer);
    m_menuAction = m_fossilContainer->menu()->menuAction();
}

void FossilPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    m_annotateFile = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_annotateFile, Constants::ANNOTATE, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateFile, &QAction::triggered, this, &FossilPlugin::annotateCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_diffFile = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_diffFile, Constants::DIFF, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+D") : tr("ALT+I,Alt+D")));
    connect(m_diffFile, &QAction::triggered, this, &FossilPlugin::diffCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logFile = new Utils::ParameterAction(tr("Timeline Current File"), tr("Timeline \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_logFile, Constants::LOG, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+L") : tr("ALT+I,Alt+L")));
    connect(m_logFile, &QAction::triggered, this, &FossilPlugin::logCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusFile = new Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_statusFile, Constants::STATUS, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+S") : tr("ALT+I,Alt+S")));
    connect(m_statusFile, &QAction::triggered, this, &FossilPlugin::statusCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_fossilContainer->addSeparator(context);

    m_addAction = new Utils::ParameterAction(tr("Add Current File"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Constants::ADD, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &FossilPlugin::addCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete Current File..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Constants::DELETE, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &FossilPlugin::deleteCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFile = new Utils::ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_revertFile, Constants::REVERT, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertFile, &QAction::triggered, this, &FossilPlugin::revertCurrentFile);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void FossilPlugin::addCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void FossilPlugin::deleteCurrentFile()
{
    promptToDeleteCurrentFile();
}

void FossilPlugin::annotateCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const int lineNumber = VcsBase::VcsBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), lineNumber);
}

void FossilPlugin::diffCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void FossilPlugin::logCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    FossilClient::SupportedFeatures features = m_client->supportedFeatures();
    QStringList extraOptions;
    extraOptions << "-n" << QString::number(m_client->settings().intValue(FossilSettings::logCountKey));

    if (features.testFlag(FossilClient::TimelineWidthFeature))
        extraOptions << "-W" << QString::number(m_client->settings().intValue(FossilSettings::timelineWidthKey));

    // disable annotate context menu for older client versions, used to be supported for current revision only
    bool enableAnnotationContextMenu = features.testFlag(FossilClient::AnnotateRevisionFeature);

    m_client->logCurrentFile(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  extraOptions, enableAnnotationContextMenu);
}

void FossilPlugin::revertCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QDialog dialog(Core::ICore::dialogParent());
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertFile(state.currentFileTopLevel(),
                         state.relativeCurrentFile(),
                         revertUi.revisionLineEdit->text());
}

void FossilPlugin::statusCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void FossilPlugin::createDirectoryActions(const Core::Context &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::DIFFMULTI, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::diffRepository);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Timeline"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::LOGMULTI, context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+T") : tr("ALT+I,Alt+T")));
    connect(action, &QAction::triggered, this, &FossilPlugin::logRepository);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::REVERTMULTI, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::revertAll);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::STATUSMULTI, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::statusMulti);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}


void FossilPlugin::diffRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel());
}

void FossilPlugin::logRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    FossilClient::SupportedFeatures features = m_client->supportedFeatures();
    QStringList extraOptions;
    extraOptions << "-n" << QString::number(m_client->settings().intValue(FossilSettings::logCountKey));

    if (features.testFlag(FossilClient::TimelineWidthFeature))
        extraOptions << "-W" << QString::number(m_client->settings().intValue(FossilSettings::timelineWidthKey));

    m_client->log(state.topLevel(), QStringList(), extraOptions);
}

void FossilPlugin::revertAll()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog(Core::ICore::dialogParent());
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), revertUi.revisionLineEdit->text());
}

void FossilPlugin::statusMulti()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->status(state.topLevel());
}

void FossilPlugin::createRepositoryActions(const Core::Context &context)
{
    QAction *action = 0;
    Core::Command *command = 0;

    action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::PULL, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::pull);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::PUSH, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::push);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::UPDATE, context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+U") : tr("ALT+I,Alt+U")));
    connect(action, &QAction::triggered, this, &FossilPlugin::update);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::COMMIT, context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I,Meta+C") : tr("ALT+I,Alt+C")));
    connect(action, &QAction::triggered, this, &FossilPlugin::commit);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Settings ..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Constants::CONFIGURE_REPOSITORY, context);
    connect(action, &QAction::triggered, this, &FossilPlugin::configureRepository);
    m_fossilContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    // Register "Create Repository..." action in global context, so that it's visible
    // without active repository to allow creating a new one.
    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Constants::CREATE_REPOSITORY);
    connect(m_createRepositoryAction, &QAction::triggered, this, &FossilPlugin::createRepository);
    m_fossilContainer->addAction(command);
}

void FossilPlugin::pull()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PullMode, Core::ICore::dialogParent());
    dialog.setLocalBaseDirectory(m_client->settings().stringValue(FossilSettings::defaultRepoPathKey));
    dialog.setDefaultRemoteLocation(m_client->synchronousGetRepositoryURL(state.topLevel()));
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString remoteLocation(dialog.remoteLocation());
    if (remoteLocation.isEmpty())
        remoteLocation = m_client->synchronousGetRepositoryURL(state.topLevel());

    if (remoteLocation.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError(tr("Remote repository is not defined."));
        return;
    }

    QStringList extraOptions;
    if (!dialog.isRememberOptionEnabled())
        extraOptions << "--once";
    if (dialog.isPrivateOptionEnabled())
        extraOptions << "--private";
    m_client->synchronousPull(state.topLevel(), remoteLocation, extraOptions);
}

void FossilPlugin::push()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PushMode, Core::ICore::dialogParent());
    dialog.setLocalBaseDirectory(m_client->settings().stringValue(FossilSettings::defaultRepoPathKey));
    dialog.setDefaultRemoteLocation(m_client->synchronousGetRepositoryURL(state.topLevel()));
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString remoteLocation(dialog.remoteLocation());
    if (remoteLocation.isEmpty())
        remoteLocation = m_client->synchronousGetRepositoryURL(state.topLevel());

    if (remoteLocation.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError(tr("Remote repository is not defined."));
        return;
    }

    QStringList extraOptions;
    if (!dialog.isRememberOptionEnabled())
        extraOptions << "--once";
    if (dialog.isPrivateOptionEnabled())
        extraOptions << "--private";
    m_client->synchronousPush(state.topLevel(), remoteLocation, extraOptions);
}

void FossilPlugin::update()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog(Core::ICore::dialogParent());
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    dialog.setWindowTitle(tr("Update"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), revertUi.revisionLineEdit->text());
}

void FossilPlugin::configureRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    ConfigureDialog dialog;

    // retrieve current settings from the repository
    RepositorySettings currentSettings = m_client->synchronousSettingsQuery(state.topLevel());

    dialog.setSettings(currentSettings);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const RepositorySettings newSettings = dialog.settings();

    m_client->synchronousConfigureRepository(state.topLevel(), newSettings, currentSettings);
}

void FossilPlugin::commit()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(m_client, &VcsBase::VcsBaseClient::parsedStatus,
            this, &FossilPlugin::showCommitWidget);

    QStringList extraOptions;
    m_client->emitParsedStatus(m_submitRepository, extraOptions);
}

void FossilPlugin::showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(m_client, &VcsBase::VcsBaseClient::parsedStatus,
               this, &FossilPlugin::showCommitWidget);

    if (status.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError(tr("There are no changes to commit."));
        return;
    }

    // Start new temp file for commit message
    Utils::TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    if (!saver.finalize()) {
        VcsBase::VcsOutputWindow::appendError(saver.errorString());
        return;
    }

    Core::IEditor *editor = Core::EditorManager::openEditor(saver.fileName(), Constants::COMMIT_ID);
    if (!editor) {
        VcsBase::VcsOutputWindow::appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        VcsBase::VcsOutputWindow::appendError(tr("Unable to create a commit editor."));
        return;
    }
    setSubmitEditor(commitEditor);

    const QString msg = tr("Commit changes for \"%1\".").
            arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->document()->setPreferredDisplayName(msg);

    const RevisionInfo currentRevision = m_client->synchronousRevisionQuery(m_submitRepository);
    const BranchInfo currentBranch = m_client->synchronousCurrentBranch(m_submitRepository);
    const QString currentUser = m_client->synchronousUserDefaultQuery(m_submitRepository);
    QStringList tags = m_client->synchronousTagQuery(m_submitRepository, currentRevision.id);
    // Fossil includes branch name in tag list -- remove.
    tags.removeAll(currentBranch.name());
    commitEditor->setFields(m_submitRepository, currentBranch, tags, currentUser, status);

    connect(commitEditor, &VcsBase::VcsBaseSubmitEditor::diffSelectedFiles,
            this, &FossilPlugin::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
}

void FossilPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

static inline bool ask(QWidget *parent, const QString &title, const QString &question, bool defaultValue = true)

{
    const QMessageBox::StandardButton defaultButton = defaultValue ? QMessageBox::Yes : QMessageBox::No;
    return QMessageBox::question(parent, title, question, QMessageBox::Yes|QMessageBox::No, defaultButton) == QMessageBox::Yes;
}

void FossilPlugin::createRepository()
{
    // re-implemented from void VcsBasePlugin::createRepository()

    // Find current starting directory
    QString directory;
    if (const ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectTree::currentProject())
        directory = currentProject->document()->filePath().toFileInfo().absolutePath();
    // Prompt for a directory that is not under version control yet
    QWidget *mw = Core::ICore::mainWindow();
    do {
        directory = QFileDialog::getExistingDirectory(mw, tr("Choose Checkout Directory"), directory);
        if (directory.isEmpty())
            return;
        const Core::IVersionControl *managingControl = Core::VcsManager::findVersionControlForDirectory(directory);
        if (managingControl == 0)
            break;
        const QString question = tr("The directory \"%1\" is already managed by a version control system (%2)."
                                    " Would you like to specify another directory?").arg(directory, managingControl->displayName());

        if (!ask(mw, tr("Repository already under version control"), question))
            return;
    } while (true);
    // Create
    const bool rc = static_cast<FossilControl *>(versionControl())->vcsCreateRepository(directory);
    const QString nativeDir = QDir::toNativeSeparators(directory);
    if (rc) {
        QMessageBox::information(mw, tr("Repository Created"),
                                 tr("A version control repository has been created in %1.").
                                 arg(nativeDir));
    } else {
        QMessageBox::warning(mw, tr("Repository Creation Failed"),
                                 tr("A version control repository could not be created in %1.").
                                 arg(nativeDir));
    }
}

void FossilPlugin::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    Core::EditorManager::closeDocument(submitEditor()->document());
}

bool FossilPlugin::submitEditorAboutToClose()
{
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    Core::IDocument *editorDocument = commitEditor->document();
    QTC_ASSERT(editorDocument, return true);

    bool promptOnSubmit = false;
    const VcsBase::VcsBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &promptOnSubmit, !m_submitActionTriggered);
    m_submitActionTriggered = false;

    switch (response) {
    case VcsBase::VcsBaseSubmitEditor::SubmitCanceled:
        return false;
    case VcsBase::VcsBaseSubmitEditor::SubmitDiscarded:
        return true;
    default:
        break;
    }

    QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!Core::DocumentManager::saveDocument(editorDocument))
            return false;

        //rewrite entries of the form 'file => newfile' to 'newfile' because
        //this would mess the commit command
        for (QStringList::iterator iFile = files.begin(); iFile != files.end(); ++iFile) {
            const QStringList parts = iFile->split(" => ", QString::SkipEmptyParts);
            if (!parts.isEmpty())
                *iFile = parts.last();
        }

        FossilCommitWidget *commitWidget = commitEditor->commitWidget();
        QStringList extraOptions;
        // Author -- override the repository-default user
        if (!commitWidget->committer().isEmpty())
            extraOptions << "--user" << commitWidget->committer();
        // Branch
        QString branch = commitWidget->newBranch();
        if (!branch.isEmpty()) {
            // @TODO: make enquote utility function
            QString enquotedBranch = branch;
            if (branch.contains(QRegularExpression("\\s")))
                enquotedBranch = QString("\"") + branch + "\"";
            extraOptions << "--branch" << enquotedBranch;
        }
        // Tags
        foreach (QString tag, commitWidget->tags()) {
            extraOptions << "--tag" << tag;
        }

        // Whether local commit or not
        if (commitWidget->isPrivateOptionEnabled())
            extraOptions += "--private";
        m_client->commit(m_submitRepository, files, editorDocument->filePath().toString(), extraOptions);
    }
    return true;
}


void FossilPlugin::updateActions(VcsBase::VcsBasePlugin::ActionState as)
{
    m_createRepositoryAction->setEnabled(true);

    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const QString filename = currentState().currentFileName();
    const bool repoEnabled = currentState().hasTopLevel();
    m_commandLocator->setEnabled(repoEnabled);

    m_annotateFile->setParameter(filename);
    m_diffFile->setParameter(filename);
    m_logFile->setParameter(filename);
    m_addAction->setParameter(filename);
    m_deleteAction->setParameter(filename);
    m_revertFile->setParameter(filename);
    m_statusFile->setParameter(filename);

    foreach (QAction *repoAction, m_repositoryActionList)
        repoAction->setEnabled(repoEnabled);
}

} // namespace Internal
} // namespace Fossil

#ifdef WITH_TESTS
#include <QTest>

void Fossil::Internal::FossilPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "ADDED   src/plugins/fossil/fossilclient.cpp\n"
            "Index: src/plugins/fossil/fossilclient.cpp\n"
            "==================================================================\n"
            "--- src/plugins/fossil/fossilclient.cpp\n"
            "+++ src/plugins/fossil/fossilclient.cpp\n"
            "@@ -0,0 +1,295 @@\n"
        )
        << QByteArray("src/plugins/fossil/fossilclient.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "DELETED src/plugins/fossil/fossilclient.cpp\n"
            "Index: src/plugins/fossil/fossilclient.cpp\n"
            "==================================================================\n"
            "--- src/plugins/fossil/fossilclient.cpp\n"
            "+++ src/plugins/fossil/fossilclient.cpp\n"
            "@@ -1,266 +0,0 @@\n"
        )
        << QByteArray("src/plugins/fossil/fossilclient.cpp");
    QTest::newRow("Modified") << QByteArray(
            "Index: src/plugins/fossil/fossilclient.cpp\n"
            "==================================================================\n"
            "--- src/plugins/fossil/fossilclient.cpp\n"
            "+++ src/plugins/fossil/fossilclient.cpp\n"
            "@@ -112,22 +112,37 @@\n"
        )
        << QByteArray("src/plugins/fossil/fossilclient.cpp");
}

void Fossil::Internal::FossilPlugin::testDiffFileResolving()
{
    VcsBase::VcsBaseEditorWidget::testDiffFileResolving(editorParameters[2].id);
}

void Fossil::Internal::FossilPlugin::testLogResolving()
{
    QByteArray data(
        "=== 2014-03-08 ===\n"
        "22:14:02 [ac6d1129b8] Change scaling algorithm. (user: ninja tags: ninja-fixes-5.1)\n"
        "   EDITED src/core/scaler.cpp\n"
        "20:23:51 [56d6917c3b] *BRANCH* Add width option (conditional). (user: ninja tags: ninja-fixes-5.1)\n"
        "   EDITED src/core/scaler.cpp\n"
        "   EDITED src/core/scaler.h\n"
    );
    VcsBase::VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "ac6d1129b8", "56d6917c3b");
}
#endif
