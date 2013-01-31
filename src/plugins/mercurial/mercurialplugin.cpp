/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
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
#include <coreplugin/id.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <locator/commandlocator.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtPlugin>
#include <QAction>
#include <QMenu>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include <QDialog>
#include <QFileDialog>

using namespace Mercurial::Internal;
using namespace Mercurial;
using namespace VcsBase;
using namespace Utils;

static const VcsBaseEditorParameters editorParameters[] = {
{
    RegularCommandOutput, //type
    Constants::COMMANDLOG_ID, // id
    Constants::COMMANDLOG_DISPLAY_NAME, // display name
    Constants::COMMANDLOG, // context
    Constants::COMMANDAPP, // mime type
    Constants::COMMANDEXT}, //extension

{   LogOutput,
    Constants::FILELOG_ID,
    Constants::FILELOG_DISPLAY_NAME,
    Constants::FILELOG,
    Constants::LOGAPP,
    Constants::LOGEXT},

{   AnnotateOutput,
    Constants::ANNOTATELOG_ID,
    Constants::ANNOTATELOG_DISPLAY_NAME,
    Constants::ANNOTATELOG,
    Constants::ANNOTATEAPP,
    Constants::ANNOTATEEXT},

{   DiffOutput,
    Constants::DIFFLOG_ID,
    Constants::DIFFLOG_DISPLAY_NAME,
    Constants::DIFFLOG,
    Constants::DIFFAPP,
    Constants::DIFFEXT}
};

static const VcsBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    Constants::COMMIT_ID,
    VcsBase::VcsBaseSubmitEditorParameters::DiffFiles
};

MercurialPlugin *MercurialPlugin::m_instance = 0;

MercurialPlugin::MercurialPlugin() :
        VcsBasePlugin(QLatin1String(Constants::COMMIT_ID)),
        optionsPage(0),
        m_client(0),
        core(0),
        m_commandLocator(0),
        m_addAction(0),
        m_deleteAction(0),
        m_createRepositoryAction(0),
        m_menuAction(0),
        m_submitActionTriggered(false)
{
    m_instance = this;
}

MercurialPlugin::~MercurialPlugin()
{
    if (m_client) {
        delete m_client;
        m_client = 0;
    }

    m_instance = 0;
}

bool MercurialPlugin::initialize(const QStringList & /* arguments */, QString * /*errorMessage */)
{
    typedef VcsEditorFactory<MercurialEditor> MercurialEditorFactory;

    m_client = new MercurialClient(&mercurialSettings);
    initializeVcs(new MercurialControl(m_client));

    optionsPage = new OptionsPage();
    addAutoReleasedObject(optionsPage);
    mercurialSettings.readSettings(core->settings());

    connect(m_client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(editorParameters[0]);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new MercurialEditorFactory(editorParameters + i, m_client, describeSlot));

    addAutoReleasedObject(new VcsSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    const QString prefix = QLatin1String("hg");
    m_commandLocator = new Locator::CommandLocator("Mercurial", prefix, prefix);
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
        static_cast<MercurialControl *>(versionControl())->emitConfigurationChanged();
    }
}

void MercurialPlugin::createMenu()
{
    Core::Context context(Core::Constants::C_GLOBAL);

    // Create menu item for Mercurial
    mercurialContainer = Core::ActionManager::createMenu(Core::Id("Mercurial.MercurialMenu"));
    QMenu *menu = mercurialContainer->menu();
    menu->setTitle(tr("Mercurial"));

    createFileActions(context);
    mercurialContainer->addSeparator(context);
    createDirectoryActions(context);
    mercurialContainer->addSeparator(context);
    createRepositoryActions(context);
    mercurialContainer->addSeparator(context);
    createRepositoryManagementActions(context);
    mercurialContainer->addSeparator(context);
    createLessUsedActions(context);

    // Request the Tools menu and add the Mercurial menu to it
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(mercurialContainer);
    m_menuAction = mercurialContainer->menu()->menuAction();
}

void MercurialPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    annotateFile = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(annotateFile, Core::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    diffFile = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(diffFile, Core::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+D") : tr("Alt+H,Alt+D")));
    connect(diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    logFile = new ParameterAction(tr("Log Current File"), tr("Log \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(logFile, Core::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+L") : tr("Alt+H,Alt+L")));
    connect(logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    statusFile = new ParameterAction(tr("Status Current File"), tr("Status \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(statusFile, Core::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+S") : tr("Alt+H,Alt+S")));
    connect(statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    mercurialContainer->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Core::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    revertFile = new ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(revertFile, Core::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void MercurialPlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void MercurialPlugin::logCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  QStringList(), true);
}

void MercurialPlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client->revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
}

void MercurialPlugin::statusCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPlugin::createDirectoryActions(const Core::Context &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertMulti()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void MercurialPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel());
}

void MercurialPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->log(state.topLevel());
}

void MercurialPlugin::revertMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog reverter;
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), reverter.revision());
}

void MercurialPlugin::statusMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_client->status(state.topLevel());
}

void MercurialPlugin::createRepositoryActions(const Core::Context &context)
{
    QAction *action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::IMPORT), context);
    connect(action, SIGNAL(triggered()), this, SLOT(import()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::INCOMING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(incoming()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::OUTGOING), context);
    connect(action, SIGNAL(triggered()), this, SLOT(outgoing()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+C") : tr("Alt+H,Alt+C")));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    mercurialContainer->addAction(command);
}

void MercurialPlugin::pull()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Pull Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->synchronousPull(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->synchronousPush(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::update()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog updateDialog;
    updateDialog.setWindowTitle(tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), updateDialog.revision());
}

void MercurialPlugin::import()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

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
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog;
    dialog.setWindowTitle(tr("Incoming Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->incoming(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPlugin::outgoing()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->outgoing(state.topLevel());
}

void MercurialPlugin::createSubmitEditorActions()
{
    Core::Context context(Constants::COMMIT_ID);
    Core::Command *command;

    editorCommit = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = Core::ActionManager::registerAction(editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    editorDiff = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = Core::ActionManager::registerAction(editorDiff, Core::Id(Constants::DIFFEDITOR), context);

    editorUndo = new QAction(tr("&Undo"), this);
    command = Core::ActionManager::registerAction(editorUndo, Core::Id(Core::Constants::UNDO), context);

    editorRedo = new QAction(tr("&Redo"), this);
    command = Core::ActionManager::registerAction(editorRedo, Core::Id(Core::Constants::REDO), context);
}

void MercurialPlugin::commit()
{
    if (VcsBaseSubmitEditor::raiseSubmitEditor())
        return;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(m_client, SIGNAL(parsedStatus(QList<VcsBase::VcsBaseClient::StatusItem>)),
            this, SLOT(showCommitWidget(QList<VcsBase::VcsBaseClient::StatusItem>)));
    m_client->emitParsedStatus(m_submitRepository);
}

void MercurialPlugin::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    VcsBaseOutputWindow *outputWindow = VcsBaseOutputWindow::instance();
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(m_client, SIGNAL(parsedStatus(QList<VcsBase::VcsBaseClient::StatusItem>)),
               this, SLOT(showCommitWidget(QList<VcsBase::VcsBaseClient::StatusItem>)));

    if (status.isEmpty()) {
        outputWindow->appendError(tr("There are no changes to commit."));
        return;
    }

    // Start new temp file
    Utils::TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    if (!saver.finalize()) {
        VcsBase::VcsBaseOutputWindow::instance()->append(saver.errorString());
        return;
    }

    Core::IEditor *editor = Core::EditorManager::openEditor(saver.fileName(),
                                                            Constants::COMMIT_ID,
                                                            Core::EditorManager::ModeSwitch);
    if (!editor) {
        outputWindow->appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    QTC_ASSERT(qobject_cast<CommitEditor *>(editor), return);
    CommitEditor *commitEditor = static_cast<CommitEditor *>(editor);

    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(QStringList)),
            this, SLOT(diffFromEditorSelected(QStringList)));
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
                        arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->setDisplayName(msg);

    QString branch = m_client->branchQuerySync(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            mercurialSettings.stringValue(MercurialSettings::userNameKey),
                            mercurialSettings.stringValue(MercurialSettings::userEmailKey), status);
}

void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

void MercurialPlugin::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    Core::ICore::editorManager()->closeEditor();
}

bool MercurialPlugin::submitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor)
{
    Core::IDocument *editorFile = submitEditor->document();
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor);
    if (!editorFile || !commitEditor)
        return true;

    bool dummyPrompt = mercurialSettings.boolValue(MercurialSettings::promptOnSubmitKey);
    const VcsBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;

    switch (response) {
    case VcsBaseSubmitEditor::SubmitCanceled:
        return false;
    case VcsBaseSubmitEditor::SubmitDiscarded:
        return true;
    default:
        break;
    }

    const QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!Core::DocumentManager::saveDocument(editorFile))
            return false;

        QStringList extraOptions;
        if (!commitEditor->committerInfo().isEmpty())
            extraOptions << QLatin1String("-u") << commitEditor->committerInfo();
        m_client->commit(m_submitRepository, files, editorFile->fileName(),
                         extraOptions);
    }
    return true;
}

