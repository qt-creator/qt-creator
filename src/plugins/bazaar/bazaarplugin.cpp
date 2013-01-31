/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
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
#include "bazaarplugin.h"
#include "constants.h"
#include "bazaarclient.h"
#include "bazaarcontrol.h"
#include "optionspage.h"
#include "bazaarcommitwidget.h"
#include "bazaareditor.h"
#include "pullorpushdialog.h"
#include "commiteditor.h"
#include "clonewizard.h"

#include "ui_revertdialog.h"

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
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtPlugin>
#include <QAction>
#include <QMenu>
#include <QMainWindow>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include <QDialog>
#include <QFileDialog>

using namespace Bazaar::Internal;
using namespace Bazaar;

static const VcsBase::VcsBaseEditorParameters editorParameters[] = {
    {
        VcsBase::RegularCommandOutput, //type
        Constants::COMMANDLOG_ID, // id
        Constants::COMMANDLOG_DISPLAY_NAME, // display name
        Constants::COMMANDLOG, // context
        Constants::COMMANDAPP, // mime type
        Constants::COMMANDEXT}, //extension

    {   VcsBase::LogOutput,
        Constants::FILELOG_ID,
        Constants::FILELOG_DISPLAY_NAME,
        Constants::FILELOG,
        Constants::LOGAPP,
        Constants::LOGEXT},

    {    VcsBase::AnnotateOutput,
         Constants::ANNOTATELOG_ID,
         Constants::ANNOTATELOG_DISPLAY_NAME,
         Constants::ANNOTATELOG,
         Constants::ANNOTATEAPP,
         Constants::ANNOTATEEXT},

    {   VcsBase::DiffOutput,
        Constants::DIFFLOG_ID,
        Constants::DIFFLOG_DISPLAY_NAME,
        Constants::DIFFLOG,
        Constants::DIFFAPP,
        Constants::DIFFEXT}
};

static const VcsBase::VcsBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    Constants::COMMIT_ID,
    VcsBase::VcsBaseSubmitEditorParameters::DiffFiles
};


BazaarPlugin *BazaarPlugin::m_instance = 0;

BazaarPlugin::BazaarPlugin()
    : VcsBase::VcsBasePlugin(QLatin1String(Constants::COMMIT_ID)),
      m_optionsPage(0),
      m_client(0),
      m_commandLocator(0),
      m_addAction(0),
      m_deleteAction(0),
      m_menuAction(0),
      m_submitActionTriggered(false)
{
    m_instance = this;
}

BazaarPlugin::~BazaarPlugin()
{
    if (m_client) {
        delete m_client;
        m_client = 0;
    }

    m_instance = 0;
}

bool BazaarPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    typedef VcsBase::VcsEditorFactory<BazaarEditor> BazaarEditorFactory;

    m_client = new BazaarClient(&m_bazaarSettings);
    initializeVcs(new BazaarControl(m_client));

    m_optionsPage = new OptionsPage();
    addAutoReleasedObject(m_optionsPage);
    m_bazaarSettings.readSettings(Core::ICore::settings());

    connect(m_client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(VcsBase::VcsBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new BazaarEditorFactory(editorParameters + i, m_client, describeSlot));

    addAutoReleasedObject(new VcsBase::VcsSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    const QString prefix = QLatin1String("bzr");
    m_commandLocator = new Locator::CommandLocator("Bazaar", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createMenu();

    createSubmitEditorActions();

    return true;
}

BazaarPlugin *BazaarPlugin::instance()
{
    return m_instance;
}

BazaarClient *BazaarPlugin::client() const
{
    return m_client;
}

const BazaarSettings &BazaarPlugin::settings() const
{
    return m_bazaarSettings;
}

void BazaarPlugin::setSettings(const BazaarSettings &settings)
{
    if (settings != m_bazaarSettings) {
        const bool userIdChanged = !m_bazaarSettings.sameUserId(settings);
        m_bazaarSettings = settings;
        if (userIdChanged)
            client()->synchronousSetUserId();
        static_cast<BazaarControl *>(versionControl())->emitConfigurationChanged();
    }
}

void BazaarPlugin::createMenu()
{
    Core::Context context(Core::Constants::C_GLOBAL);

    // Create menu item for Bazaar
    m_bazaarContainer = Core::ActionManager::createMenu(Core::Id("Bazaar.BazaarMenu"));
    QMenu *menu = m_bazaarContainer->menu();
    menu->setTitle(tr("Bazaar"));

    createFileActions(context);
    m_bazaarContainer->addSeparator(context);
    createDirectoryActions(context);
    m_bazaarContainer->addSeparator(context);
    createRepositoryActions(context);
    m_bazaarContainer->addSeparator(context);

    // Request the Tools menu and add the Bazaar menu to it
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(m_bazaarContainer);
    m_menuAction = m_bazaarContainer->menu()->menuAction();
}

void BazaarPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    m_annotateFile = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_annotateFile, Core::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_diffFile = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_diffFile, Core::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Z,Meta+D") : tr("ALT+Z,Alt+D")));
    connect(m_diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logFile = new Utils::ParameterAction(tr("Log Current File"), tr("Log \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_logFile, Core::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Z,Meta+L") : tr("ALT+Z,Alt+L")));
    connect(m_logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusFile = new Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_statusFile, Core::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Z,Meta+S") : tr("ALT+Z,Alt+S")));
    connect(m_statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_bazaarContainer->addSeparator(context);

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Core::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFile = new Utils::ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_revertFile, Core::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void BazaarPlugin::addCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::annotateCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::diffCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void BazaarPlugin::logCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  QStringList(), true);
}

void BazaarPlugin::revertCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertFile(state.currentFileTopLevel(),
                         state.relativeCurrentFile(),
                         revertUi.revisionLineEdit->text());
}

void BazaarPlugin::statusCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::createDirectoryActions(const Core::Context &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertAll()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}


void BazaarPlugin::diffRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel());
}

