/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialplugin.h"
#include "optionspage.h"
#include "constants.h"
#include "mercurialclient.h"
#include "mercurialcontrol.h"
#include "mercurialeditor.h"
#include "revertdialog.h"
#include "srcdestdialog.h"
#include "commiteditor.h"
#include "mercurialsettings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/id.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <coreplugin/locator/commandlocator.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QtPlugin>
#include <QAction>
#include <QMenu>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include <QDialog>
#include <QFileDialog>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace VcsBase;
using namespace Utils;

namespace Mercurial {
namespace Internal {

static const VcsBaseEditorParameters editorParameters[] = {
{
    LogOutput,
    Constants::FILELOG_ID,
    Constants::FILELOG_DISPLAY_NAME,
    Constants::LOGAPP},

{   AnnotateOutput,
    Constants::ANNOTATELOG_ID,
    Constants::ANNOTATELOG_DISPLAY_NAME,
    Constants::ANNOTATEAPP},

{   DiffOutput,
    Constants::DIFFLOG_ID,
    Constants::DIFFLOG_DISPLAY_NAME,
    Constants::DIFFAPP}
};

static const VcsBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

MercurialPlugin *MercurialPlugin::m_instance = 0;

MercurialPlugin::MercurialPlugin()
{
    m_instance = this;
}

MercurialPlugin::~MercurialPlugin()
{
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    m_instance = nullptr;
}

bool MercurialPlugin::initialize(const QStringList & /* arguments */, QString * /*errorMessage */)
{
    Core::Context context(Constants::MERCURIAL_CONTEXT);

    m_client = new MercurialClient;
    auto vc = initializeVcs<MercurialControl>(context, m_client);

    addAutoReleasedObject(new OptionsPage(vc));

    connect(m_client, &VcsBaseClient::changed, vc, &MercurialControl::changed);
    connect(m_client, &MercurialClient::needUpdate, this, &MercurialPlugin::update);

    const auto describeFunc = [this](const QString &source, const QString &id) {
        m_client->view(source, id);
    };
    const int editorCount = sizeof(editorParameters)/sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new MercurialEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, describeFunc));

    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitEditorParameters,
        []() { return new CommitEditor(&submitEditorParameters); }));

    const QString prefix = QLatin1String("hg");
    m_commandLocator = new Core::CommandLocator("Mercurial", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createMenu(context);

    createSubmitEditorActions();

    return true;
}

void MercurialPlugin::createMenu(const Core::Context &context)
{
    // Create menu item for Mercurial
    m_mercurialContainer = Core::ActionManager::createMenu("Mercurial.MercurialMenu");
    QMenu *menu = m_mercurialContainer->menu();
    menu->setTitle(tr("Me&rcurial"));

    createFileActions(context);
    m_mercurialContainer->addSeparator(context);
    createDirectoryActions(context);
    m_mercurialContainer->addSeparator(context);
    createRepositoryActions(context);
    m_mercurialContainer->addSeparator(context);

    // Request the Tools menu and add the Mercurial menu to it
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(m_mercurialContainer);
    m_menuAction = m_mercurialContainer->menu()->menuAction();
}

void MercurialPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    annotateFile = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(annotateFile, Core::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, &QAction::triggered, this, &MercurialPlugin::annotateCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    diffFile = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(diffFile, Core::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+D") : tr("Alt+G,Alt+D")));
    connect(diffFile, &QAction::triggered, this, &MercurialPlugin::diffCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    logFile = new ParameterAction(tr("Log Current File"), tr("Log \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(logFile, Core::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+L") : tr("Alt+G,Alt+L")));
    connect(logFile, &QAction::triggered, this, &MercurialPlugin::logCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    statusFile = new ParameterAction(tr("Status Current File"), tr("Status \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(statusFile, Core::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+S") : tr("Alt+G,Alt+S")));
    connect(statusFile, &QAction::triggered, this, &MercurialPlugin::statusCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_mercurialContainer->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Core::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &MercurialPlugin::addCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &MercurialPlugin::promptToDeleteCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    revertFile = new ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(revertFile, Core::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, &QAction::triggered, this, &MercurialPlugin::revertCurrentFile);
    m_mercurialContainer->addAction(command);
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
    int currentLine = -1;
    if (Core::IEditor *editor = Core::EditorManager::currentEditor())
        currentLine = editor->currentLine();
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), currentLine);
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

    RevertDialog reverter(Core::ICore::dialogParent());
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
    auto action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::diffRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::logRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::revertMulti);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::statusMulti);
    m_mercurialContainer->addAction(command);
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

    RevertDialog reverter(Core::ICore::dialogParent());
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
    auto action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::pull);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::push);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::update);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::IMPORT), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::import);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::INCOMING), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::incoming);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::OUTGOING), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::outgoing);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+C") : tr("Alt+G,Alt+C")));
    connect(action, &QAction::triggered, this, &MercurialPlugin::commit);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, &QAction::triggered, this, &MercurialPlugin::createRepository);
    m_mercurialContainer->addAction(command);
}