void MercurialPlugin::createRepositoryManagementActions(const Core::Context &context)
{
    //TODO create menu for these options
    Q_UNUSED(context);
    return;
    //    QAction *action = new QAction(tr("Branch"), this);
    //    actionList.append(action);
    //    Core::Command *command = Core::ActionManager::registerAction(action, Constants::BRANCH, context);
    //    //    connect(action, SIGNAL(triggered()), this, SLOT(branch()));
    //    mercurialContainer->addAction(command);
}

void MercurialPlugin::createLessUsedActions(const Core::Context &context)
{
    //TODO create menue for these options
    Q_UNUSED(context);
    return;
}

void MercurialPlugin::updateActions(VcsBasePlugin::ActionState as)
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

#ifdef WITH_TESTS
#include <QTest>

void MercurialPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "diff --git a/src/plugins/mercurial/mercurialeditor.cpp b/src/plugins/mercurial/mercurialeditor.cpp\n"
            "new file mode 100644\n"
            "--- /dev/null\n"
            "+++ b/src/plugins/mercurial/mercurialeditor.cpp\n"
            "@@ -0,0 +1,112 @@\n\n")
        << QByteArray("src/plugins/mercurial/mercurialeditor.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "diff --git a/src/plugins/mercurial/mercurialeditor.cpp b/src/plugins/mercurial/mercurialeditor.cpp\n"
            "deleted file mode 100644\n"
            "--- a/src/plugins/mercurial/mercurialeditor.cpp\n"
            "+++ /dev/null\n"
            "@@ -1,112 +0,0 @@\n\n")
        << QByteArray("src/plugins/mercurial/mercurialeditor.cpp");
    QTest::newRow("Normal") << QByteArray(
            "diff --git a/src/plugins/mercurial/mercurialeditor.cpp b/src/plugins/mercurial/mercurialeditor.cpp\n"
            "--- a/src/plugins/mercurial/mercurialeditor.cpp\n"
            "+++ b/src/plugins/mercurial/mercurialeditor.cpp\n"
            "@@ -49,6 +49,8 @@\n\n")
        << QByteArray("src/plugins/mercurial/mercurialeditor.cpp");
}

void MercurialPlugin::testDiffFileResolving()
{
    MercurialEditor editor(editorParameters + 3, 0);
    editor.testDiffFileResolving();
}

void MercurialPlugin::testLogResolving()
{
    QByteArray data(
                "changeset:   18473:692cbda1eb50\n"
                "branch:      stable\n"
                "bookmark:    @\n"
                "tag:         tip\n"
                "user:        FUJIWARA Katsunori <foozy@lares.dti.ne.jp>\n"
                "date:        Wed Jan 23 22:52:55 2013 +0900\n"
                "summary:     revset: evaluate sub expressions correctly (issue3775)\n"
                "\n"
                "changeset:   18472:37100f30590f\n"
                "branch:      stable\n"
                "user:        Pierre-Yves David <pierre-yves.david@ens-lyon.org>\n"
                "date:        Sat Jan 19 04:08:16 2013 +0100\n"
                "summary:     test-rebase: add another test for rebase with multiple roots\n"
                );
    MercurialEditor editor(editorParameters + 1, 0);
    editor.testLogResolving(data, "18473:692cbda1eb50", "18472:37100f30590f");
}
#endif

Q_EXPORT_PLUGIN(MercurialPlugin)
