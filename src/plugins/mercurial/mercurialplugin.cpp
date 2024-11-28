// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commiteditor.h"
#include "constants.h"
#include "mercurialclient.h"
#include "mercurialeditor.h"
#include "mercurialsettings.h"
#include "mercurialtr.h"
#include "revertdialog.h"
#include "srcdestdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/iplugin.h>

#include <utils/action.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbasetr.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QtGlobal>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace VcsBase;
using namespace Utils;
using namespace std::placeholders;

namespace Mercurial::Internal {

class MercurialPluginPrivate final : public VcsBase::VersionControlBase
{
public:
    MercurialPluginPrivate();

    // IVersionControl
    QString displayName() const final { return "Mercurial"; }
    Utils::Id id() const final;
    bool isVcsFileOrDirectory(const FilePath &filePath) const final;

    bool managesDirectory(const FilePath &filePath, FilePath *topLevel = nullptr) const final;
    bool managesFile(const FilePath &workingDirectory, const QString &fileName) const final;
    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const FilePath &filePath) final;
    bool vcsAdd(const FilePath &filePath) final;
    bool vcsDelete(const FilePath &filePath) final;
    bool vcsMove(const FilePath &from, const FilePath &to) final;
    bool vcsCreateRepository(const FilePath &directory) final;
    void vcsAnnotate(const FilePath &filePath, int line) final;
    void vcsLog(const Utils::FilePath &topLevel, const Utils::FilePath &relativeDirectory) final {
        mercurialClient().log(topLevel, {relativeDirectory.path()});
    }
    void vcsDescribe(const FilePath &source, const QString &id) final
    {
        mercurialClient().view(source, id);
    }

    VcsCommand *createInitialCheckoutCommand(const QString &url,
                                             const Utils::FilePath &baseDirectory,
                                             const QString &localName,
                                             const QStringList &extraArgs) final;

    bool sccManaged(const QString &filename);

    // To be connected to the HgTask's success signal to emit the repository/
    // files changed signals according to the variant's type:
    // String -> repository, StringList -> files
    void changed(const QVariant&);

private:
    void updateActions(VcsBase::VersionControlBase::ActionState) final;
    bool activateCommit() final;

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
    void diffFromEditorSelected(const QStringList &files);

    void createMenu(const Core::Context &context);
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    Core::CommandLocator *m_commandLocator = nullptr;
    Core::ActionContainer *m_mercurialContainer = nullptr;

    QList<QAction *> m_repositoryActionList;

    // Menu items (file actions)
    Action *m_addAction = nullptr;
    Action *m_deleteAction = nullptr;
    Action *annotateFile = nullptr;
    Action *diffFile = nullptr;
    Action *logFile = nullptr;
    Action *revertFile = nullptr;
    Action *statusFile = nullptr;

    QAction *m_createRepositoryAction = nullptr;
    QAction *m_menuAction = nullptr;

    FilePath m_submitRepository;

public:
    VcsEditorFactory logEditorFactory {{
        LogOutput,
        Constants::FILELOG_ID,
        ::VcsBase::Tr::tr("Mercurial File Log Editor"),
        Constants::LOGAPP,
        [] { return new MercurialEditorWidget; },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    }};

    VcsEditorFactory annotateEditorFactory {{
        AnnotateOutput,
        Constants::ANNOTATELOG_ID,
        ::VcsBase::Tr::tr("Mercurial Annotation Editor"),
        Constants::ANNOTATEAPP,
        [] { return new MercurialEditorWidget; },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    }};

    VcsEditorFactory diffEditorFactory {{
        DiffOutput,
        Constants::DIFFLOG_ID,
        ::VcsBase::Tr::tr("Mercurial Diff Editor"),
        Constants::DIFFAPP,
        [] { return new MercurialEditorWidget; },
        std::bind(&MercurialPluginPrivate::vcsDescribe, this, _1, _2)
    }};
};

static MercurialPluginPrivate *dd = nullptr;