void BazaarPlugin::logRepository()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QStringList extraOptions;
    extraOptions += QLatin1String("--limit=") + QString::number(settings().intValue(BazaarSettings::logCountKey));
    m_client->log(state.topLevel(), QStringList(), extraOptions);
}

void BazaarPlugin::revertAll()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::statusMulti()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->status(state.topLevel());
}

void BazaarPlugin::createRepositoryActions(const Core::Context &context)
{
    QAction *action = 0;
    Core::Command *command = 0;

    action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Z,Meta+C") : tr("ALT+Z,Alt+C")));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    QAction *createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    m_bazaarContainer->addAction(command);
}

void BazaarPlugin::pull()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PullMode);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QStringList extraOptions;
    if (dialog.isRememberOptionEnabled())
        extraOptions += QLatin1String("--remember");
    if (dialog.isOverwriteOptionEnabled())
        extraOptions += QLatin1String("--overwrite");
    if (dialog.isLocalOptionEnabled())
        extraOptions += QLatin1String("--local");
    if (!dialog.revision().isEmpty())
        extraOptions << QLatin1String("-r") << dialog.revision();
    m_client->synchronousPull(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPlugin::push()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PushMode);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QStringList extraOptions;
    if (dialog.isRememberOptionEnabled())
        extraOptions += QLatin1String("--remember");
    if (dialog.isOverwriteOptionEnabled())
        extraOptions += QLatin1String("--overwrite");
    if (dialog.isUseExistingDirectoryOptionEnabled())
        extraOptions += QLatin1String("--use-existing-dir");
    if (dialog.isCreatePrefixOptionEnabled())
        extraOptions += QLatin1String("--create-prefix");
    if (!dialog.revision().isEmpty())
        extraOptions << QLatin1String("-r") << dialog.revision();
    m_client->synchronousPush(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPlugin::update()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    dialog.setWindowTitle(tr("Update"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::createSubmitEditorActions()
{
    Core::Context context(Constants::COMMIT_ID);
    Core::Command *command;

    m_editorCommit = new QAction(VcsBase::VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = Core::ActionManager::registerAction(m_editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    m_editorDiff = new QAction(VcsBase::VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = Core::ActionManager::registerAction(m_editorDiff, Core::Id(Constants::DIFFEDITOR), context);

    m_editorUndo = new QAction(tr("&Undo"), this);
    command = Core::ActionManager::registerAction(m_editorUndo, Core::Id(Core::Constants::UNDO), context);

    m_editorRedo = new QAction(tr("&Redo"), this);
    command = Core::ActionManager::registerAction(m_editorRedo, Core::Id(Core::Constants::REDO), context);
}

void BazaarPlugin::commit()
{
    if (VcsBase::VcsBaseSubmitEditor::raiseSubmitEditor())
        return;

    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(m_client, SIGNAL(parsedStatus(QList<VcsBase::VcsBaseClient::StatusItem>)),
            this, SLOT(showCommitWidget(QList<VcsBase::VcsBaseClient::StatusItem>)));
    // The "--short" option allows to easily parse status output
    m_client->emitParsedStatus(m_submitRepository, QStringList(QLatin1String("--short")));
}

void BazaarPlugin::showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status)
{
    VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
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

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        outputWindow->appendError(tr("Unable to create a commit editor."));
        return;
    }

    commitEditor->registerActions(m_editorUndo, m_editorRedo, m_editorCommit, m_editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(QStringList)),
            this, SLOT(diffFromEditorSelected(QStringList)));
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
            arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->setDisplayName(msg);

    const BranchInfo branch = m_client->synchronousBranchQuery(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            m_bazaarSettings.stringValue(BazaarSettings::userNameKey),
                            m_bazaarSettings.stringValue(BazaarSettings::userEmailKey), status);
}

void BazaarPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

#ifdef WITH_TESTS
#include <QTest>

void BazaarPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "=== added file 'src/plugins/bazaar/bazaareditor.cpp'\n"
            "--- src/plugins/bazaar/bazaareditor.cpp\t1970-01-01 00:00:00 +0000\n"
            "+++ src/plugins/bazaar/bazaareditor.cpp\t2013-01-20 21:39:47 +0000\n"
            "@@ -0,0 +1,121 @@\n\n")
        << QByteArray("src/plugins/bazaar/bazaareditor.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "=== removed file 'src/plugins/bazaar/bazaareditor.cpp'\n"
            "--- src/plugins/bazaar/bazaareditor.cpp\t2013-01-20 21:39:47 +0000\n"
            "+++ src/plugins/bazaar/bazaareditor.cpp\t1970-01-01 00:00:00 +0000\n"
            "@@ -1,121 +0,0 @@\n\n")
        << QByteArray("src/plugins/bazaar/bazaareditor.cpp");
    QTest::newRow("Modified") << QByteArray(
            "=== modified file 'src/plugins/bazaar/bazaareditor.cpp'\n"
            "--- src/plugins/bazaar/bazaareditor.cpp\t2010-08-27 14:12:44 +0000\n"
            "+++ src/plugins/bazaar/bazaareditor.cpp\t2011-02-28 21:24:19 +0000\n"
            "@@ -727,6 +727,9 @@\n\n")
        << QByteArray("src/plugins/bazaar/bazaareditor.cpp");
}

void BazaarPlugin::testDiffFileResolving()
{
    BazaarEditor editor(editorParameters + 3, 0);
    editor.testDiffFileResolving();
}

void BazaarPlugin::testLogResolving()
{
    QByteArray data(
                "------------------------------------------------------------\n"
                "revno: 6572 [merge]\n"
                "committer: Patch Queue Manager <pqm@pqm.ubuntu.com>\n"
                "branch nick: +trunk\n"
                "timestamp: Mon 2012-12-10 10:18:33 +0000\n"
                "message:\n"
                "  (vila) Fix LC_ALL=C test failures related to utf8 stderr encoding (Vincent\n"
                "   Ladeuil)\n"
                "------------------------------------------------------------\n"
                "revno: 6571 [merge]\n"
                "committer: Patch Queue Manager <pqm@pqm.ubuntu.com>\n"
                "branch nick: +trunk\n"
                "timestamp: Thu 2012-10-25 11:13:27 +0000\n"
                "message:\n"
                "  (gz) Set approved revision and vote \"Approve\" when using lp-propose\n"
                "   --approve (Jonathan Lange)\n"
                );
    BazaarEditor editor(editorParameters + 1, 0);
    editor.testLogResolving(data, "6572", "6571");
}
#endif

void BazaarPlugin::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    Core::ICore::editorManager()->closeEditor();
}

