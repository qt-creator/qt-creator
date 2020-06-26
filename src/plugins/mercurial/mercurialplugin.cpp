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
#include "mercurialeditor.h"
#include "revertdialog.h"
#include "srcdestdialog.h"
#include "commiteditor.h"
#include "mercurialsettings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
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
#include <vcsbase/vcscommand.h>

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
using namespace std::placeholders;

namespace Mercurial {
namespace Internal {

class MercurialTopicCache : public Core::IVersionControl::TopicCache
{
public:
    MercurialTopicCache(MercurialClient *client) : m_client(client) {}

protected:
    QString trackFile(const QString &repository) override
    {
        return repository + QLatin1String("/.hg/branch");
    }

    QString refreshTopic(const QString &repository) override
    {
        return m_client->branchQuerySync(repository);
    }

private:
    MercurialClient *m_client;
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    Constants::FILELOG_ID,
    Constants::FILELOG_DISPLAY_NAME,
    Constants::LOGAPP
};

const VcsBaseEditorParameters annotateEditorParameters {
    AnnotateOutput,
    Constants::ANNOTATELOG_ID,
    Constants::ANNOTATELOG_DISPLAY_NAME,
    Constants::ANNOTATEAPP
};

const VcsBaseEditorParameters diffEditorParameters {
    DiffOutput,
    Constants::DIFFLOG_ID,
    Constants::DIFFLOG_DISPLAY_NAME,
    Constants::DIFFAPP
};

const VcsBaseSubmitEditorParameters submitEditorParameters {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

class MercurialPluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_DECLARE_TR_FUNCTIONS(Mercurial::Internal::MercurialPlugin)

public:
    MercurialPluginPrivate();

    // IVersionControl
    QString displayName() const final;
    Utils::Id id() const final;
    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &filename, QString *topLevel = nullptr) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;
    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const QString &fileName) final;
    bool vcsAdd(const QString &filename) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;
    void vcsAnnotate(const QString &file, int line) final;
    void vcsDescribe(const QString &source, const QString &id) final { m_client.view(source, id); }

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FilePath &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) final;

    bool sccManaged(const QString &filename);

    // To be connected to the HgTask's success signal to emit the repository/
    // files changed signals according to the variant's type:
    // String -> repository, StringList -> files
    void changed(const QVariant&);

private:
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) final;
    bool submitEditorAboutToClose() final;

    // File menu action slots
    void addCurrentFile();
    void annotateCurrentFile();
    void diffCurrentFile();
    void logCurrentFile();
    void revertCurrentFile();
    void statusCurrentFile();

    // Directory menu action slots
    void diffRepository();
    void logRepository();
    void revertMulti();
    void statusMulti();

    // Repository menu action slots
    void pull();
    void push();
    void update();
    void import();
    void incoming();
    void outgoing();
    void commit();
    void showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status);
    void commitFromEditor() override;
    void diffFromEditorSelected(const QStringList &files);

    void createMenu(const Core::Context &context);
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    MercurialSettings m_settings;
    MercurialClient m_client{&m_settings};

    OptionsPage m_optionsPage{[this] { configurationChanged(); }, &m_settings};

    Core::CommandLocator *m_commandLocator = nullptr;
    Core::ActionContainer *m_mercurialContainer = nullptr;

    QList<QAction *> m_repositoryActionList;

    // Menu items (file actions)
    ParameterAction *m_addAction = nullptr;
    ParameterAction *m_deleteAction = nullptr;
    ParameterAction *annotateFile = nullptr;
    ParameterAction *diffFile = nullptr;
    ParameterAction *logFile = nullptr;
    ParameterAction *revertFile = nullptr;
    ParameterAction *statusFile = nullptr;

    QAction *m_createRepositoryAction = nullptr;
    QAction *m_menuAction = nullptr;

    QString m_submitRepository;

    bool m_submitActionTriggered = false;

public:
    VcsSubmitEditorFactory submitEditorFactory {
        submitEditorParameters,
        [] { return new CommitEditor; },
        this
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [this] { return new MercurialEditorWidget(&m_client); },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory annotateEditorFactory {
        &annotateEditorParameters,
        [this] { return new MercurialEditorWidget(&m_client); },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory diffEditorFactory {
        &diffEditorParameters,
        [this] { return new MercurialEditorWidget(&m_client); },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    };
};

static MercurialPluginPrivate *dd = nullptr;

MercurialPlugin::~MercurialPlugin()
{
    delete dd;
    dd = nullptr;
}

bool MercurialPlugin::initialize(const QStringList & /* arguments */, QString * /*errorMessage */)
{
    dd = new MercurialPluginPrivate;
    return true;
}

void MercurialPlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
}