MercurialPluginPrivate::MercurialPluginPrivate()
    : VcsBase::VersionControlBase(Core::Context(Constants::MERCURIAL_CONTEXT))
{
    dd = this;

    setupVcsSubmitEditor(this, {
        Constants::COMMITMIMETYPE,
        Constants::COMMIT_ID,
        ::VcsBase::Tr::tr("Mercurial Commit Log Editor"),
        VcsBaseSubmitEditorParameters::DiffFiles,
        [] { return new CommitEditor; }
    });

    setTopicFileTracker([](const FilePath &repository) {
        return repository.pathAppended(".hg/branch");
    });
    setTopicRefresher([](const FilePath &repository) {
        return mercurialClient().branchQuerySync(repository.toString());
    });

    Core::Context context(Constants::MERCURIAL_CONTEXT);

    connect(&mercurialClient(), &VcsBaseClient::changed, this, &MercurialPluginPrivate::changed);
    connect(&mercurialClient(), &MercurialClient::needUpdate, this, &MercurialPluginPrivate::update);

    const QString prefix = QLatin1String("hg");
    m_commandLocator = new Core::CommandLocator("Mercurial", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Mercurial version control operation."));

    createMenu(context);

    connect(&settings(), &AspectContainer::applied, this, &IVersionControl::configurationChanged);
}

void MercurialPluginPrivate::createMenu(const Core::Context &context)
{
    // Create menu item for Mercurial
    m_mercurialContainer = Core::ActionManager::createMenu("Mercurial.MercurialMenu");
    QMenu *menu = m_mercurialContainer->menu();
    menu->setTitle(Tr::tr("Me&rcurial"));

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

    annotateFile = new Action(Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(annotateFile, Utils::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(annotateFile, &QAction::triggered, this, &MercurialPluginPrivate::annotateCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    diffFile = new Action(Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(diffFile, Utils::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+H,Meta+D") : Tr::tr("Alt+G,Alt+D")));
    connect(diffFile, &QAction::triggered, this, &MercurialPluginPrivate::diffCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    logFile = new Action(Tr::tr("Log Current File"), Tr::tr("Log \"%1\""), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(logFile, Utils::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+H,Meta+L") : Tr::tr("Alt+G,Alt+L")));
    connect(logFile, &QAction::triggered, this, &MercurialPluginPrivate::logCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    statusFile = new Action(Tr::tr("Status Current File"), Tr::tr("Status \"%1\""), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(statusFile, Utils::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+H,Meta+S") : Tr::tr("Alt+G,Alt+S")));
    connect(statusFile, &QAction::triggered, this, &MercurialPluginPrivate::statusCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_mercurialContainer->addSeparator(context);

    m_addAction = new Action(Tr::tr("Add"), Tr::tr("Add \"%1\""), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, Utils::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &MercurialPluginPrivate::addCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Action(Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."), Action::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, Utils::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &MercurialPluginPrivate::promptToDeleteCurrentFile);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    revertFile = new Action(Tr::tr("Revert Current File..."), Tr::tr("Revert \"%1\"..."), Action::EnabledWithParameter, this);
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
    mercurialClient().synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPluginPrivate::annotateCurrentFile()
{
    int currentLine = -1;
    if (Core::IEditor *editor = Core::EditorManager::currentEditor())
        currentLine = editor->currentLine();
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    mercurialClient().annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), currentLine);
}

void MercurialPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    mercurialClient().showDiffEditor(state.currentFileTopLevel(), {state.relativeCurrentFile()});
}

void MercurialPluginPrivate::logCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    mercurialClient().log(state.currentFileTopLevel(), {state.relativeCurrentFile()}, {}, true);
}

void MercurialPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    RevertDialog reverter(Core::ICore::dialogParent());
    if (reverter.exec() != QDialog::Accepted)
        return;
    mercurialClient().revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
}

void MercurialPluginPrivate::statusCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    mercurialClient().status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void MercurialPluginPrivate::createDirectoryActions(const Core::Context &context)
{
    auto action = new QAction(Tr::tr("Diff"), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Utils::Id(Constants::DIFFMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::diffRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Log"), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::LOGMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::logRepository);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::REVERTMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::revertMulti);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Status"), this);
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
    mercurialClient().showDiffEditor(state.topLevel());
}

void MercurialPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    mercurialClient().log(state.topLevel());
}

void MercurialPluginPrivate::revertMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog reverter(Core::ICore::dialogParent());
    if (reverter.exec() != QDialog::Accepted)
        return;
    mercurialClient().revertAll(state.topLevel(), reverter.revision());
}

void MercurialPluginPrivate::statusMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    mercurialClient().status(state.topLevel());
}

void MercurialPluginPrivate::createRepositoryActions(const Core::Context &context)
{
    auto action = new QAction(Tr::tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Core::Command *command = Core::ActionManager::registerAction(action, Utils::Id(Constants::PULL), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::pull);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::PUSH), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::push);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::UPDATE), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::update);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Import..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::IMPORT), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::import);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Incoming..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::INCOMING), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::incoming);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Outgoing..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::OUTGOING), context);
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::outgoing);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = Core::ActionManager::registerAction(action, Utils::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+H,Meta+C") : Tr::tr("Alt+G,Alt+C")));
    connect(action, &QAction::triggered, this, &MercurialPluginPrivate::commit);
    m_mercurialContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_createRepositoryAction = new QAction(Tr::tr("Create Repository..."), this);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Utils::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, &QAction::triggered, this, &MercurialPluginPrivate::createRepository);
    m_mercurialContainer->addAction(command);
}