bool BazaarPlugin::submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor)
{
    Core::IDocument *editorDocument = submitEditor->document();
    const CommitEditor *commitEditor = qobject_cast<const CommitEditor *>(submitEditor);
    if (!editorDocument || !commitEditor)
        return true;

    bool dummyPrompt = m_bazaarSettings.boolValue(BazaarSettings::promptOnSubmitKey);
    const VcsBase::VcsBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, !m_submitActionTriggered);
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
            const QStringList parts = iFile->split(QLatin1String(" => "), QString::SkipEmptyParts);
            if (!parts.isEmpty())
                *iFile = parts.last();
        }

        const BazaarCommitWidget *commitWidget = commitEditor->commitWidget();
        QStringList extraOptions;
        // Author
        if (!commitWidget->committer().isEmpty())
            extraOptions.append(QLatin1String("--author=") + commitWidget->committer());
        // Fixed bugs
        foreach (const QString &fix, commitWidget->fixedBugs()) {
            if (!fix.isEmpty())
                extraOptions << QLatin1String("--fixes") << fix;
        }
        // Whether local commit or not
        if (commitWidget->isLocalOptionEnabled())
            extraOptions += QLatin1String("--local");
        m_client->commit(m_submitRepository, files, editorDocument->fileName(), extraOptions);
    }
    return true;
}

void BazaarPlugin::updateActions(VcsBase::VcsBasePlugin::ActionState as)
{
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

Q_EXPORT_PLUGIN(BazaarPlugin)