MercurialPluginPrivate::MercurialPluginPrivate()
    : VcsBase::VcsBasePluginPrivate(Core::Context(Constants::MERCURIAL_CONTEXT))
{
    dd = this;

    setTopicCache(new MercurialTopicCache(&m_client));

    Core::Context context(Constants::MERCURIAL_CONTEXT);

    connect(&m_client, &VcsBaseClient::changed, this, &MercurialPluginPrivate::changed);
    connect(&m_client, &MercurialClient::needUpdate, this, &MercurialPluginPrivate::update);

    const QString prefix = QLatin1String("hg");
    m_commandLocator = new Core::CommandLocator("Mercurial", prefix, prefix, this);

    createMenu(context);
}

void MercurialPluginPrivate::createMenu(const Core::Context &context)
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
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Utils::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(m_mercurialContainer);
    m_menuAction = m_mercurialContainer->menu()->menuAction();
}

void MercurialPluginPrivate::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    annotateFile = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(annotateFile, Utils::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, &QAction::triggered, this, &MercurialPluginPrivate::annotateCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    diffFile = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(diffFile, Utils::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+H,Meta+D") : tr("Alt+G,Alt+D")));
    connect(diffFile, &QAction::triggered, this, &MercurialPluginPrivate::diffCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    logFile = new ParameterAction(tr("Log Current File"), tr("Log \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(logFile, Utils::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+H,Meta+L") : tr("Alt+G,Alt+L")));
    connect(logFile, &QAction::triggered, this, &MercurialPluginPrivate::logCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    statusFile = new ParameterAction(tr("Status Current File"), tr("Status \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(statusFile, Utils::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+H,Meta+S") : tr("Alt+G,Alt+S")));
    connect(statusFile, &QAction::triggered, this, &MercurialPluginPrivate::statusCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_mercurialContainer->addSeparator(context);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Utils::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &MercurialPluginPrivate::addCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Utils::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &MercurialPluginPrivate::promptToDeleteCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    revertFile = new ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(revertFile, Utils::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(revertFile, &QAction::triggered, this, &MercurialPluginPrivate::revertCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void MercurialPluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPluginPrivate::annotateCurrentFile()
{
    int currentLine = -1;
    if (Core::IEditor *editor = Core::EditorManager::currentEditor())
        currentLine = editor->currentLine();
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), currentLine);
}

void MercurialPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void MercurialPluginPrivate::logCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                 QStringList(), true);
}

void MercurialPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    RevertDialog reverter(Core::ICore::dialogParent());
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client.revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
}

void MercurialPluginPrivate::statusCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPluginPrivate::createDirectoryActions(const Core::Context &context)
{
    auto action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Utils::Id(Constants::DIFFMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::diffRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::LOGMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::logRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::REVERTMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::revertMulti);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::STATUSMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::statusMulti);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void MercurialPluginPrivate::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client.diff(state.topLevel());
}

void MercurialPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client.log(state.topLevel());
}

void MercurialPluginPrivate::revertMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog reverter(Core::ICore::dialogParent());
    if (reverter.exec() != QDialog::Accepted)
        return;
    m_client.revertAll(state.topLevel(), reverter.revision());
}

void MercurialPluginPrivate::statusMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_client.status(state.topLevel());
}

void MercurialPluginPrivate::createRepositoryActions(const Core::Context &context)
{
    auto action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Utils::Id(Constants::PULL), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::pull);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::PUSH), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::push);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::UPDATE), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::update);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::IMPORT), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::import);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::INCOMING), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::incoming);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::OUTGOING), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::outgoing);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+H,Meta+C") : tr("Alt+G,Alt+C")));
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::commit);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Utils::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, &QAction::triggered, this, &MercurialPluginPrivate::createRepository);
    m_mercurialContainer->addAction(command);
}