void MercurialPluginPrivate::pull()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::incoming, Core::ICore::dialogParent());
    dialog.setWindowTitle(Tr::tr("Pull Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    mercurialClient().synchronousPull(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::outgoing, Core::ICore::dialogParent());
    dialog.setWindowTitle(Tr::tr("Push Destination"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    mercurialClient().synchronousPush(dialog.workingDir(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::update()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog updateDialog(Core::ICore::dialogParent());
    updateDialog.setWindowTitle(Tr::tr("Update"));
    if (updateDialog.exec() != QDialog::Accepted)
        return;
    mercurialClient().update(state.topLevel(), updateDialog.revision());
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
    mercurialClient().import(state.topLevel(), fileNames);
}

void MercurialPluginPrivate::incoming()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    SrcDestDialog dialog(state, SrcDestDialog::incoming, Core::ICore::dialogParent());
    dialog.setWindowTitle(Tr::tr("Incoming Source"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    mercurialClient().incoming(state.topLevel(), dialog.getRepositoryString());
}

void MercurialPluginPrivate::outgoing()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    mercurialClient().outgoing(state.topLevel());
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

    connect(&mercurialClient(), &MercurialClient::parsedStatus,
            this, &MercurialPluginPrivate::showCommitWidget);

    mercurialClient().emitParsedStatus(m_submitRepository);
}

void MercurialPluginPrivate::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(&mercurialClient(), &MercurialClient::parsedStatus,
               this, &MercurialPluginPrivate::showCommitWidget);

    if (status.isEmpty()) {
        VcsOutputWindow::appendError(Tr::tr("There are no changes to commit."));
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

    Core::IEditor *editor = Core::EditorManager::openEditor(saver.filePath(), Constants::COMMIT_ID);
    if (!editor) {
        VcsOutputWindow::appendError(Tr::tr("Unable to create an editor for the commit."));
        return;
    }

    QTC_ASSERT(qobject_cast<CommitEditor *>(editor), return);
    auto commitEditor = static_cast<CommitEditor *>(editor);
    setSubmitEditor(commitEditor);

    connect(commitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &MercurialPluginPrivate::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = Tr::tr("Commit changes for \"%1\".").arg(m_submitRepository.toUserOutput());
    commitEditor->document()->setPreferredDisplayName(msg);

    const QString branch = vcsTopic(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            settings().userName(),
                            settings().userEmail(), status);
}

void MercurialPluginPrivate::diffFromEditorSelected(const QStringList &files)
{
    mercurialClient().showDiffEditor(m_submitRepository, files);
}

bool MercurialPluginPrivate::activateCommit()
{
    auto commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    Core::IDocument *editorFile = commitEditor->document();
    QTC_ASSERT(editorFile, return true);

    const QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!Core::DocumentManager::saveDocument(editorFile))
            return false;

        QStringList extraOptions;
        if (!commitEditor->committerInfo().isEmpty())
            extraOptions << QLatin1String("-u") << commitEditor->committerInfo();
        mercurialClient().commit(m_submitRepository, files, editorFile->filePath().toString(),
                                 extraOptions);
    }
    return true;
}

void MercurialPluginPrivate::updateActions(VersionControlBase::ActionState as)
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

    for (QAction *repoAction : std::as_const(m_repositoryActionList))
        repoAction->setEnabled(repoEnabled);
}

Utils::Id MercurialPluginPrivate::id() const
{
    return {VcsBase::Constants::VCS_ID_MERCURIAL};
}

bool MercurialPluginPrivate::isVcsFileOrDirectory(const FilePath &filePath) const
{
    return mercurialClient().isVcsDirectory(filePath);
}

bool MercurialPluginPrivate::managesDirectory(const FilePath &filePath, FilePath *topLevel) const
{
    const FilePath topLevelFound = Core::VcsManager::findRepositoryForFiles(
        filePath, {QString(Constants::MERCURIALREPO) + "/requires"});
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool MercurialPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    return mercurialClient().managesFile(workingDirectory, fileName);
}

bool MercurialPluginPrivate::isConfigured() const
{
    const FilePath binary = settings().binaryPath.effectiveBinary();
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

bool MercurialPluginPrivate::vcsOpen(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    return true;
}

bool MercurialPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return mercurialClient().synchronousAdd(filePath.parentDir(), filePath.fileName());
}

bool MercurialPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return mercurialClient().synchronousRemove(filePath.parentDir(), filePath.fileName());
}

bool MercurialPluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return mercurialClient().synchronousMove(from.parentDir(),
                                    fromInfo.absoluteFilePath(),
                                    toInfo.absoluteFilePath());
}

bool MercurialPluginPrivate::vcsCreateRepository(const FilePath &directory)
{
    return mercurialClient().synchronousCreateRepository(directory);
}

void MercurialPluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    mercurialClient().annotate(filePath.parentDir(), filePath.fileName(), line);
}

