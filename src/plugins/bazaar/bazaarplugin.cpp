// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaarplugin.h"

#include "bazaarclient.h"
#include "bazaarcommitwidget.h"
#include "bazaareditor.h"
#include "bazaarsettings.h"
#include "bazaartr.h"
#include "commiteditor.h"
#include "constants.h"
#include "pullorpushdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/layoutbuilder.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace Utils;
using namespace VcsBase;
using namespace std::placeholders;

namespace Bazaar::Internal {

// Submit editor parameters
const char COMMIT_ID[] = "Bazaar Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Bazaar Commit Log Editor");
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

const VcsBaseEditorParameters logEditorParameters {
    LogOutput, // type
    Constants::FILELOG_ID, // id
    Constants::FILELOG_DISPLAY_NAME, // display name
    Constants::LOGAPP // mime type
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
    COMMITMIMETYPE,
    COMMIT_ID,
    COMMIT_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

class RevertDialog : public QDialog
{
public:
    RevertDialog() : QDialog(ICore::dialogParent())
    {
        resize(400, 162);
        setWindowTitle(Tr::tr("Revert"));

        auto groupBox = new QGroupBox(Tr::tr("Specify a revision other than the default?"));
        groupBox->setCheckable(true);
        groupBox->setChecked(false);

        revisionLineEdit = new QLineEdit;

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        using namespace Layouting;
        Form {
            Tr::tr("Revision:"), revisionLineEdit
        }.attachTo(groupBox);

        Column {
            groupBox,
            buttonBox,
        }.attachTo(this);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QLineEdit *revisionLineEdit;
};

class BazaarPluginPrivate final : public VcsBasePluginPrivate
{
public:
    BazaarPluginPrivate();

    QString displayName() const final;
    Utils::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const final;

    bool managesDirectory(const Utils::FilePath &filePath, Utils::FilePath *topLevel) const final;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const final;
    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const Utils::FilePath &filePath) final;
    bool vcsAdd(const Utils::FilePath &filePath) final;
    bool vcsDelete(const Utils::FilePath &filePath) final;
    bool vcsMove(const Utils::FilePath &from, const Utils::FilePath &to) final;
    bool vcsCreateRepository(const Utils::FilePath &directory) final;
    void vcsAnnotate(const Utils::FilePath &file, int line) final;
    void vcsDescribe(const Utils::FilePath &source, const QString &id) final { m_client.view(source, id); }

    VcsCommand *createInitialCheckoutCommand(const QString &url,
                                             const Utils::FilePath &baseDirectory,
                                             const QString &localName,
                                             const QStringList &extraArgs) final;

    // To be connected to the VCSTask's success signal to emit the repository/
    // files changed signals according to the variant's type:
    // String -> repository, StringList -> files
    void changed(const QVariant &);
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) final;
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
    void revertAll();
    void statusMulti();

    // Repository menu action slots
    void pull();
    void push();
    void update();
    void commit();
    void showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status);
    void uncommit();
    void diffFromEditorSelected(const QStringList &files);

    // Functions
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    BazaarClient m_client;

    VcsSubmitEditorFactory m_submitEditorFactory {
        submitEditorParameters,
        [] { return new CommitEditor; },
        this
    };
    Core::CommandLocator *m_commandLocator = nullptr;
    Core::ActionContainer *m_bazaarContainer = nullptr;

    QList<QAction *> m_repositoryActionList;

    // Menu Items (file actions)
    ParameterAction *m_addAction = nullptr;
    ParameterAction *m_deleteAction = nullptr;
    ParameterAction *m_annotateFile = nullptr;
    ParameterAction *m_diffFile = nullptr;
    ParameterAction *m_logFile = nullptr;
    ParameterAction *m_revertFile = nullptr;
    ParameterAction *m_statusFile = nullptr;

    QAction *m_menuAction = nullptr;

    FilePath m_submitRepository;

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new BazaarEditorWidget; },
        std::bind(&BazaarPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory annotateEditorFactory {
        &annotateEditorParameters,
        [] { return new BazaarEditorWidget; },
        std::bind(&BazaarPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory diffEditorFactory {
        &diffEditorParameters,
        [] { return new BazaarEditorWidget; },
        std::bind(&BazaarPluginPrivate::vcsDescribe, this, _1, _2)
    };
};

class UnCommitDialog : public QDialog
{
public:
    explicit UnCommitDialog(BazaarPluginPrivate *plugin)
        : QDialog(ICore::dialogParent())
    {
        resize(412, 124);
        setWindowTitle(Tr::tr("Uncommit"));

        keepTagsCheckBox = new QCheckBox(Tr::tr("Keep tags that point to removed revisions"));

        localCheckBox = new QCheckBox(Tr::tr("Only remove the commits from the local branch when in a checkout"));

        revisionLineEdit = new QLineEdit(this);
        revisionLineEdit->setToolTip(Tr::tr("If a revision is specified, uncommits revisions to leave "
            "the branch at the specified revision.\n"
            "For example, \"Revision: 15\" will leave the branch at revision 15."));
        revisionLineEdit->setPlaceholderText(Tr::tr("Last committed"));

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        auto dryRunBtn = new QPushButton(Tr::tr("Dry Run"));
        dryRunBtn->setToolTip(Tr::tr("Test the outcome of removing the last committed revision, without actually removing anything."));
        buttonBox->addButton(dryRunBtn, QDialogButtonBox::ApplyRole);

        using namespace Layouting;
        Column {
            Form {
                keepTagsCheckBox, br,
                localCheckBox, br,
                Tr::tr("Revision:"), revisionLineEdit, br,
            },
            st,
            buttonBox,
        }.attachTo(this);

        connect(dryRunBtn, &QPushButton::clicked, this, [this, plugin] {
            QTC_ASSERT(plugin->currentState().hasTopLevel(), return);
            plugin->m_client.synchronousUncommit(plugin->currentState().topLevel(),
                                                 revision(),
                                                 extraOptions() << "--dry-run");
        });
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QStringList extraOptions() const
    {
        QStringList opts;
        if (keepTagsCheckBox->isChecked())
            opts += "--keep-tags";
        if (localCheckBox->isChecked())
            opts += "--local";
        return opts;
    }

    QString revision() const
    {
        return revisionLineEdit->text().trimmed();
    }

private:
    QCheckBox *keepTagsCheckBox;
    QCheckBox *localCheckBox;
    QLineEdit *revisionLineEdit;
};

BazaarPlugin::~BazaarPlugin()
{
    delete d;
    d = nullptr;
}

void BazaarPlugin::initialize()
{
    d = new BazaarPluginPrivate;
}

void BazaarPlugin::extensionsInitialized()
{
    d->extensionsInitialized();
}

BazaarPluginPrivate::BazaarPluginPrivate()
    : VcsBasePluginPrivate(Context(Constants::BAZAAR_CONTEXT))
{
    Context context(Constants::BAZAAR_CONTEXT);

    connect(&m_client, &VcsBaseClient::changed, this, &BazaarPluginPrivate::changed);

    const QString prefix = QLatin1String("bzr");
    m_commandLocator = new CommandLocator("Bazaar", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Bazaar version control operation."));

    // Create menu item for Bazaar
    m_bazaarContainer = ActionManager::createMenu("Bazaar.BazaarMenu");
    QMenu *menu = m_bazaarContainer->menu();
    menu->setTitle(Tr::tr("Bazaar"));

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

    connect(&settings(), &AspectContainer::applied, this, &IVersionControl::configurationChanged);
}

void BazaarPluginPrivate::createFileActions(const Context &context)
{
    m_annotateFile = new ParameterAction(Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    Command *command = ActionManager::registerAction(m_annotateFile, ANNOTATE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateFile, &QAction::triggered, this, &BazaarPluginPrivate::annotateCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_diffFile = new ParameterAction(Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFile, DIFF, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Z,Meta+D")
                                                                : Tr::tr("Alt+Z,Alt+D")));
    connect(m_diffFile, &QAction::triggered, this, &BazaarPluginPrivate::diffCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logFile = new ParameterAction(Tr::tr("Log Current File"), Tr::tr("Log \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logFile, LOG, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Z,Meta+L")
                                                                : Tr::tr("Alt+Z,Alt+L")));
    connect(m_logFile, &QAction::triggered, this, &BazaarPluginPrivate::logCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusFile = new ParameterAction(Tr::tr("Status Current File"), Tr::tr("Status \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusFile, STATUS, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Z,Meta+S")
                                                                : Tr::tr("Alt+Z,Alt+S")));
    connect(m_statusFile, &QAction::triggered, this, &BazaarPluginPrivate::statusCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_bazaarContainer->addSeparator(context);

    m_addAction = new ParameterAction(Tr::tr("Add"), Tr::tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, ADD, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_addAction, &QAction::triggered, this, &BazaarPluginPrivate::addCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, DELETE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &BazaarPluginPrivate::promptToDeleteCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFile = new ParameterAction(Tr::tr("Revert Current File..."), Tr::tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFile, REVERT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertFile, &QAction::triggered, this, &BazaarPluginPrivate::revertCurrentFile);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void BazaarPluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void BazaarPluginPrivate::logCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), {}, true);
}

void BazaarPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    RevertDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.revertFile(state.currentFileTopLevel(),
                         state.relativeCurrentFile(),
                         dialog.revisionLineEdit->text());
}

void BazaarPluginPrivate::statusCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client.status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPluginPrivate::createDirectoryActions(const Context &context)
{
    auto action = new QAction(Tr::tr("Diff"), this);
    m_repositoryActionList.append(action);
    Command *command = ActionManager::registerAction(action, DIFFMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::diffRepository);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Log"), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, LOGMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::logRepository);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, REVERTMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::revertAll);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Status"), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, STATUSMULTI, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::statusMulti);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void BazaarPluginPrivate::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client.diff(state.topLevel());
}

void BazaarPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QStringList extraOptions;
    extraOptions += "--limit=" + QString::number(settings().logCount());
    m_client.log(state.topLevel(), QStringList(), extraOptions);
}

void BazaarPluginPrivate::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.revertAll(state.topLevel(), dialog.revisionLineEdit->text());
}