void MercurialPluginPrivate::pull()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::incoming, Core::ICore::dialogParent());
    dialog.setWindowTitle(tr("Pull Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.synchronousPull(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::outgoing, Core::ICore::dialogParent());
    dialog.setWindowTitle(tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.synchronousPush(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::update()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog updateDialog(Core::ICore::dialogParent());
    updateDialog.setWindowTitle(tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    m_client.update(state.topLevel(), updateDialog.revision());
}

void MercurialPluginPrivate::import()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QFileDialog importDialog(Core::ICore::dialogParent());
    importDialog.setFileMode(QFileDialog::ExistingFiles);
    importDialog.setViewMode(QFileDialog::Detail);

    if (importDialog.exec() != QDialog::Accepted)
        return;

    const QStringList fileNames = importDialog.selectedFiles();
    m_client.import(state.topLevel(), fileNames);
}

void MercurialPluginPrivate::incoming()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::incoming, Core::ICore::dialogParent());
    dialog.setWindowTitle(tr("Incoming Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.incoming(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::outgoing()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client.outgoing(state.topLevel());
}

void MercurialPluginPrivate::commit()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(&m_client, &MercurialClient::parsedStatus, this, &MercurialPluginPrivate::showCommitWidget);
    m_client.emitParsedStatus(m_submitRepository);
}

void MercurialPluginPrivate::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(&m_client, &MercurialClient::parsedStatus, this, &MercurialPluginPrivate::showCommitWidget);

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
    auto commitEditor = static_cast<CommitEditor *>(editor);
    setSubmitEditor(commitEditor);

    connect(commitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &MercurialPluginPrivate::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
                        arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->document()->setPreferredDisplayName(msg);

    const QString branch = vcsTopic(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            m_settings.stringValue(MercurialSettings::userNameKey),
                            m_settings.stringValue(MercurialSettings::userEmailKey), status);
}

void MercurialPluginPrivate::diffFromEditorSelected(const QStringList &files)
{
    m_client.diff(m_submitRepository, files);
}

void MercurialPluginPrivate::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    Core::EditorManager::closeDocument(submitEditor()->document());
}

bool MercurialPluginPrivate::submitEditorAboutToClose()
{
    auto commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    Core::IDocument *editorFile = commitEditor->document();
    QTC_ASSERT(editorFile, return true);

    const VcsBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(this, nullptr, !m_submitActionTriggered);
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
        m_client.commit(m_submitRepository, files, editorFile->filePath().toString(),
                        extraOptions);
    }
    return true;
}

void MercurialPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
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

QString MercurialPluginPrivate::displayName() const
{
    return tr("Mercurial");
}

Utils::Id MercurialPluginPrivate::id() const
{
    return {VcsBase::Constants::VCS_ID_MERCURIAL};
}

bool MercurialPluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &fileName) const
{
    return m_client.isVcsDirectory(fileName);
}

bool MercurialPluginPrivate::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = m_client.findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool MercurialPluginPrivate::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_client.managesFile(workingDirectory, fileName);
}

bool MercurialPluginPrivate::isConfigured() const
{
    const Utils::FilePath binary = m_settings.binaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool MercurialPluginPrivate::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();
    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
    case Core::IVersionControl::MoveOperation:
    case Core::IVersionControl::CreateRepositoryOperation:
    case Core::IVersionControl::AnnotateOperation:
    case Core::IVersionControl::InitialCheckoutOperation:
        break;
    case Core::IVersionControl::SnapshotOperations:
        supported = false;
        break;
    }
    return supported;
}

bool MercurialPluginPrivate::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool MercurialPluginPrivate::vcsAdd(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_client.synchronousAdd(fi.absolutePath(), fi.fileName());
}

bool MercurialPluginPrivate::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_client.synchronousRemove(fi.absolutePath(), fi.fileName());
}

bool MercurialPluginPrivate::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_client.synchronousMove(fromInfo.absolutePath(),
                                            fromInfo.absoluteFilePath(),
                                            toInfo.absoluteFilePath());
}

bool MercurialPluginPrivate::vcsCreateRepository(const QString &directory)
{
    return m_client.synchronousCreateRepository(directory);
}

void MercurialPluginPrivate::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_client.annotate(fi.absolutePath(), fi.fileName(), QString(), line);
}

Core::ShellCommand *MercurialPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                                   const Utils::FilePath &baseDirectory,
                                                                   const QString &localName,
                                                                   const QStringList &extraArgs)
{
    QStringList args;
    args << QLatin1String("clone") << extraArgs << url << localName;
    auto command = new VcsBase::VcsCommand(baseDirectory.toString(),
                                           m_client.processEnvironment());
    command->addJob({m_settings.binaryPath(), args}, -1);
    return command;
}

bool MercurialPluginPrivate::sccManaged(const QString &filename)
{
    const QFileInfo fi(filename);
    QString topLevel;
    const bool managed = managesDirectory(fi.absolutePath(), &topLevel);
    if (!managed || topLevel.isEmpty())
        return false;
    const QDir topLevelDir(topLevel);
    return m_client.manifestSync(topLevel, topLevelDir.relativeFilePath(filename));
}

void MercurialPluginPrivate::changed(const QVariant &v)
{
    switch (v.type()) {
    case QVariant::String:
        emit repositoryChanged(v.toString());
        break;
    case QVariant::StringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
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
    VcsBaseEditorWidget::testDiffFileResolving(dd->diffEditorFactory);
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
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data, "18473:692cbda1eb50", "18472:37100f30590f");
}
#endif

} // namespace Internal
} // namespace Mercurial