VcsCommand *MercurialPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                                 const Utils::FilePath &baseDirectory,
                                                                 const QString &localName,
                                                                 const QStringList &extraArgs)
{
    auto command = VcsBaseClient::createVcsCommand(baseDirectory,
                   mercurialClient().processEnvironment(baseDirectory));
    command->addJob({settings().binaryPath(), {"clone", extraArgs, url, localName}}, -1);
    return command;
}

bool MercurialPluginPrivate::sccManaged(const QString &filename)
{
    const QFileInfo fi(filename);
    FilePath topLevel;
    const bool managed = managesDirectory(FilePath::fromString(fi.absolutePath()), &topLevel);
    if (!managed || topLevel.isEmpty())
        return false;
    const QDir topLevelDir(topLevel.toString());
    return mercurialClient().manifestSync(topLevel, topLevelDir.relativeFilePath(filename));
}

void MercurialPluginPrivate::changed(const QVariant &v)
{
    switch (v.typeId()) {
    case QMetaType::QString:
        emit repositoryChanged(FilePath::fromVariant(v));
        break;
    case QMetaType::QStringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
}

#ifdef WITH_TESTS

class MercurialTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
};

void MercurialTest::testDiffFileResolving_data()
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

void MercurialTest::testDiffFileResolving()
{
    VcsBaseEditorWidget::testDiffFileResolving(dd->diffEditorFactory);
}

void MercurialTest::testLogResolving()
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

class MercurialPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Mercurial.json")

    ~MercurialPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new MercurialPluginPrivate;
#ifdef WITH_TESTS
        addTest<MercurialTest>();
#endif
    }

    void extensionsInitialized() final
    {
        dd->extensionsInitialized();
    }
};

} // Mercurial::Internal

#include "mercurialplugin.moc"