void BazaarPluginPrivate::statusMulti()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client.status(state.topLevel());
}

void BazaarPluginPrivate::createRepositoryActions(const Context &context)
{
    auto action = new QAction(Tr::tr("Pull..."), this);
    m_repositoryActionList.append(action);
    Command *command = ActionManager::registerAction(action, PULL, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::pull);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, PUSH, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::push);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, UPDATE, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::update);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, COMMIT, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Z,Meta+C")
                                                                : Tr::tr("Alt+Z,Alt+C")));
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::commit);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(Tr::tr("Uncommit..."), this);
    m_repositoryActionList.append(action);
    command = ActionManager::registerAction(action, UNCOMMIT, context);
    connect(action, &QAction::triggered, this, &BazaarPluginPrivate::uncommit);
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    auto createRepositoryAction = new QAction(Tr::tr("Create Repository..."), this);
    command = ActionManager::registerAction(createRepositoryAction, CREATE_REPOSITORY, context);
    connect(createRepositoryAction, &QAction::triggered, this, &BazaarPluginPrivate::createRepository);
    m_bazaarContainer->addAction(command);
}

void BazaarPluginPrivate::pull()
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
    m_client.synchronousPull(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPluginPrivate::push()
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
    m_client.synchronousPush(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPluginPrivate::update()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    RevertDialog dialog;
    dialog.setWindowTitle(Tr::tr("Update"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client.update(state.topLevel(), dialog.revisionLineEdit->text());
}

void BazaarPluginPrivate::commit()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    QObject::connect(&m_client, &VcsBaseClient::parsedStatus,
                     this, &BazaarPluginPrivate::showCommitWidget);
    // The "--short" option allows to easily parse status output
    m_client.emitParsedStatus(m_submitRepository, QStringList(QLatin1String("--short")));
}

void BazaarPluginPrivate::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
{
    //Once we receive our data release the connection so it can be reused elsewhere
    QObject::disconnect(&m_client, &VcsBaseClient::parsedStatus,
                        this, &BazaarPluginPrivate::showCommitWidget);

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

    IEditor *editor = EditorManager::openEditor(saver.filePath(), COMMIT_ID);
    if (!editor) {
        VcsOutputWindow::appendError(Tr::tr("Unable to create an editor for the commit."));
        return;
    }

    auto commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        VcsOutputWindow::appendError(Tr::tr("Unable to create a commit editor."));
        return;
    }
    setSubmitEditor(commitEditor);

    connect(commitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &BazaarPluginPrivate::diffFromEditorSelected);
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = Tr::tr("Commit changes for \"%1\".").arg(m_submitRepository.toUserOutput());
    commitEditor->document()->setPreferredDisplayName(msg);

    const BranchInfo branch = m_client.synchronousBranchQuery(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            settings().userName(),
                            settings().userEmail(), status);
}

void BazaarPluginPrivate::diffFromEditorSelected(const QStringList &files)
{
    m_client.diff(m_submitRepository, files);
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
    VcsBaseEditorWidget::testDiffFileResolving(d->diffEditorFactory);
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
    VcsBaseEditorWidget::testLogResolving(d->logEditorFactory, data, "6572", "6571");
}
#endif

void BazaarPluginPrivate::uncommit()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    UnCommitDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        m_client.synchronousUncommit(state.topLevel(), dialog.revision(), dialog.extraOptions());
}

