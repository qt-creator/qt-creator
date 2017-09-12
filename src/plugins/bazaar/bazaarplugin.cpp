/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaarplugin.h"
#include "constants.h"
#include "bazaarclient.h"
#include "bazaarcontrol.h"
#include "optionspage.h"
#include "bazaarcommitwidget.h"
#include "bazaareditor.h"
#include "pullorpushdialog.h"
#include "uncommitdialog.h"
#include "commiteditor.h"

#include "ui_revertdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>

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
#include <QDebug>
#include <QDir>
#include <QDialog>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Bazaar {
namespace Internal {

// Submit editor parameters
const char COMMIT_ID[] = "Bazaar Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Commit Log Editor");
const char COMMITMIMETYPE[] = "text/vnd.qtcreator.bazaar.commit";

// Menu items
// File menu actions
const char ADD[] = "Bazaar.AddSingleFile";
const char DELETE[] = "Bazaar.DeleteSingleFile";
const char ANNOTATE[] = "Bazaar.Annotate";
const char DIFF[] = "Bazaar.DiffSingleFile";
const char LOG[] = "Bazaar.LogSingleFile";
const char REVERT[] = "Bazaar.RevertSingleFile";
const char STATUS[] = "Bazaar.Status";

// Directory menu Actions
const char DIFFMULTI[] = "Bazaar.Action.DiffMulti";
const char REVERTMULTI[] = "Bazaar.Action.RevertALL";
const char STATUSMULTI[] = "Bazaar.Action.StatusMulti";
const char LOGMULTI[] = "Bazaar.Action.Logmulti";

// Repository menu actions
const char PULL[] = "Bazaar.Action.Pull";
const char PUSH[] = "Bazaar.Action.Push";
const char UPDATE[] = "Bazaar.Action.Update";
const char COMMIT[] = "Bazaar.Action.Commit";
const char UNCOMMIT[] = "Bazaar.Action.UnCommit";
const char CREATE_REPOSITORY[] = "Bazaar.Action.CreateRepository";

// Submit editor actions
const char DIFFEDITOR[] = "Bazaar.Action.Editor.Diff";

const VcsBaseEditorParameters editorParameters[] = {
    {   LogOutput, // type
        Constants::FILELOG_ID, // id
        Constants::FILELOG_DISPLAY_NAME, // display name
        Constants::LOGAPP}, // mime type

    {    AnnotateOutput,
         Constants::ANNOTATELOG_ID,
         Constants::ANNOTATELOG_DISPLAY_NAME,
         Constants::ANNOTATEAPP},

    {   DiffOutput,
        Constants::DIFFLOG_ID,
        Constants::DIFFLOG_DISPLAY_NAME,
        Constants::DIFFAPP}
};

const VcsBaseSubmitEditorParameters submitEditorParameters = {
    COMMITMIMETYPE,
    COMMIT_ID,
    COMMIT_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};


BazaarPlugin *BazaarPlugin::m_instance = 0;

BazaarPlugin::BazaarPlugin()
{
    m_instance = this;
}

BazaarPlugin::~BazaarPlugin()
{
    delete m_client;
    m_client = 0;
    m_instance = 0;
}

bool BazaarPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    Context context(Constants::BAZAAR_CONTEXT);

    m_client = new BazaarClient;
    auto vcsCtrl = initializeVcs<BazaarControl>(context, m_client);
    connect(m_client, &VcsBaseClient::changed, vcsCtrl, &BazaarControl::changed);

    addAutoReleasedObject(new OptionsPage(vcsCtrl));

    const auto describeFunc = [this](const QString &source, const QString &id) {
        m_client->view(source, id);
    };
    const int editorCount = sizeof(editorParameters) / sizeof(VcsBaseEditorParameters);
    const auto widgetCreator = []() { return new BazaarEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc));

    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitEditorParameters,
        []() { return new CommitEditor(&submitEditorParameters); }));

    const QString prefix = QLatin1String("bzr");
    m_commandLocator = new CommandLocator("Bazaar", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createMenu(context);

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

void BazaarPlugin::createMenu(const Context &context)
{
    // Create menu item for Bazaar
    m_bazaarContainer = ActionManager::createMenu("Bazaar.BazaarMenu");
    QMenu *menu = m_bazaarContainer->menu();
    menu->setTitle(tr("Bazaar"));

    createFileActions(context);
    m_bazaarContainer->addSeparator(context);
    createDirectoryActions(context);
    m_bazaarContainer->addSeparator(context);
    createRepositoryActions(context);
    m_bazaarContainer->addSeparator(context);

    // Request the Tools menu and add the Bazaar menu to it
    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(m_bazaarContainer);
    m_menuAction = m_bazaarContainer->menu()->menuAction();
}

void BazaarPlugin::createFileActions(const Context &context)
{
    m_annotateFile = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    Command *command = ActionManager::registerAction(m_annotateFile, ANNOTATE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateFile, &QAction::triggered, this, &BazaarPlugin::annotateCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_diffFile = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFile, DIFF, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Z,Meta+D") : tr("ALT+Z,Alt+D")));
    connect(m_diffFile, &QAction::triggered, this, &BazaarPlugin::diffCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logFile = new ParameterAction(tr("Log Current File"), tr("Log \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logFile, LOG, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Z,Meta+L") : tr("ALT+Z,Alt+L")));
    connect(m_logFile, &QAction::triggered, this, &BazaarPlugin::logCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusFile = new ParameterAction(tr("Status Current File"), tr("Status \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusFile, STATUS, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Z,Meta+S") : tr("ALT+Z,Alt+S")));
    connect(m_statusFile, &QAction::triggered, this, &BazaarPlugin::statusCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_bazaarContainer->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, ADD, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &BazaarPlugin::addCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, DELETE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &BazaarPlugin::promptToDeleteCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFile = new ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFile, REVERT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertFile, &QAction::triggered, this, &BazaarPlugin::revertCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void BazaarPlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void BazaarPlugin::logCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  QStringList(), true);
}

void BazaarPlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QDialog dialog(ICore::dialogParent());
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
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::createDirectoryActions(const Context &context)
{
    auto action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    Command *command = ActionManager::registerAction(action, DIFFMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::diffRepository);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, LOGMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::logRepository);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, REVERTMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::revertAll);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, STATUSMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::statusMulti);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}


void BazaarPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel());
}

void BazaarPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QStringList extraOptions;
    extraOptions += QLatin1String("--limit=") + QString::number(m_client->settings().intValue(BazaarSettings::logCountKey));
    m_client->log(state.topLevel(), QStringList(), extraOptions);
}

void BazaarPlugin::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog(ICore::dialogParent());
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::statusMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->status(state.topLevel());
}

void BazaarPlugin::createRepositoryActions(const Context &context)
{
    auto action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Command *command = ActionManager::registerAction(action, PULL, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::pull);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, PUSH, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::push);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, UPDATE, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::update);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, COMMIT, context);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Z,Meta+C") : tr("ALT+Z,Alt+C")));
    connect(action, &QAction::triggered, this, &BazaarPlugin::commit);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Uncommit..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, UNCOMMIT, context);
    connect(action, &QAction::triggered, this, &BazaarPlugin::uncommit);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    auto createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = ActionManager::registerAction(createRepositoryAction, CREATE_REPOSITORY, context);
    connect(createRepositoryAction, &QAction::triggered, this, &BazaarPlugin::createRepository);
    m_bazaarContainer->addAction(command);
}

void BazaarPlugin::pull()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PullMode, ICore::dialogParent());
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
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PushMode, ICore::dialogParent());
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
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog(ICore::dialogParent());
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    dialog.setWindowTitle(tr("Update"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::createSubmitEditorActions()
{
    Context context(COMMIT_ID);
    Command *command;

    m_editorCommit = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ActionManager::registerAction(m_editorCommit, COMMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_editorCommit, &QAction::triggered, this, &BazaarPlugin::commitFromEditor);

    m_editorDiff = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_editorDiff, DIFFEDITOR, context);

    m_editorUndo = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_editorUndo, Core::Constants::UNDO, context);

    m_editorRedo = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_editorRedo, Core::Constants::REDO, context);
}

void BazaarPlugin::commit()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    QObject::connect(m_client, &VcsBaseClient::parsedStatus,
                     this, &BazaarPlugin::showCommitWidget);
    // The "--short" option allows to easily parse status output
    m_client->emitParsedStatus(m_submitRepository, QStringList(QLatin1String("--short")));
}

void BazaarPlugin::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    QObject::disconnect(m_client, &VcsBaseClient::parsedStatus,
                        this, &BazaarPlugin::showCommitWidget);

    if (status.isEmpty()) {
        VcsOutputWindow::appendError(tr("There are no changes to commit."));
        return;
    }

    // Start new temp file
    TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }

    IEditor *editor = EditorManager::openEditor(saver.fileName(), COMMIT_ID);
    if (!editor) {
        VcsOutputWindow::appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        VcsOutputWindow::appendError(tr("Unable to create a commit editor."));
        return;
    }
    setSubmitEditor(commitEditor);

    commitEditor->registerActions(m_editorUndo, m_editorRedo, m_editorCommit, m_editorDiff);
    connect(commitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &BazaarPlugin::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
            arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->document()->setPreferredDisplayName(msg);

    const BranchInfo branch = m_client->synchronousBranchQuery(m_submitRepository);
    const VcsBaseClientSettings &s = m_client->settings();
    commitEditor->setFields(m_submitRepository, branch,
                            s.stringValue(BazaarSettings::userNameKey),
                            s.stringValue(BazaarSettings::userEmailKey), status);
}

void BazaarPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

#ifdef WITH_TESTS

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
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[2].id);
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
    VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "6572", "6571");
}
#endif

void BazaarPlugin::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

void BazaarPlugin::uncommit()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    UnCommitDialog dialog(ICore::dialogParent());
    if (dialog.exec() == QDialog::Accepted)
        m_client->synchronousUncommit(state.topLevel(), dialog.revision(), dialog.extraOptions());
}

bool BazaarPlugin::submitEditorAboutToClose()
{
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    IDocument *editorDocument = commitEditor->document();
    QTC_ASSERT(editorDocument, return true);

    bool dummyPrompt = false;
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

    QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!DocumentManager::saveDocument(editorDocument))
            return false;

        //rewrite entries of the form 'file => newfile' to 'newfile' because
        //this would mess the commit command
        for (QStringList::iterator iFile = files.begin(); iFile != files.end(); ++iFile) {
            const QStringList parts = iFile->split(QLatin1String(" => "), QString::SkipEmptyParts);
            if (!parts.isEmpty())
                *iFile = parts.last();
        }

        BazaarCommitWidget *commitWidget = commitEditor->commitWidget();
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
        m_client->commit(m_submitRepository, files, editorDocument->filePath().toString(), extraOptions);
    }
    return true;
}

void BazaarPlugin::updateActions(VcsBasePlugin::ActionState as)
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

} // namespace Internal
} // namespace Bazaar