void MercurialPlugin::pull()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(SrcDestDialog::incoming, Core::ICore::dialogParent());
    dialog.setWindowTitle(tr("Pull Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->synchronousPull(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPlugin::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(SrcDestDialog::outgoing, Core::ICore::dialogParent());
    dialog.setWindowTitle(tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->synchronousPush(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPlugin::update()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog updateDialog(Core::ICore::dialogParent());
    updateDialog.setWindowTitle(tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), updateDialog.revision());
}

void MercurialPlugin::import()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QFileDialog importDialog(Core::ICore::dialogParent());
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

    SrcDestDialog dialog(SrcDestDialog::incoming, Core::ICore::dialogParent());
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

    editorCommit = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Core::Command *command = Core::ActionManager::registerAction(editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(editorCommit, &QAction::triggered, this, &MercurialPlugin::commitFromEditor);

    editorDiff = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    Core::ActionManager::registerAction(editorDiff, Core::Id(Constants::DIFFEDITOR), context);

    editorUndo = new QAction(tr("&Undo"), this);
    Core::ActionManager::registerAction(editorUndo, Core::Id(Core::Constants::UNDO), context);

    editorRedo = new QAction(tr("&Redo"), this);
    Core::ActionManager::registerAction(editorRedo, Core::Id(Core::Constants::REDO), context);
}

void MercurialPlugin::commit()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(m_client, &MercurialClient::parsedStatus, this, &MercurialPlugin::showCommitWidget);
    m_client->emitParsedStatus(m_submitRepository);
}

void MercurialPlugin::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(m_client, &MercurialClient::parsedStatus, this, &MercurialPlugin::showCommitWidget);

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

    Core::IEditor *editor = Core::EditorManager::openEditor(saver.fileName(),
                                                            Constants::COMMIT_ID);
    if (!editor) {
        VcsOutputWindow::appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    QTC_ASSERT(qobject_cast<CommitEditor *>(editor), return);
    CommitEditor *commitEditor = static_cast<CommitEditor *>(editor);
    setSubmitEditor(commitEditor);

    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
    connect(commitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &MercurialPlugin::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
                        arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->document()->setPreferredDisplayName(msg);

    QString branch = versionControl()->vcsTopic(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            m_client->settings().stringValue(MercurialSettings::userNameKey),
                            m_client->settings().stringValue(MercurialSettings::userEmailKey), status);
}

void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

void MercurialPlugin::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    Core::EditorManager::closeDocument(submitEditor()->document());
}

bool MercurialPlugin::submitEditorAboutToClose()
{
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    Core::IDocument *editorFile = commitEditor->document();
    QTC_ASSERT(editorFile, return true);

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

    const QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!Core::DocumentManager::saveDocument(editorFile))
            return false;

        QStringList extraOptions;
        if (!commitEditor->committerInfo().isEmpty())
            extraOptions << QLatin1String("-u") << commitEditor->committerInfo();
        m_client->commit(m_submitRepository, files, editorFile->filePath().toString(),
                         extraOptions);
    }
    return true;
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
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[2].id);
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
    VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "18473:692cbda1eb50", "18472:37100f30590f");
}
#endif

} // namespace Internal
} // namespace Mercurial