bool BazaarPluginPrivate::activateCommit()
{
    auto commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    IDocument *editorDocument = commitEditor->document();
    QTC_ASSERT(editorDocument, return true);

    QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!DocumentManager::saveDocument(editorDocument))
            return false;

        //rewrite entries of the form 'file => newfile' to 'newfile' because
        //this would mess the commit command
        for (QStringList::iterator iFile = files.begin(); iFile != files.end(); ++iFile) {
            const QStringList parts = iFile->split(QLatin1String(" => "), Qt::SkipEmptyParts);
            if (!parts.isEmpty())
                *iFile = parts.last();
        }

        BazaarCommitWidget *commitWidget = commitEditor->commitWidget();
        QStringList extraOptions;
        // Author
        if (!commitWidget->committer().isEmpty())
            extraOptions.append(QLatin1String("--author=") + commitWidget->committer());
        // Fixed bugs
        const QStringList fixes = commitWidget->fixedBugs();
        for (const QString &fix : fixes) {
            if (!fix.isEmpty())
                extraOptions << QLatin1String("--fixes") << fix;
        }
        // Whether local commit or not
        if (commitWidget->isLocalOptionEnabled())
            extraOptions += QLatin1String("--local");
        m_client.commit(m_submitRepository, files, editorDocument->filePath().toString(), extraOptions);
    }
    return true;
}

void BazaarPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
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

    for (QAction *repoAction : std::as_const(m_repositoryActionList))
        repoAction->setEnabled(repoEnabled);
}

QString BazaarPluginPrivate::displayName() const
{
    return Tr::tr("Bazaar");
}

Utils::Id BazaarPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_BAZAAR);
}

bool BazaarPluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &fileName) const
{
    return m_client.isVcsDirectory(fileName);
}

bool BazaarPluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel) const
{
    const FilePath topLevelFound = m_client.findTopLevelForFile(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool BazaarPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    return m_client.managesFile(workingDirectory, fileName);
}

bool BazaarPluginPrivate::isConfigured() const
{
    const FilePath binary = settings().binaryPath();
    return !binary.isEmpty() && binary.isExecutableFile();
}

bool BazaarPluginPrivate::supportsOperation(Operation operation) const
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

bool BazaarPluginPrivate::vcsOpen(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    return true;
}

bool BazaarPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return m_client.synchronousAdd(filePath.parentDir(), filePath.fileName());
}

bool BazaarPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return m_client.synchronousRemove(filePath.parentDir(), filePath.fileName());
}

bool BazaarPluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return m_client.synchronousMove(from.absolutePath(),
                                    fromInfo.absoluteFilePath(),
                                    toInfo.absoluteFilePath());
}

bool BazaarPluginPrivate::vcsCreateRepository(const FilePath &directory)
{
    return m_client.synchronousCreateRepository(directory);
}

void BazaarPluginPrivate::vcsAnnotate(const FilePath &file, int line)
{
    m_client.annotate(file.parentDir(), file.fileName(), line);
}

VcsCommand *BazaarPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                              const FilePath &baseDirectory,
                                                              const QString &localName,
                                                              const QStringList &extraArgs)
{
    QStringList args;
    args << m_client.vcsCommandString(BazaarClient::CloneCommand)
         << extraArgs << url << localName;

    Environment env = m_client.processEnvironment();
    env.set("BZR_PROGRESS_BAR", "text");
    auto command = VcsBaseClient::createVcsCommand(baseDirectory, env);
    command->addJob({m_client.vcsBinary(), args}, -1);
    return command;
}

void BazaarPluginPrivate::changed(const QVariant &v)
{
    switch (v.type()) {
    case QVariant::String:
        emit repositoryChanged(FilePath::fromVariant(v));
        break;
    case QVariant::StringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
}

} // Bazaar::Internal
